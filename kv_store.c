#include "ring_buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// global variables
int hashtable_size;
int server_threads;

// struct of each bucket in the hashtable
// each bucket has a value and a mutex
// MIGHT WANT TO CHANGE THIS FROM VALUE_TYPE TO A LIST OF VALUE TYPES
typedef struct {
    value_type *value;
    key_type *keys;
    int capacity;
    int count;
    pthread_mutex_t mutex; 
} bucket_t;


bucket_t *hashtable;
FILE *error_log;

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// void openErrorLog(){
//     error_log = fopen("logfile.txt", "w");
//     if (error_log == NULL) {
//         perror("Error opening log file");
//         // Handle error
//     }
    
// }
// void closeErrorLog(){
//     fclose(error_log);
// }
// void logMessage(char* message){
//     pthread_mutex_lock(&log_mutex);

//     f////printf(error_log, "%s\n", message);
//     fflush(error_log); //f////printf output is buffered. this "unbuffers" it (forces buffered output to write to disk, worse for performance but guarentees we see print in our logfile)

//     pthread_mutex_unlock(&log_mutex);
// }

void initHashtable(int size){
    hashtable = (bucket_t *)malloc(size * sizeof(bucket_t));
    if (hashtable == NULL){
        ////printf("malloc failed for hashtable\n");
    }
    for(int i = 0; i < size; i++){
        // DON"T MALLOC KEYS+VALS FOR EVERY BUCKET JUST BUCKETS WE NEED
        pthread_mutex_init(&hashtable[i].mutex, NULL);
        hashtable[i].count = 0;
        hashtable[i].capacity = 1024;
        hashtable[i].value = NULL;
        hashtable[i].keys = NULL;
    }
    ////printf("exiting hashtable\n");
}
// returns idx that value lives at (this is for the case when key already exists in this bucket)
// return -1 if no value found
int get_value_idx(bucket_t* bucket, key_type k){
    ////printf("entering get_value_idx\n");
    for (int i = 0; i < bucket->count; i++){
        if (bucket->keys[i] == k){
            ////printf("index found, returning from get_value_idx\n");
            return i;
        }
    }
    ////printf("leaving get_value_idx\n");
    return -1;
    
}

value_type get(key_type k){
    index_t hash_index = hash_function(k, hashtable_size);
    value_type v = 0;
    
    //printf("GET acquiring the lock\n");
    pthread_mutex_lock(&hashtable[hash_index].mutex);

    // potentially count to capacity
    for(int i = 0; i < hashtable[hash_index].count; i++){
        if(hashtable[hash_index].keys[i] == k){
            v = hashtable[hash_index].value[i];
            break;
        }
    }
    pthread_mutex_unlock(&hashtable[hash_index].mutex);
    //printf("GET releasing the lock\n");
    return v;
}

void resize_bucket (bucket_t* bucket){
    int new_capacity = bucket->capacity * 2;
    value_type *new_values = (value_type *)realloc(bucket->value, new_capacity * sizeof(value_type));
    key_type *new_keys = (key_type *)realloc(bucket->keys, new_capacity * sizeof(key_type));
    if (new_values && new_keys) {
        bucket->value = new_values;
        bucket->keys = new_keys;
        bucket->capacity = new_capacity;

    } else {
        // Handle memory allocation failure
        // For simplicity, you can just print an error message
        ////printf("Memory reallocation failed\n");
    }
}


void put(key_type k, value_type v){
    ////printf("Put k = %d v = %d\n", k, v);
    //printf("entering put\n");
    index_t hash_index = hash_function(k, hashtable_size);
    ////printf("hash index: %d\n", hash_index);
    ////printf("bucket capacity = %d bucket count = %d\n", hashtable[hash_index].capacity, hashtable[hash_index].count);
    
    //printf("PUT acquiring the lock\n");
    pthread_mutex_lock(&hashtable[hash_index].mutex);
    // if empty bucket: malloc
    if (hashtable[hash_index].value == NULL){
        hashtable[hash_index].value = (value_type *)malloc(hashtable[hash_index].capacity * sizeof(value_type));
        if (hashtable[hash_index].value == NULL){
            ////printf("malloc failed for value array\n");
        }
        hashtable[hash_index].keys = (key_type *)malloc(hashtable[hash_index].capacity * sizeof(key_type));
        if (hashtable[hash_index].keys == NULL){
            ////printf("malloc failed for keys array\n");
        }
        if (hashtable[hash_index].keys && hashtable[hash_index].value){
            for (int j = 0; j < hashtable[hash_index].capacity; j++){
                hashtable[hash_index].value[j] = 0;
                hashtable[hash_index].keys[j] = 0;
            }
        }else{
            ////printf("mem alloc failure\n");
        }
    }
    //int valueIdx = get_value_idx(&hashtable[hash_index], k);
    // ////printf("index of key = %d\n", valueIdx);
    // new item: insert value and key
    // if (valueIdx == -1){
        //////printf("inside the valueIdx if statement\n");
        // naive insert
        // could cut down on time here
        ////printf("start of for loop capacity = %d and count = %d\n", hashtable[hash_index].capacity, hashtable[hash_index].count);
        ////printf(" hashtable[hash_index] %d",  hashtable[hash_index].keys[hash_index]);
        
        for (int i = 0; i < hashtable[hash_index].capacity; i++){
            //printf("insloop\n");
            //////printf("hashtable[hash_index].value[i] = %d\n", hashtable[hash_index].value[i]);
            //printf("Key being checked = %d\n", hashtable[hash_index].keys[i]);
            if (hashtable[hash_index].keys[i] == 0) {
                hashtable[hash_index].keys[i] = k;
                hashtable[hash_index].value[i] = v;
                hashtable[hash_index].count++;
                if(hashtable[hash_index].count == hashtable[hash_index].capacity){
                    //printf("resizing in put\n");
                    resize_bucket(&hashtable[hash_index]);
                }
                
                break;
            }
            else if(hashtable[hash_index].keys[i] == k){
                hashtable[hash_index].value[i] = v;
                break;
            }
            
        }
        pthread_mutex_unlock(&hashtable[hash_index].mutex);
        ////printf("left the for loop\n");
        
    // }else{ // item exists (key exists): update value
    //     hashtable[hash_index].value[valueIdx] = v;
        
    // }
    
    
    //printf("PUT releasing the lock\n");
}

// CHANGE changed to void* in order to fit pthread_create
// may require more work
void* workerThread(void* r) {
    // worker thread should run indefinitely, processing requests from ring.
    // this function will call get 
    
    // ////printf("spawned worker thread\n");
    
    // need to mutate shmem upon processing requests
    struct ring* ring_buffer = (struct ring*) r;
    struct buffer_descriptor* buf;
    
    buf = malloc(sizeof(struct buffer_descriptor));
    
    while(1){
        char s[64];
        //printf("ring before get: phead %d\tptail %d\tchead %d\tctail %d\n",
                 //ring_buffer->p_head, ring_buffer->p_tail, ring_buffer->c_head, ring_buffer->c_tail);
        ring_get(ring_buffer, buf); // hanging here, not able to process requests
        // struct buffer_descriptor *result = &ring_buffer->buffer[buf->res_off];
        // client only submits "w" requests before waiting to hear back from server?

        //printf("ring info after ring_get: phead %d\tptail %d\tchead %d\tctail %d\n",
               // ring_buffer->p_head, ring_buffer->p_tail, ring_buffer->c_head, ring_buffer->c_tail);
        if(buf->req_type == PUT){
            //printf("\nPUT request...\n");
            put(buf->k, buf->v);
        }
        else if(buf->req_type == GET){
            //printf("\nGET request...\n");
            buf->v = get(buf->k);
            // may need to handle fail case // TODO get return value needs to go somewhere - needs to be copied to shmem?
        }
        
        struct buffer_descriptor* window = (struct buffer_descriptor*) ((char*) ring_buffer + buf->res_off); // do we dereference ring_buffer when adding
        // ////printf("window calc without &: %p\n", ring_buffer + buf->res_off);
        // ////printf("window calc with &: %p\n", &ring_buffer + buf->res_off);
        // ////printf("Addr of ring buffer: %p\n", (void*)ring_buffer);
        // ////printf("buf->res_off %d\n", buf->res_off);
        // us,
        // ////printf("memcpying to shmem...\n");
        ////printf("buf contents: buf->k %d, buf->v %d, buf->ready %d, buf->req_type %d\n", buf->k, buf->v, buf->ready, buf->req_type);
        memcpy(window, buf, sizeof(struct buffer_descriptor));
        window->ready = 1;
        // ////printf("window ptr: %p\n", (void*) window);
        ////printf("window contents: window->k %d, window->v %d, window->ready %d, window->req_type %d\n", window->k, window->v, window->ready, window->req_type);
        //printf("memcpy complete, request processed\n\n");
        
    }
    
    free(buf);
    return NULL;
}

int main(int argc, char *argv[]){
    //printf("main\n");
    char s[64];
    
    int num_threads = -1;
    int init_hashtable_size = -1;
    struct ring* ring1;

    int opt;
    int opt_idx = 0;

    while ((opt = getopt(argc, argv, "n:s:")) != -1) {
        switch (opt) {
            case 'n':
                num_threads = atoi(optarg);
                break;
            case 's':
                init_hashtable_size = atoi(optarg);
                break;
            default: 
                ////printf("error, need -n, and -s flags\n");
                return -1;
        }
    }

    if(num_threads == -1 || init_hashtable_size == -1){
        return -1;
    }
    //printf("init hasthable...\n");
    initHashtable(init_hashtable_size);
    hashtable_size = init_hashtable_size;
    
    // TODO: spawn threads that will be infinitely looping and calling ring_get - based on bd filled in from ring_get, we call put or get
    char* shmem_file = "shmem_file";
    // stat to get size of file
    // // map shmem file to memory so we can access it
    
    int fd = open(shmem_file, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    
	if (fd < 0)
	    perror("open");
    // // map file
    struct stat buf;
    fstat(fd, &buf);
    int shm_size = buf.st_size; // https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
    char *mem = mmap(NULL, shm_size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
	if (mem == (void *)-1) 
	    perror("mmap");
    // start of mem + res_off
	close(fd);
    struct ring* r = (struct ring *) mem; // reference to our ring struct in shared mem
        // create the number of threads necessary
    pthread_t threads[num_threads];
    for(int i = 0; i < num_threads; i++){
        if (pthread_create(&threads[i], NULL, workerThread, (void *)r) != 0) { 
        // ^ ring1 declared but never initialized - change param to r and AFTER initiliazation of r from mmapped file
            ////printf("Error creating thread %d\n", i);
            return -1;
        }

    }
    // does this run before worker threads terminate?
    for(int i = 0; i < num_threads; i++){
        if(pthread_join(threads[i], NULL) != 0){
            ////printf("Error joining threads %d\n", i);
            return -1;
        }
    }
    return 0;
}
