/* NAME: Samuel Shen */
/* EMAIL: sam.shen321@gmail.com */
/* ID: 405325252 */

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include "SortedList.h"

int threads = 1;
int iterations = 1;
int opt_yield = 0;
char opt_sync ='n';
SortedListElement_t* nodelist;
int* threadInds;
SortedList_t list;
pthread_mutex_t lock;
int spin_lock = 0;

char* getSync(){
    if(opt_sync=='n'){
        return "none";
    }else if(opt_sync=='s'){
        return "s";
    }else if(opt_sync=='m'){
        return "m";
    }

    return "Error";
}

char* getYield(){
    if(opt_yield&INSERT_YIELD && opt_yield&DELETE_YIELD && opt_yield&LOOKUP_YIELD){
        return "idl";
    }else if(opt_yield&INSERT_YIELD && opt_yield&DELETE_YIELD){
        return "id";
    }else if(opt_yield&DELETE_YIELD && opt_yield&LOOKUP_YIELD){
        return "dl";
    }else if(opt_yield&INSERT_YIELD && opt_yield&LOOKUP_YIELD){
        return "il";
    }else if(opt_yield&INSERT_YIELD){
        return "i";
    }else if(opt_yield&DELETE_YIELD){
        return "d";
    }else if(opt_yield&LOOKUP_YIELD){
        return "l";
    }

    return "none";

}

void sighandler(){
    fprintf(stderr, "SEGFAULT: --threads=%d --iterations=%d --yield=%s --sync=%s\n", threads, iterations, getYield(), getSync());
    exit(2);
}

void* runThread(void* input){
    int t_index = *((int*)input);
    int startind = t_index*iterations;

    if(opt_sync=='m'){
        pthread_mutex_lock(&lock);
    }else if(opt_sync=='s'){
        //spin until lock is released
        while(__sync_lock_test_and_set(&spin_lock, 1)==1);
    }
    
    for(int i=startind; i<startind+iterations; i++){
        SortedList_insert(&list, &nodelist[i]);
    }
    
    SortedList_length(&list);

    for(int i=startind; i<startind+iterations; i++){
        if(SortedList_delete(&nodelist[i])==1){
            fprintf(stderr, "Delete failed, corrupted pointer in list. --threads=%d --iterations=%d --yield=%s --sync=%s\n", threads, iterations, getYield(), getSync());
            exit(2);
        }
    }

    if(opt_sync=='m'){
        pthread_mutex_unlock(&lock);
    }else if(opt_sync=='s'){
        __sync_lock_release(&spin_lock);
    }

    return NULL;
}

  
int main(int argc, char** argv){
    signal(SIGSEGV, sighandler);
    
    int c;
    while(1){
        static struct option long_options[] = {
            {"threads", required_argument, 0, 't'},
            {"iterations", required_argument, 0, 'i'},
            {"yield", required_argument, 0, 'y'},
            {"sync", required_argument, 0, 's'},
            {0,0,0,0}
        };
        int option_index = 0;
        c = getopt_long(argc, argv, "s:t:i:y:", long_options, &option_index);

        if(c==-1){
            break;
        }

        switch(c){
        case 't':
            threads = atoi(optarg);
            break;
        case 'i':
            iterations = atoi(optarg);
            break;
        case 'y':
            for(size_t i=0; i<strlen(optarg); i++){
                if(optarg[i]=='i'){
                    opt_yield = opt_yield|INSERT_YIELD;
                }else if(optarg[i]=='d'){
                    opt_yield = opt_yield|DELETE_YIELD;
                }else if(optarg[i]=='l'){
                    opt_yield = opt_yield|LOOKUP_YIELD;
                }else{
                    fprintf(stderr, "Invalid argument to --yield. Only idl\n");
                    exit(1);
                }
            }
            break;
        case 's':
            opt_sync = optarg[0];
            break;
        case '?':
            //unrecognized argument                                                        
            fprintf(stderr, "Unrecognized Argument: --threads=# --iterations=#\n");
            exit(1);
        default:
            abort();
        }
    }

    
    if(opt_sync!='n' && opt_sync!='s' && opt_sync!='m'){
        fprintf(stderr, "Unrecognized Argument For Sync, only s or m\n");
        exit(1);
    }

    if(opt_sync=='m'){
        if(pthread_mutex_init(&lock, NULL) != 0){
            fprintf(stderr, "Mutex init failed\n");
            exit(1);
        }
    }

    list.key = NULL;
    list.next = NULL;
    list.prev = NULL;


    int numNodes = threads*iterations;
    nodelist = (SortedListElement_t*)malloc(numNodes*sizeof(SortedListElement_t));
    
    for(int i=0; i<numNodes; i++){
        char* keystring = (char*)malloc(5*sizeof(char));
        for(int j=0; j<5; j++){
            keystring[j] = 'a'+rand()%26;
        }
        nodelist[i].key = keystring;
    }

    //start timer
    struct timespec starttime;
    struct timespec endtime;
    if(clock_gettime(CLOCK_MONOTONIC, &starttime)==-1){
        fprintf(stderr, "Error with getting start time: %s\n", strerror(errno));
        exit(1);
    }

    //start threads
    pthread_t* tid = (pthread_t*)malloc(threads*sizeof(pthread_t));
    threadInds = (int*)malloc(threads*sizeof(int));
    
    //create threads and mark indicies

    for(int i=0; i<threads; i++){
        threadInds[i] = i;
        if(pthread_create(&(tid[i]), NULL, &runThread, (void*)&threadInds[i])){
            fprintf(stderr, "Error creating thread : [%s]\n", strerror(errno));
            exit(1);
        }
    }


    //join threads
    for(int i=0; i<threads; i++){
        if(pthread_join(tid[i], NULL)!=0){
            fprintf(stderr, "Error joining threads\n");
            exit(1);
        }
    }

    //get end time
    if(clock_gettime(CLOCK_MONOTONIC, &endtime)==-1){
        fprintf(stderr, "Error with getting end time: %s\n", strerror(errno));
        exit(1);
    }

    //destroy mutex lock
    if(opt_sync=='m'){
        if(pthread_mutex_destroy(&lock) != 0){
            fprintf(stderr, "Error destroying lock\n");
            exit(1);
        }
    }

    //free all memory
    free(threadInds);
    free(tid);
    for(int i=0; i<numNodes; i++){
        free((char*)nodelist[i].key);
    }
    free(nodelist);

    int final_length = SortedList_length(&list);
    if(final_length!=0){
        fprintf(stderr, "Error: final list length is not 0 --threads=%d --iterations=%d --yield=%s --sync=%s\n", threads, iterations, getYield(), getSync());
        exit(2);
    }

    long billion = 1000000000;
    long total_time = billion*(endtime.tv_sec - starttime.tv_sec) + endtime.tv_nsec-starttime.tv_nsec;

    long operations = threads*iterations*3;
    long avg_time = total_time/operations;

    printf("list-%s-%s,%d,%d,1,%ld,%ld,%ld\n", getYield(), getSync(), threads, iterations, operations, total_time, avg_time);
}
