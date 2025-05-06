#include "ring.h"
#include "utils.h"

#include <unistd.h>
#include <semaphore.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <pthread.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

volatile sig_atomic_t stop_flag = 0;


#define DEFAULT_RING_SIZE 10
#define MAX_THREADS_COUNT 50

ring_t* ring;
int sync_method = 0; // 0 - semaphores, 1 - condvars


sem_t* producer;
sem_t* consumer;
sem_t* mutex;

// Для реализации с условными переменными
pthread_mutex_t ring_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;

void handle_sigint(int sig);
void graceful_shutdown(
    pthread_t* producer_threads, size_t* producer_threads_count,
    pthread_t* consumer_threads, size_t* consumer_threads_count,
    sem_t* producer, sem_t* consumer, sem_t* mutex);
void initialize_sync_primitives();
void cleanup_sync_primitives();

int main()
{
    signal(SIGINT, handle_sigint);

    printf("Select synchronization method:\n");
    printf("1 - POSIX semaphores and mutex\n");
    printf("2 - Conditional variables\n");
    printf("Your choice: ");
    scanf("%d", &sync_method);
    sync_method--; // 0 or 1

    initialize_sync_primitives();

    pthread_t producer_threads[MAX_THREADS_COUNT];
    pthread_t consumer_threads[MAX_THREADS_COUNT];
    size_t producer_threads_count = 0;
    size_t consumer_threads_count = 0;

    size_t producer_count = 0;
    size_t consumer_count = 0;
    char input[2];

    ring = (ring_t*)calloc(1, sizeof(ring_t));
    if (!ring) {
        perror("Failed to allocate ring");
        exit(EXIT_FAILURE);
    }
    
    // Инициализация кольцевого буфера
    ring->size = DEFAULT_RING_SIZE;
    ring->head = NULL;
    ring->tail = NULL;
    ring->cur = 0;
    ring->added = 0;
    ring->deleted = 0;

    printf("\n=== IPC Producer-Consumer Program ===\n");
    printf("Using %s\n", sync_method == 0 ? "POSIX semaphores" : "conditional variables");
    printf("Commands:\n");
    printf("  p - Add new producer thread\n");
    printf("  c - Add new consumer thread\n");
    printf("  r - Remove last producer thread\n");
    printf("  d - Remove last consumer thread\n");
    printf("  s - Show statistics\n");
    printf("  + - Increase ring buffer size\n");
    printf("  - - Decrease ring buffer size\n");
    printf("  q - Quit program (graceful shutdown)\n");
    printf("  h - help\n");
    printf("\nCurrent ring size: %ld\n", ring->size);

    while (!stop_flag)
    {
        scanf("%s", input);

        switch (input[0])
        {
        case 'p':
        {
            if (producer_threads_count >= MAX_THREADS_COUNT) {
                printf("Max producer threads reached.\n");
                break;
            }
            void* (*routine)(void*) = sync_method == 0 ? producer_routine : producer_routine_cond;
            pthread_create(&producer_threads[producer_threads_count++], NULL, routine, NULL);
            producer_count++;
            break;
        }        
        case 'c':
        {
            if (consumer_threads_count >= MAX_THREADS_COUNT) {
                printf("Max consumer threads reached.\n");
                break;
            }
            void* (*routine)(void*) = sync_method == 0 ? consumer_routine : consumer_routine_cond;
            pthread_create(&consumer_threads[consumer_threads_count++], NULL, routine, NULL);
            consumer_count++;
            break;
        }
        case 's':
        {
            if (sync_method == 0) {
                sem_wait(mutex);
            } else {
                pthread_mutex_lock(&ring_mutex);
            }

            printf("\n=====================\n");
            printf("Added: %ld\nGetted: %ld\nProducers count: %ld\nConsumers count: %ld\nCurrent size: %ld\nMax size: %ld\n", 
                ring->added, 
                ring->deleted, 
                producer_count, 
                consumer_count,
                ring->cur,
                ring->size);
            printf("=====================\n\n");

            if (sync_method == 0) {
                sem_post(mutex);
            } else {
                pthread_mutex_unlock(&ring_mutex);
            }
            break;
        }
        case '+':
        {
            sem_wait(mutex);
            ring->size++;
            sem_post(mutex);
            break;
        }
        case '-':
        {
            if (sync_method == 0) {
                sem_wait(mutex);
                if (ring->size > 0) {
                    ring->size--;
                    if (ring->cur > ring->size) {
                        pop(&ring->head, &ring->tail);
                        ring->cur--;
                    }
                } else {
                    printf("\nRING IS EMPTY\n");
                }
                sem_post(mutex);
            } else {
                pthread_mutex_lock(&ring_mutex);
                if (ring->size > 0) {
                    ring->size--;
                    if (ring->cur > ring->size) {
                        pop(&ring->head, &ring->tail);
                        ring->cur--;
                    }
                    pthread_cond_broadcast(&not_full);
                } else {
                    printf("\nRING IS EMPTY\n");
                }
                pthread_mutex_unlock(&ring_mutex);
            }
            break;
        }
        case 'r':
        {
            if (producer_threads_count > 0)
            {
                pthread_kill(producer_threads[--producer_threads_count], SIGUSR1);
                pthread_join(producer_threads[producer_threads_count], NULL);
                producer_count--;
                printf("Last producer thread removed\n");
            }
            else
            {
                printf("No producer threads to remove\n");
            }
            break;
        }
        case 'd':
        {
            if (consumer_threads_count > 0)
            {
                pthread_kill(consumer_threads[--consumer_threads_count], SIGUSR1);
                pthread_join(consumer_threads[consumer_threads_count], NULL);
                consumer_count--;
                printf("Last consumer thread removed\n");
            }
            else
            {
                printf("No consumer threads to remove\n");
            }
            break;
        }
        
        case 'q':
        {
            graceful_shutdown(producer_threads, &producer_threads_count,
                consumer_threads, &consumer_threads_count,
                producer, consumer, mutex);
            return 0;
        }
        case 'h':
            printf("\n=== IPC Producer-Consumer Program ===\n");
            printf("Commands:\n");
            printf("  p - Add new producer thread\n");
            printf("  c - Add new consumer thread\n");
            printf("  r - Remove last producer thread\n");
            printf("  d - Remove last consumer thread\n");
            printf("  s - Show statistics\n");
            printf("  + - Increase ring buffer size\n");
            printf("  - - Decrease ring buffer size\n");
            printf("  q - Quit program (graceful shutdown)\n");
            printf("  h - help\n");
            printf("\nCurrent ring size: %ld\n", ring->size);
            break;
        default:
            break;
        }
    }

    if (stop_flag) {
        graceful_shutdown(producer_threads, &producer_threads_count, consumer_threads, &consumer_threads_count, producer, consumer, mutex);
    }
    
    return 0;
}

void handle_sigint(int sig)
{
    stop_flag = 1;
}

void graceful_shutdown(
    pthread_t* producer_threads, size_t* producer_threads_count,
    pthread_t* consumer_threads, size_t* consumer_threads_count,
    sem_t* producer, sem_t* consumer, sem_t* mutex)
{
    while (*producer_threads_count > 0) {
        pthread_kill(producer_threads[--(*producer_threads_count)], SIGUSR1);
        pthread_join(producer_threads[*producer_threads_count], NULL);
    }

    while (*consumer_threads_count > 0) {
        pthread_kill(consumer_threads[--(*consumer_threads_count)], SIGUSR1);
        pthread_join(consumer_threads[*consumer_threads_count], NULL);
    }

    cleanup_sync_primitives();

    ring_clear(ring);
    free(ring);

    printf("\nGraceful shutdown complete.\n");
}

void initialize_sync_primitives() {
    if (sync_method == 0) {
        // Инициализация семафоров
        sem_unlink("/producer");
        sem_unlink("/consumer");
        sem_unlink("/mutex");

        producer = sem_open("/producer", O_CREAT, 0644, 1);
        consumer = sem_open("/consumer", O_CREAT, 0644, 1);
        mutex = sem_open("/mutex", O_CREAT, 0644, 1);
    } else {
        // Инициализация мьютекса и условных переменных
        pthread_mutex_init(&ring_mutex, NULL);
        pthread_cond_init(&not_empty, NULL);
        pthread_cond_init(&not_full, NULL);
    }
}

void cleanup_sync_primitives() {
    if (sync_method == 0) {
        sem_close(producer);
        sem_close(consumer);
        sem_close(mutex);

        sem_unlink("/producer");
        sem_unlink("/consumer");
        sem_unlink("/mutex");
    } else {
        pthread_mutex_destroy(&ring_mutex);
        pthread_cond_destroy(&not_empty);
        pthread_cond_destroy(&not_full);
    }
}