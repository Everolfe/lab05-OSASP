#include "ring.h"

void push(node_t** head, node_t** tail) 
{
    if (*head != NULL) 
    {
        node_t *temp = (node_t*)malloc(sizeof(node_t));
        temp->message = (mes_t*)malloc(sizeof(mes_t));
        init_mes(temp->message);
        temp->next = *head;
        temp->prev = *tail;
        (*tail)->next = temp;
        (*head)->prev = temp;
        *tail = temp;
    } 
    else 
    {
        *head = (node_t*)malloc(sizeof(node_t));
        (*head)->message = (mes_t*)malloc(sizeof(mes_t));
        init_mes((*head)->message);
        (*head)->prev = *head;
        (*head)->next = *head;
        *tail = *head;
    }
}

void pop(node_t** head, node_t** tail) 
{
    if (*head != NULL) 
    {
        if (*head != *tail) 
        {
            node_t* temp = *head;
            (*tail)->next = (*head)->next;
            *head = (*head)->next;
            (*head)->prev = *tail;

            // Освобождаем память для данных сообщения и самого сообщения
            if (temp->message != NULL) {
                free(temp->message->data);  // Освобождаем данные
                free(temp->message);        // Освобождаем само сообщение
            }

            free(temp);  // Освобождаем сам узел
        } 
        else 
        {
            // В случае одного элемента в кольцевом буфере
            if (*head != NULL && (*head)->message != NULL) {
                free((*head)->message->data);  // Освобождаем данные
                free((*head)->message);        // Освобождаем само сообщение
            }
            free(*head);  // Освобождаем сам узел
            *head = NULL;
            *tail = NULL;
        }
    }
}


void init_mes(mes_t* message) 
{
    const char letters[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

    message->type = 0;
    message->hash = 0;
    message->size = rand() % 257;
    message->data = (uint8_t*)malloc(message->size * sizeof(uint8_t));
    
    for (size_t i = 0; i < message->size; i++) 
    {
        message->data[i] = letters[rand() % 53];
        message->hash += message->data[i];
    }
    message->hash = ~message->hash;
}

void print_mes(mes_t* mes) 
{
    if(mes!=NULL){
        printf("Message type: %d, hash: %d, size: %d, data: ", mes->type, mes->hash, mes->size);
        for(size_t i = 0; i<mes->size; i++)
            printf("%c", mes->data[i]);
        printf("\n");
    }
    else{
        printf("Message is NULL\n");
    }
}

void ring_clear(ring_t* ring) {
    if (!ring) return;

    // Если кольцо пустое, сразу выходим
    if (ring->head == NULL) {
        return;
    }

    // Итерируем по кольцу и освобождаем все узлы
    node_t* current = ring->head;
    do {
        node_t* temp = current;
        current = current->next;

        // Освобождаем память для сообщения
        if (temp->message) {
            mes_clear(temp->message);
            free(temp->message);
        }

        // Освобождаем память для узла
        free(temp);
    } while (current != ring->head); // Пока не вернулись в начало кольца

    // После очистки обнуляем все поля кольца
    ring->head = NULL;
    ring->tail = NULL;
    ring->added = 0;
    ring->deleted = 0;
    ring->cur = 0;
    ring->size = 0;  // Размер кольца можно сбросить, если нужно
}


// Очистка отдельного сообщения
void mes_clear(mes_t* msg) {
    if (!msg) return;
    
    if (msg->data) {
        free(msg->data);
        msg->data = NULL;
    }
    
    msg->type = 0;
    msg->hash = 0;
    msg->size = 0;
}