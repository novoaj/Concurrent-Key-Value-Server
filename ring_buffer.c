#include "ring_buffer.h"
#include <stdlib.h>
#include <string.h> // remove these 2 includes if linker errors occur
#include <stdio.h>
#include <stdatomic.h>
#include <unistd.h>

pthread_mutex_t submit_ring_mutex;
pthread_mutex_t get_ring_mutex;
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
void init_mutex(){
    pthread_mutex_init(&submit_ring_mutex, NULL);
    pthread_mutex_init(&get_ring_mutex, NULL);
}

/*
 * Initialize the ring
 * @param r A pointer to the ring
 * @return 0 on success, negative otherwise - this negative value will be
 * printed to output by the client program
*/
int init_ring(struct ring *r){
    // r->c_head = 0; 
    // r->c_tail = 0; 
    // r->p_head = 0; 
    // r->p_tail = 0; 

    // for(int i = 0; i < RING_SIZE; i++){
    //     r->buffer[i].req_type = -1; // not set to anything yet
    //     r->buffer[i].k = 0;
    //     r->buffer[i].v = 0;
    //     r->buffer[i].res_off = 0;
    //     r->buffer[i].ready = 0;
    // }
    init_mutex();
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
    printf("\nring_submit\n");
    printf("phead: %d, ptail: %d, chead: %d, ctail: %d\n", r->p_head, r->p_tail, r->c_head, r->c_tail);
    while(r->p_head - r->c_tail >= RING_SIZE){
        printf("spinwaiting, ring is full\n");
    }

    pthread_mutex_lock(&submit_ring_mutex);
    printf("incrementing p_head\n");
    int old_p_head = r->p_head;
    int old_c_tail = r->c_tail;
    // get next index and wrap around if too large
    int next_index = (r->p_head + 1);
    r->p_head = next_index;
    pthread_mutex_unlock(&submit_ring_mutex);

    printf("copying - %d, %d \n", bd->k, bd->v);
    memcpy(&r->buffer[old_p_head % RING_SIZE], (void*)bd, sizeof(struct buffer_descriptor));
   
    pthread_mutex_lock(&submit_ring_mutex);
    printf("incrementing p_tail\n");
    r->p_tail = r->p_tail + 1;
    pthread_mutex_unlock(&submit_ring_mutex);
    printf("%ls", &r->buffer[old_p_head].k);
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
    printf("\nring_get\n");
    printf("phead: %d, ptail: %d, chead: %d, ctail: %d\n", r->p_head, r->p_tail, r->c_head, r->c_tail);
    while(r->c_head >= r->p_tail){
        // printf("spinwaiting, ring is empty\n");
        // printf("1");
    }

    
    pthread_mutex_lock(&get_ring_mutex);
    printf("incrementing c_head\n");
    int old_c_head = r->c_head;
    int old_p_tail = r->p_tail;
    int c_head_next =(r->c_head + 1);
    r->c_head = c_head_next; //increment head before copy operation
    pthread_mutex_unlock(&get_ring_mutex);

    memcpy((void*)bd, &r->buffer[old_c_head % RING_SIZE], sizeof(struct buffer_descriptor));

    pthread_mutex_lock(&get_ring_mutex);
    printf("incrementing c_tail\n");
    r->c_tail = r->c_tail + 1;
    pthread_mutex_unlock(&get_ring_mutex);
    
    
}


