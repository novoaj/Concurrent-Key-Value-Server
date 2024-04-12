#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ring_buffer.h"



typedef struct Node{
    key_type key;
    value_type val;
    struct Node* next;
} Node;

typedef struct hashtable{
    Node** array;
    int size;
}hashtable;

hashtable* serverHashtable;

hashtable* initializeHashtable(int size){
    hashtable* ht = (hashtable*)malloc(sizeof(hashtable));
    if (ht == NULL){
        fprintf(stderr, "Mem allocation for ht failed");
        exit(EXIT_FAILURE);
    }

    ht->array = (Node**)malloc(size*sizeof(Node*));
    if (ht->array == NULL){
        fprintf(stderr, "Mem allocation for ht->array failed");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < size; i++) {
        ht->array[i] = NULL;
    }

    ht->size = size;

    return ht;
}
/*
 * insert or update value at key "k"
*/
int put(key_type k, value_type v){
    int BucketIdx = hash_function(k, serverHashtable->size);
    // acquire lock for this idx
    return -1;
}
/*
 * retrieve value with key "k" from hashtable, returns 0 if it doesn't exist
*/
int get(key_type k) {
    int BucketIdx = hash_function(k, serverHashtable->size);
    // acquire lock for this idx
    return -1;
}
void workerThread(){

}
/*
 * this is our server program. it will use ring_buffer get in order to retrieve requests. 
 "The server should be able to fetch requests from the Ring Buffer, and update the Request-status Board 
  after completing the requests. We expect the server to be faster with an increase in number of threads"
*/
int main(int argc, char* argv[]){
    // args can be with flags -n and -s
    // both flags should be followed by a number - e.g: './server -n 2 -s 5'
    // Here, -n is the number of threads, and -s is the initial hashtable size for the KV Store.
    int numThreads = 0;
    int hashtableSize = 0;

    int opt;
    int opt_idx = 0;

    while ((opt = getopt(argc, argv, "n:s:")) != -1) {
        switch (opt) {
            case 'n':
                numThreads = atoi(optarg);
                break;
            case 's':
                hashtableSize = atoi(optarg);
                break;
            default: 
                printf("error, need -n, and -s flags\n");
                return -1;
        }
    }
    printf("numThreads: %d, htsize: %d\n", numThreads, hashtableSize);
    if (numThreads <= 0 || hashtableSize <= 0){
        printf("-n and -s flags must be provided with a positive integer\n");
        return -1;
    }
    // create threads as specified by args?
    // thread function will run indefinitely processing requests. 
    // try to get something from ring (ring_get) within loop and service this request, 
    // client status board: no structure, just a bunch of memory, we need to use that and do some addition of addrs to figure out how to access this
    // mmap to map shmem into mem? to access it
    serverHashtable = initializeHashtable(hashtableSize);
    // initialize a array same size as hashtable where each idx holds a lock

    return 0;
}