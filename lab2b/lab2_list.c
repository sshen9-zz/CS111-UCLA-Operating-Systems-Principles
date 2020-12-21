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
int num_lists = 1;
SortedListElement_t* nodelist;
int* threadInds;

SortedList_t* lists;
pthread_mutex_t* mutexlocks;
int* spinlocks;
//pthread_mutex_t lock;
//int spin_lock = 0;

long* mutex_thread_wait_times;

void getLockWaitTime(int t_index, int listindex){
    struct timespec lock_starttime;
    struct timespec lock_endtime;
    //get start time                                                                                                                              
    if(clock_gettime(CLOCK_MONOTONIC, &lock_starttime)==-1){
        fprintf(stderr, "Error with getting start time: %s\n", strerror(errno));
        exit(1);
    }

    if(opt_sync=='m'){
        pthread_mutex_lock(&mutexlocks[listindex]);
    }else if(opt_sync=='s'){
        while(__sync_lock_test_and_set(&spinlocks[listindex], 1)==1);
    }


    //get end time                                                                                                                                
    if(clock_gettime(CLOCK_MONOTONIC, &lock_endtime)==-1){
        fprintf(stderr, "Error with getting end time: %s\n", strerror(errno));
        exit(1);
    }

    long billion = 1000000000;
    long total_time = billion*(lock_endtime.tv_sec - lock_starttime.tv_sec) + lock_endtime.tv_nsec-lock_starttime.tv_nsec;
    //    printf("total time: %ld\n", total_time);
    
    mutex_thread_wait_times[t_index]+=total_time;

}

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
    fprintf(stderr, "SEGFAULT\n");
    //    fprintf(stderr, "SEGFAULT: --threads=%d --iterations=%d --yield=%s --sync=%s\n", threads, iterations, getYield(), getSync());
    exit(2);
}

void* runThread(void* input){
    int t_index = *((int*)input);
    int startind = t_index*iterations;

    //INSERT FOR EACH NODE
    for(int i=startind; i<startind+iterations; i++){
        //hash key to identify which list
        int listindex = nodelist[i].key[0]%num_lists;
       
        if(opt_sync!='n'){
            getLockWaitTime(t_index, listindex);
        }
        
        SortedList_insert(&lists[listindex], &nodelist[i]);

        if(opt_sync=='m'){            
            pthread_mutex_unlock(&mutexlocks[listindex]);
        }else if(opt_sync=='s'){
            __sync_lock_release(&spinlocks[listindex]);
        }

    }

    if(opt_sync=='m'){
        int length = 0;
        for(int i=0; i<num_lists; i++){
            getLockWaitTime(t_index, i);
            length+=SortedList_length(&lists[i]);
            pthread_mutex_unlock(&mutexlocks[i]);
        }
    }else if(opt_sync=='s'){
        int length = 0;
        for(int i=0; i<num_lists; i++){
            getLockWaitTime(t_index, i);
            length+=SortedList_length(&lists[i]);
            __sync_lock_release(&spinlocks[i]);
        }
    }

    /* //GET LENGTH */
    /* if(opt_sync=='m'){ */
    /*     getMutexWaitTime(t_index); */
        
    /* }else if(opt_sync=='s'){ */
    /*     //spin until lock is released */
    /*     while(__sync_lock_test_and_set(&spin_lock, 1)==1); */
    /* }     */
    
    /* SortedList_length(&list); */

    /* if(opt_sync=='m'){ */
    /*     pthread_mutex_unlock(&lock); */
    /* }else if(opt_sync=='s'){ */
    /*     __sync_lock_release(&spin_lock); */
    /* } */


    //LOOKUP AND DELETE FOR EACH NODE
    for(int i=startind; i<startind+iterations; i++){
        int listindex = nodelist[i].key[0]%num_lists;
            
        if(opt_sync!='n'){
            getLockWaitTime(t_index, listindex);
            
        }

        SortedListElement_t* foundelement = SortedList_lookup(&lists[listindex], nodelist[i].key);
        
        if(foundelement==NULL){
            fprintf(stderr, "Lookup failed\n");
            exit(2);
        }

        
        if(SortedList_delete(foundelement)==1){
            fprintf(stderr, "delete failed\n");
            //fprintf(stderr, "Delete failed, corrupted pointer in list. --threads=%d --iterations=%d --yield=%s --sync=%s\n", threads, iterations, getYield(), getSync());
            exit(2);
        }

        if(opt_sync=='m'){
            pthread_mutex_unlock(&mutexlocks[listindex]);
        }else if(opt_sync=='s'){
            __sync_lock_release(&spinlocks[listindex]);
        }

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
            {"lists", required_argument, 0, 'l'},
            {0,0,0,0}
        };
        int option_index = 0;
        c = getopt_long(argc, argv, "s:t:i:y:l:", long_options, &option_index);

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
        case 'l':
            num_lists = atoi(optarg);
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
        mutexlocks = (pthread_mutex_t*)malloc(num_lists*sizeof(pthread_mutex_t));
        for(int i=0; i<num_lists; i++){
            if(pthread_mutex_init(&mutexlocks[i], NULL) != 0){
                fprintf(stderr, "Mutex init failed\n");
                exit(1);
            }
        }
    }else if(opt_sync=='s'){
        spinlocks = (int*)malloc(num_lists*sizeof(int));
        for(int i=0; i<num_lists; i++){
            spinlocks[i] = 0;
        }
    }

    //CREATE LIST HEAD ARRAY
    lists = (SortedList_t*)malloc(num_lists*sizeof(SortedList_t));
    for(int i=0; i<num_lists; i++){
        lists[i].key = NULL;
        lists[i].next = NULL;
        lists[i].prev = NULL;
    }


    int numNodes = threads*iterations;
    nodelist = (SortedListElement_t*)malloc(numNodes*sizeof(SortedListElement_t));
    
    for(int i=0; i<numNodes; i++){
        char* keystring = (char*)malloc(5*sizeof(char));
        for(int j=0; j<5; j++){
            keystring[j] = 'a'+rand()%26;
        }
        nodelist[i].key = keystring;
    }


    //start threads
    pthread_t* tid = (pthread_t*)malloc(threads*sizeof(pthread_t));
    threadInds = (int*)malloc(threads*sizeof(int));

    //create array to track mutex wait times for each thread
    mutex_thread_wait_times = (long*)malloc(threads*sizeof(long));
    for(int i=0; i<threads; i++){
        mutex_thread_wait_times[i] = 0;
    }

    //start timer
    struct timespec starttime;
    struct timespec endtime;
    if(clock_gettime(CLOCK_MONOTONIC, &starttime)==-1){
        fprintf(stderr, "Error with getting start time: %s\n", strerror(errno));
        exit(1);
    }

    
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

    //get total mutex waits
    long total_mutex_waits = 0;
    for(int i=0; i<threads; i++){
        total_mutex_waits+=mutex_thread_wait_times[i];
    }

    //get end time
    if(clock_gettime(CLOCK_MONOTONIC, &endtime)==-1){
        fprintf(stderr, "Error with getting end time: %s\n", strerror(errno));
        exit(1);
    }

    //destroy mutex lock
    if(opt_sync=='m'){
        for(int i=0; i<num_lists; i++){
            if(pthread_mutex_destroy(&mutexlocks[i]) != 0){
                fprintf(stderr, "Error destroying lock\n");
                exit(1);
            }
        }
        free(mutexlocks);
    }else if(opt_sync=='s'){
        free(spinlocks);
    }

    //free all memory
    free(threadInds);
    free(tid);
    for(int i=0; i<numNodes; i++){
        free((char*)nodelist[i].key);
    }
    free(nodelist);
    free(mutex_thread_wait_times);

    int final_length = 0;
    for(int i=0; i<num_lists; i++){
        final_length+=SortedList_length(&lists[i]);
    }
    
    if(final_length!=0){
        fprintf(stderr, "Error: final list length is not 0 --threads=%d --iterations=%d --yield=%s --sync=%s\n", threads, iterations, getYield(), getSync());
        exit(2);
    }

    long billion = 1000000000;
    long total_time = billion*(endtime.tv_sec - starttime.tv_sec) + endtime.tv_nsec-starttime.tv_nsec;

    long operations = threads*iterations*3;
    long avg_time = total_time/operations;

    long avg_wait_for_mutex_lock = total_mutex_waits/operations;
    printf("list-%s-%s,%d,%d,%d,%ld,%ld,%ld,%ld\n", getYield(), getSync(), threads, iterations, num_lists, operations, total_time, avg_time, avg_wait_for_mutex_lock);
}
