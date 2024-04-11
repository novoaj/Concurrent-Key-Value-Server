#include "ring_buffer.h"
#include <stdatomic.h>
#include <stdio.h>

/*
 * Initialize the ring
 * @param r A pointer to the ring
 * @return 0 on success, negative otherwise - this negative value will be
 * printed to output by the client program
*/
int init_ring(struct ring *r){
    pthread_mutex_init(&r->lk, NULL); // Initialize the mutex lock
    pthread_mutex_lock(&r->lk); // instead of lock maybe atomic instructions
    r->p_head = 0; // all start at 0
    r->p_tail = 0; 
    r->c_head = 0;
    r->c_tail = 0;
    // r->buffer // this buffer -> array of structs
    pthread_mutex_unlock(&r->lk);
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
    // atomically increment p_head https://doc.dpdk.org/guides/prog_guide/ring_lib.html 
    uint32_t prod_head, prod_next, cons_tail;

    prod_head = r->p_head; 
    cons_tail = r->c_tail; 
    prod_next = prod_head + 1;
    // if not enough space, block and wait
    // ?ph - ct >= ring_size //
    if (prod_next == cons_tail){
        // wait until an item is consumed? so wait until cons_tail changes?
        // if p_head + 1 overlaps with consumer tail, we cant insert because there isn't enough space
    }
    // atomically increment producer head
    while (!atomic_compare_exchange_strong(&r->p_head, &prod_head, prod_head + 1)){
        prod_head = r->p_head;
    }

    // copy data
    r->buffer[prod_head] = *bd; // copy item allowing for concurrent copies (atomic operation is moving the head pointer)
    // increment tail once copy is complete
    r->p_tail = r->p_tail + 1; // does incrementing tail need to be atomic?
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
    // save prod tail and cons head in vars
    // if prod tail > cons_head, move cons head forward one. so we are currently reserving that idx
    // atomically xchg prod head for get (increment it)

    uint32_t cons_head, cons_next, prod_tail;
    cons_head = r->c_head;
    //cons_next = (r->c_head + 1) % RING_SIZE;
    prod_tail = r->p_tail;
    // block if no valid items to consume
    if (cons_head == prod_tail) { // means that cons_next isn't valid for consuming yet? 
        // wait?
    }
    // original, expected, new
    // https://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html
    // https://stackoverflow.com/questions/26463074/is-there-a-difference-between-the-atomic-type-qualifier-and-type-specifier 
    while (!atomic_compare_exchange_strong(&r->c_head, &cons_head, cons_head + 1)){
        cons_head = r->c_head;
    }

    *bd = r->buffer[cons_head]; // consume item, maybe memcpy or deep copy? this is reference copy which could be fine?
    // increment tail once after copy is complete
    r->c_tail = r->c_tail + 1;
}

// int print_ring(struct ring *r){
//     for (int i = 0; i < 1024; i++){
//         printf("%d: %d\n", r->buffer[i].k, r->buffer[i].v);
//     }
//     return 0;
// }