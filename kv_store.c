#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ring_buffer.h"

int put(key_type k, value_type v){
    return -1;
}
/*
 * retrieve value with key "k" from hashtable, returns 0 if it doesn't exist
*/
int get(key_type k) {
    return -1;
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
    
    // need to initialize hashtable given our arguments
    // get and put methods need to support concurrent operations
    // hashtable should look like a array of linked lists. 
    // use hash function to get array idx to insert
    // what about resizing?

    return 0;
}