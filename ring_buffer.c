#include "ring_buffer.h"
#include <stdlib.h>
#include <string.h> // remove these 2 includes if linker errors occur
#include <stdio.h>

pthread_mutex_t ring_mutex;
pthread_cond_t ring_not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t ring_not_empty = PTHREAD_COND_INITIALIZER;

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
    pthread_mutex_init(&ring_mutex, NULL);
}

/*
 * Initialize the ring
 * @param r A pointer to the ring
 * @return 0 on success, negative otherwise - this negative value will be
 * printed to output by the client program
*/
int init_ring(struct ring *r){
    r->c_head = 0; 
    r->c_tail = 0; 
    r->p_head = 0; 
    r->p_tail = 0; 

    for(int i = 0; i < RING_SIZE; i++){
        r->buffer[i].req_type = -1; // not set to anything yet
        r->buffer[i].k = 0;
        r->buffer[i].v = 0;
        r->buffer[i].res_off = 0;
        r->buffer[i].ready = 0;
    }

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
    
    pthread_mutex_lock(&ring_mutex);
    // wait for buffer to have open spots
    // while(is_ring_full(r) == 1){
    //     pthread_cond_wait(&ring_not_full, &ring_mutex);
    // }
    
    while(r->p_head + 1 - r->c_tail >= RING_SIZE){
        pthread_cond_wait(&ring_not_full, &ring_mutex);
    }

    int old_p_head = r->p_head;
    int old_c_tail = r->c_tail;
    // get next index and wrap around if too large
    int next_index = (r->p_head + 1) % RING_SIZE;
    r->p_head = next_index;
    // r->buffer[next_index] = *bd; // ref copy lets try deep copy
    memcpy(&r->buffer[old_p_head], &bd, sizeof(struct buffer_descriptor));
    // this should be a copy at head? head holds empty struct initially, we wanna copy to head, incrementing head to next so any other ops see that head = head+1 essentially so no collisions
    // with current logic, first insert happens at idx 1 with 0 being empty
    //r->p_tail = (r->p_tail + 1) % RING_SIZE;
    r->p_tail = r->p_tail + 1; 
    // just incrementing tail here, problem could occur with concurrent requests i believe if we just set the value to old_head

    // signal buffer is not empty 
    pthread_cond_signal(&ring_not_empty);

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

    pthread_mutex_lock(&ring_mutex);
    
    // while(is_ring_empty(r) == 1){
    //     printf("ring_get\n");
    //     pthread_cond_wait(&ring_not_empty, &ring_mutex);
    // }
    while(r->c_head == r->p_tail){
        pthread_cond_wait(&ring_not_empty, &ring_mutex);
    }
    // copy buffer descriptor from ring buffer
    // potentially move this out of lock to improve performance
    int old_c_head = r->c_head;
    int old_p_tail = r->p_tail;
    int c_head_next =(r->c_head + 1) % RING_SIZE;
    r->c_head = c_head_next; //increment head before copy operation
    //*bd = r->buffer[r->c_head];
    memcpy((void*)bd, &r->buffer[old_c_head], sizeof(struct buffer_descriptor));
    
    r->c_tail = r->c_tail + 1;

    // signal that buffer is not full anymore
    pthread_cond_signal(&ring_not_full);
    
    pthread_mutex_unlock(&ring_mutex);
}


