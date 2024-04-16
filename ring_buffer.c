#include "ring_buffer.h"
#include <stdlib.h>
#include <string.h> // remove these 2 includes if linker errors occur
#include <stdio.h>
#include <stdatomic.h>
#include <unistd.h>

pthread_mutex_t ring_mutex;
pthread_cond_t ring_not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t ring_not_empty = PTHREAD_COND_INITIALIZER;

int CompareAndSwap(int* addr, int expected, int new){
    
}

/**
 * Helper method that checks if ring buffer is full
 * Returns 0 if not full (meaning some space still available)
 * Returns 1 if full
*/
int is_ring_full(struct ring *r){
    for(int i = 0; i < RING_SIZE; i++){
        if(r->buffer[i].v == 0){
            return 0;
        }
    }
    return 1;
}

/**
 * Helper method that checks if ring buffer is empty
 * Returns 0 if not empty (meaning there's something in there)
 * Returns 1 if empty
*/
int is_ring_empty(struct ring *r){
    for(int i = 0; i < RING_SIZE; i++){
        if(r->buffer[i].v != 0){
            return 0;
        }
    }
    return 1;
}
void init_mutex(struct ring* r){
    pthread_mutex_init(&r->mutex, NULL);
    ring_mutex = r->mutex;
}

/*
 * Initialize the ring
 * @param r A pointer to the ring
 * @return 0 on success, negative otherwise - this negative value will be
 * printed to output by the client program
*/
int init_ring(struct ring *r){
    r->c_head = -1; 
    r->c_tail = -1; 
    r->p_head = 0; 
    r->p_tail = 0; 

    for(int i = 0; i < RING_SIZE; i++){
        r->buffer[i].req_type = -1; // not set to anything yet
        r->buffer[i].k = 0;
        r->buffer[i].v = 0;
        r->buffer[i].res_off = 0;
        r->buffer[i].ready = 0;
    }
    init_mutex(r);
    return 0;
}

/*
 * Submit a new item - should be thread-safe
 * This call will block the calling thread if there's not enough space
 * @param r The shared ring
 * @param bd A pointer to a valid buffer_descriptor - This pointer is only
 * guaranteed to be valid during the invocation of the function
*/
void ring_submit(struct ring *r, struct buffer_descriptor *bd){
    // wait for buffer to have open spots
    // while(is_ring_full(r) == 1){
    //     pthread_cond_wait(&ring_not_full, &ring_mutex);
    // }
    

    // ring full? if phead is at ptail? how to block
    pthread_mutex_lock(&ring_mutex);
    int old_p_head = r->p_head;
    int old_c_tail = r->c_tail;
    // get next index and wrap around if too large
    int next_index = (r->p_head + 1) % RING_SIZE;
    
    r->p_head = next_index;
    r->p_tail = next_index;
    while(next_index == r->c_tail){
        pthread_mutex_unlock(&ring_mutex);
        sleep(1);
        pthread_mutex_lock(&ring_mutex);
    }
    // ring is full
    // producer trying to put item where item already exists
    if(r->p_head == r->c_tail){
        //pthread_cond_wait(&ring_not_full, &ring_mutex);
    }
    //r->buffer[next_index] = *bd; // ref copy lets try deep copy

    memcpy(&bd, &r->buffer[old_p_head], sizeof(struct buffer_descriptor));
    // bd->k = r->buffer[old_p_head].k;
    // bd->v = r->buffer[old_p_head].v;
    // bd->ready = r->buffer[old_p_head].ready;
    // bd->req_type = r->buffer[old_p_head].req_type;
    // bd->res_off = r->buffer[old_p_head].res_off;
    printf("%ls", &r->buffer[old_p_head].k);
    // this should be a copy at head? head holds empty struct initially, we wanna copy to head, incrementing head to next so any other ops see that head = head+1 essentially so no collisions
    // with current logic, first insert happens at idx 1 with 0 being empty
    //r->p_tail = (r->p_tail + 1) % RING_SIZE;
    //r->p_tail = r->p_tail + 1 ; 
    // just incrementing tail here, problem could occur with concurrent requests i believe if we just set the value to old_head

    // signal buffer is not empty 
    //pthread_cond_signal(&ring_not_empty);

    pthread_mutex_unlock(&ring_mutex);
}

/*
 * Get an item from the ring - should be thread-safe
 * This call will block the calling thread if the ring is empty
 * @param r A pointer to the shared ring 
 * @param bd pointer to a valid buffer_descriptor to copy the data to
 * Note: This function is not used in the clinet program, so you can change
 * the signature.
*/
void ring_get(struct ring *r, struct buffer_descriptor *bd){
    // while(is_ring_empty(r) == 1){
    //     printf("ring_get\n");
    //     pthread_cond_wait(&ring_not_empty, &ring_mutex);
    // }
     pthread_mutex_lock(&ring_mutex);
    int old_c_head = r->c_head;
    int old_p_tail = r->p_tail;
    int c_head_next = (old_c_head+ 1) % RING_SIZE;
    while(r->c_head == r->p_tail){
        // pthread_cond_wait(&ring_not_empty, &ring_mutex);
        // ppthread yield? this check for full/empty seems like it needs some atomicity
        //usleep(1000);
        pthread_mutex_unlock(&ring_mutex);
        sleep(1);
        pthread_mutex_lock(&ring_mutex);
    } 
    pthread_mutex_unlock(&ring_mutex);
    // copy buffer descriptor from ring buffer
    while (!atomic_compare_exchange_strong(&r->c_head, &old_c_head, old_c_head + 1)){
        old_c_head = r->c_head; 
    }
    // pthread_mutex_lock(&ring_mutex);
    // r->c_head = c_head_next; //increment head before copy operation
    // pthread_mutex_unlock(&ring_mutex);
    //*bd = r->buffer[r->c_head];
    memcpy((void*)bd, &r->buffer[old_c_head], sizeof(struct buffer_descriptor));
    
    r->c_tail = (r->c_tail + 1) % RING_SIZE;

    // signal that buffer is not full anymore
    //pthread_cond_signal(&ring_not_full);
    
    
}


