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
    value_type value[1024];
    key_type keys[1024];
    pthread_mutex_t mutex; 
} bucket_t;


bucket_t *hashtable;

void initHashtable(int size){
    hashtable = (bucket_t *)malloc(size * sizeof(bucket_t));
    hashtable_size = size;
    for(int i = 0; i < size; i++){
        pthread_mutex_init(&hashtable[i].mutex, NULL);
        hashtable->value[i] = 0;
    }
}
// returns idx that value lives at (this is for the case when key already exists in this bucket)
// return -1 if no value found
int get_value_idx(bucket_t* bucket, key_type k){
    for (int i = 0; i < 1024; i++){
        if (bucket->keys[i] == k){
            return i;
        }
    }
    return -1;
}

value_type get(key_type k){
    index_t hash_index = hash_function(k, hashtable_size);
    value_type v = 0;
    
    pthread_mutex_lock(&hashtable[hash_index].mutex);
    for(int i = 0; i < 1024; i++){
        if(hashtable[hash_index].keys[i] == k){
            v = hashtable[hash_index].value[i];
        }
    }
    pthread_mutex_unlock(&hashtable[hash_index].mutex);
    return v;
}


void put(key_type k, value_type v){
    
    index_t hash_index = hash_function(k, hashtable_size);
    pthread_mutex_lock(&hashtable[hash_index].mutex);
   
    int valueIdx = get_value_idx(&hashtable[hash_index], k);
     
    // new item: insert value and key
    if (valueIdx == -1){
        // naive insert
        for (int i = 0; i < 1024; i++){
            if (hashtable[hash_index].value[i] == 0) {
                hashtable[hash_index].value[i] = v;
                break;
            }
        }
    }else{ // item exists (key exists): update value
        hashtable[hash_index].value[valueIdx] = v;
    }
    pthread_mutex_unlock(&hashtable[hash_index].mutex);
}

// CHANGE changed to void* in order to fit pthread_create
// may require more work
void* workerThread(void* r) {
    // worker thread should run indefinitely, processing requests from ring.
    // this function will call put and get 
    // need to mutate shmem upon processing requests
    struct ring* ring_buffer = (struct ring*) r;
    struct buffer_descriptor* buf;

    buf = malloc(sizeof(struct buffer_descriptor));
    while(1){
        ring_get(ring_buffer, buf);
        struct buffer_descriptor *result = &ring_buffer->buffer[buf->res_off];
        memcpy(result, buf, sizeof(struct buffer_descriptor));
        result->ready = 1;
        if(buf->req_type == PUT){
            put(buf->k, buf->v);
        }
        else if(buf->req_type == GET){
            get(buf->k);
            // may need to handle fail case
        }
        
        
    }
    free(buf);
    return NULL;
}

int main(int argc, char *argv[]){
    if (argc != 5) {
        printf("Usage: %s -n <num_threads> -s <initial_hashtable_size>\n", argv[0]);
        return -1;
    }

    int num_threads = -1;
    int init_hashtable_size = -1;
    struct ring* ring1;

    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-n") == 0) {
            num_threads = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "-s") == 0) {
            init_hashtable_size = atoi(argv[i + 1]);
        } else {
            printf("Invalid argument: %s\n", argv[i]);
            return -1;
        }
    }

    if(num_threads == -1 || init_hashtable_size == -1){
        return -1;
    }
    initHashtable(init_hashtable_size);

    // create the number of threads necessary
    pthread_t threads[num_threads];

    for(int i = 0; i < num_threads; i++){
        if (pthread_create(&threads[i], NULL, &workerThread, (void *)ring1) != 0) {
            printf("Error creating thread %d\n", i);
            return -1;
        }
    }

    for(int i = 0; i < num_threads; i++){
        if(pthread_join(threads[i], NULL) != 0){
            return -1;
        }
    }
    
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
    struct ring* r = (struct ring *) mem; // refernece  to our ring struct in shared mem
    return 0;
}
