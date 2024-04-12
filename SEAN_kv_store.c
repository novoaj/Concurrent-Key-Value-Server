#include "ring_buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// global variables
int hashtable_size;
int server_threads;

// struct of each bucket in the hashtable
// each bucket has a value and a mutex
// MIGHT WANT TO CHANGE THIS FROM VALUE_TYPE TO A LIST OF VALUE TYPES
typedef struct {
    value_type value[1024];
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

value_type get(key_type k){
    index_t hash_index = hash_function(k, hashtable_size);
    value_type v;
    
    pthread_mutex_lock(&hashtable[hash_index].mutex);
    for(int i = 0; i < 1024; i++){
        if(hashtable[hash_index].value[i])
    }
    if(!hashtable[hash_index].value){
        v = hashtable[hash_index].value;
    }
    else{
        v = 0;
    }
    pthread_mutex_unlock(&hashtable[hash_index].mutex);
    return v;
}


void put(key_type k, value_type v){
    
    index_t hash_index = hash_function(k, hashtable_size);
    pthread_mutex_lock(&hashtable[hash_index].mutex);
    if(get(k) != 0){
        hash_index = get(k);
    }
    hashtable[hash_index].value = v; // put value in hash table
    pthread_mutex_unlock(&hashtable[hash_index].mutex);
}


int main(int argc, char *argv[]){
    if (argc != 5) {
        printf("Usage: %s -n <num_threads> -s <initial_hashtable_size>\n", argv[0]);
        return -1;
    }

    int init_server_threads = -1;
    int init_hashtable_size = -1;

    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-n") == 0) {
            init_server_threads = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "-s") == 0) {
            init_hashtable_size = atoi(argv[i + 1]);
        } else {
            printf("Invalid argument: %s\n", argv[i]);
            return -1;
        }
    }

    if(init_server_threads == -1 || hashtable_size == -1){
        return -1;
    }

    hashtable_size = init_hashtable_size;
    server_threads = init_server_threads;
    return 0;
}
