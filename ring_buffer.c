#include "ring_buffer.h"
#include <stdatomic.h>

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
    pthread_mutex_lock(&r->lk); // instead of lock maybe atomic instructions
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

    prod_head = r->p_head; // 0 1
    cons_tail = r->c_tail; // 0 0
    // if not enough space, block and wait
    if (prod_head >= cons_tail){
        // wait?
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
    // block if not enough space
    if (prod_tail <= cons_head) {
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

int test_get(){
    return 0;
}
int main() {
    return 1;
}