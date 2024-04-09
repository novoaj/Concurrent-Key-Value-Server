#include "ring_buffer.h"

/*
 * Initialize the ring
 * @param r A pointer to the ring
 * @return 0 on success, negative otherwise - this negative value will be
 * printed to output by the client program
*/
int init_ring(struct ring *r){
    // init ring buffer
    // need to allow for concurrent accesses of buffer
    // buffer max size is 1024, defined in .h file
    // mem already allocated?
    // ring is simply an array
    pthread_mutex_init(&r->lk, NULL); // Initialize the mutex lock
    pthread_mutex_lock(&r->lk);
    r->p_head = 0; // consider 0 invalid
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

    pthread_mutex_lock(&r->lk);
    prod_head = r->p_head; // 0 1
    prod_next = (r->p_head + 1) % RING_SIZE; // 1 2
    cons_tail = r->c_tail; // 0 0
    // if not enough space, block and wait
    // while (cons_tail == prod_next){
    //     ;// put to sleep/wait/block, need to signal when there is space?
    // }
    // increment p_head // 1 2
    r->p_head = prod_next; // ring structs p_head points to next ("prod_head" block is reserved for copy operation)
    pthread_mutex_unlock(&r->lk);
    // bd copied to 0, bd copied to 1
    r->buffer[prod_head] = *bd; // copy item allowing for concurrent copies (atomic operation is moving the head pointer)
    // tail points to 0, 1
    r->p_tail = prod_next; // does incrementing tail need to be atomic?
    // consider two concurrent submits - what if first submit gets copied over before second?
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
    // copy c_head to bd?
    uint32_t cons_head, cons_next, prod_tail;

    pthread_mutex_lock(&r->lk);
    cons_head = r->c_head;
    cons_next = (r->c_head + 1) % RING_SIZE;
    prod_tail = r->p_tail;
    // are there any items to consume?
    // p_tail marks a valid item to consume, idxs between p_tail and p_head are not yet ready to consume
    // from cons_head to prod_tail is valid items to consume?
    // while (cons_next > prod_tail) {
        
    // }
    r->c_head = cons_next; // increment consumer head
    pthread_mutex_unlock(&r->lk);

    *bd = r->buffer[cons_head]; // consume item
    r->c_tail = cons_next;
}