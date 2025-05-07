#define _POSIX_C_SOURCE 199309L
#include "utils.h"
#include "ring.h"

#include <unistd.h>
#include <semaphore.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

extern ring_t* ring;
extern pthread_mutex_t ring_mutex;
extern pthread_cond_t not_empty;
extern pthread_cond_t not_full;
_Thread_local volatile sig_atomic_t thread_continue = 1;

atomic_int producer_count = 0;
atomic_int consumer_count = 0;

void thread_stop_handler(int signo) 
{
    if (signo == SIGUSR1) {
        thread_continue = 0;
    }
}

#define SLEEP_TIME 2

void* consumer_routine(void* arg)
{
    struct sigaction sa;
    sa.sa_handler = thread_stop_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);

    sem_t* consumer = sem_open("/consumer", 0);
    sem_t* mutex = sem_open("/mutex", 0);
    
    if (consumer == SEM_FAILED || mutex == SEM_FAILED) {
        perror("sem_open failed in consumer");
        return NULL;
    }

    while (thread_continue) 
    {
        if (sem_wait(consumer) != 0) {
            if (errno == EINTR) continue; 
            perror("sem_wait failed in consumer");
            break;
        }
        
        if (sem_wait(mutex) != 0) {
            sem_post(consumer); 
            if (errno == EINTR) continue;
            perror("sem_wait failed in consumer");
            break;
        }

        if (ring->cur > 0) 
        {  
            mes_t* temp = ring->head->message;

            printf("--Ejected %ld message:\n", ring->deleted);
            print_mes(temp);
            pop(&ring->head, &ring->tail);

            ring->deleted++;
            ring->cur--;
        }

        sem_post(mutex);
        sem_post(consumer);

        sleep(SLEEP_TIME);
    }

    sem_close(consumer);
    sem_close(mutex);

    printf("\nConsumer %d has finished\n", atomic_fetch_add(&consumer_count, 1) + 1);
    return NULL;
}

void* producer_routine(void* arg)
{
    struct sigaction sa;
    sa.sa_handler = thread_stop_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);

    sem_t* producer = sem_open("/producer", 0);
    sem_t* mutex = sem_open("/mutex", 0);
    
    if (producer == SEM_FAILED || mutex == SEM_FAILED) {
        perror("sem_open failed in producer");
        return NULL;
    }

    while (thread_continue) 
    {
        if (sem_wait(producer) != 0) {
            if (errno == EINTR) continue;
            perror("sem_wait failed in producer");
            break;
        }
        
        if (sem_wait(mutex) != 0) {
            sem_post(producer);
            if (errno == EINTR) continue;
            perror("sem_wait failed in producer");
            break;
        }

        if (ring->cur < ring->size) 
        {  
            push(&ring->head, &ring->tail);

            ring->added++;
            ring->cur++;

            printf("--Append %ld message:\n", ring->added);
            print_mes(ring->tail->message);
        }

        sem_post(mutex);
        sem_post(producer);

        sleep(SLEEP_TIME);
    }

    sem_close(producer);
    sem_close(mutex);


    printf("\nProducer %d has finished\n", atomic_fetch_add(&producer_count, 1) + 1);
    return NULL;
}

void* consumer_routine_cond(void* arg)
{
    struct sigaction sa;
    sa.sa_handler = thread_stop_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);

    while (thread_continue) 
    {
        pthread_mutex_lock(&ring_mutex);
        
        while (ring->cur <= 0 && thread_continue) {
            pthread_cond_wait(&not_empty, &ring_mutex);
        }
        
        if (!thread_continue) {
            pthread_mutex_unlock(&ring_mutex);
            break;
        }

        if (ring->cur > 0) 
        {  
            mes_t* temp = ring->head->message;

            printf("--Ejected %ld message:\n", ring->deleted);
            print_mes(temp);
            pop(&ring->head, &ring->tail);

            ring->deleted++;
            ring->cur--;

            pthread_cond_signal(&not_full);
        }

        pthread_mutex_unlock(&ring_mutex);
        sleep(SLEEP_TIME);
    }

    printf("\nConsumer %d has finished\n", atomic_fetch_add(&consumer_count, 1) + 1);
    return NULL;
}

void* producer_routine_cond(void* arg)
{
    struct sigaction sa;
    sa.sa_handler = thread_stop_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);

    while (thread_continue) 
    {
        pthread_mutex_lock(&ring_mutex);
        
        while (ring->cur >= ring->size && thread_continue) {
            pthread_cond_wait(&not_full, &ring_mutex);
        }
        
        if (!thread_continue) {
            pthread_mutex_unlock(&ring_mutex);
            break;
        }

        if (ring->cur < ring->size) 
        {  
            push(&ring->head, &ring->tail);

            ring->added++;
            ring->cur++;

            printf("--Append %ld message:\n", ring->added);
            print_mes(ring->tail->message);

            pthread_cond_signal(&not_empty);
        }

        pthread_mutex_unlock(&ring_mutex);
        sleep(SLEEP_TIME);
    }

    printf("\nProducer %d has finished\n", atomic_fetch_add(&producer_count, 1) + 1);
    return NULL;
}