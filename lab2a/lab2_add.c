/* NAME: Samuel Shen */
/* EMAIL: sam.shen321@gmail.com */
/* ID: 405325252 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sched.h>

pthread_mutex_t lock;
int threads = 1;
int iterations = 1;
long long counter = 0;
int opt_yield = 0;
char opt_sync = 'n';
int spin_lock = 0;
extern int errno;

void print_output(long operations, long total_time, long avg_time){
    if(opt_yield==0){
        if(opt_sync=='n'){
            printf("add-none,%d,%d,%ld,%ld,%ld,%lli\n", threads, iterations, operations, total_time, avg_time, counter);
        }else if(opt_sync=='m'){
            printf("add-m,%d,%d,%ld,%ld,%ld,%lli\n", threads, iterations, operations, total_time, avg_time, counter);
        }else if(opt_sync=='s'){
            printf("add-s,%d,%d,%ld,%ld,%ld,%lli\n", threads, iterations, operations, total_time, avg_time, counter);
        }else if(opt_sync=='c'){
            printf("add-c,%d,%d,%ld,%ld,%ld,%lli\n", threads, iterations, operations, total_time, avg_time, counter);
        }
    }else if(opt_yield==1){
        if(opt_sync=='n'){
            printf("add-yield-none,%d,%d,%ld,%ld,%ld,%lli\n", threads, iterations, operations, total_time, avg_time, counter);
        }else if(opt_sync=='m'){
            printf("add-yield-m,%d,%d,%ld,%ld,%ld,%lli\n", threads, iterations, operations, total_time, avg_time, counter);
        }else if(opt_sync=='s'){
            printf("add-yield-s,%d,%d,%ld,%ld,%ld,%lli\n", threads, iterations, operations, total_time, avg_time, counter);
        }else if(opt_sync=='c'){
            printf("add-yield-c,%d,%d,%ld,%ld,%ld,%lli\n", threads, iterations, operations, total_time, avg_time, counter);
        }
    }
}

void add_mutex(long long *pointer, long long value) {
    //mutex
    pthread_mutex_lock(&lock);
    long long sum = *pointer + value;
    if(opt_yield){
        sched_yield();
    }
    *pointer = sum;
    pthread_mutex_unlock(&lock);
}

void add_spin(long long *pointer, long long value) {
    //spin until lock = 0
    while(__sync_lock_test_and_set(&spin_lock, 1)==1);
    
    long long sum = *pointer + value;
    if(opt_yield){
        sched_yield();
    }
    *pointer = sum;

    __sync_lock_release(&spin_lock);
}

void add_cas(long long *pointer, long long value) {
    //compare and switch
    long long old;
    long long new;
    do{
        old = *pointer;
        new = old+value;
        if(opt_yield){
            sched_yield();
        }
    }while(__sync_val_compare_and_swap(pointer, old, new)!=old);

}

void add(long long *pointer, long long value) {
    long long sum = *pointer + value;
    if(opt_yield){
        sched_yield();
    }
    *pointer = sum;
}


void* runThread(){
    for(int i=0; i<iterations; i++){
        if(opt_sync=='n'){
            add(&counter, 1);
            add(&counter, -1);
        }else if(opt_sync=='m'){
            add_mutex(&counter, 1);
            add_mutex(&counter, -1);
        }else if(opt_sync=='s'){
            add_spin(&counter, 1);
            add_spin(&counter, -1);
        }else if(opt_sync=='c'){
            add_cas(&counter, 1);
            add_cas(&counter, -1);
        }
    }

    return NULL;
}

int main(int argc, char** argv){
    int c;
    while(1){
        static struct option long_options[] = {
            {"threads", required_argument, 0, 't'},
            {"iterations", required_argument, 0, 'i'},
            {"yield", no_argument, 0, 'y'},
            {"sync", required_argument, 0, 's'},
            {0,0,0,0}
        };
        int option_index = 0;
        c = getopt_long(argc, argv, "s:t:i:y", long_options, &option_index);

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
            opt_yield = 1;
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

    if(opt_sync!='n' && opt_sync!='m' && opt_sync!='s' && opt_sync!='c'){
        fprintf(stderr, "Unrecognized Argument for sync: (m, s, c)\n");
        exit(1);
    }

    if(opt_sync=='m'){
        if(pthread_mutex_init(&lock, NULL) != 0){
            fprintf(stderr, "Mutex init failed\n");
            exit(1);
        }
    }

    struct timespec starttime;
    struct timespec endtime;

    pthread_t* tid = (pthread_t*)malloc(threads*sizeof(pthread_t));

    //    printf("Creating %d threads with %d iterations\n", threads, iterations);

    if(clock_gettime(CLOCK_MONOTONIC, &starttime)==-1){
        fprintf(stderr, "Error with getting start time: %s\n", strerror(errno));
        exit(1);
    }
    
    //create threads
    for(int i=0; i<threads; i++){
        if(pthread_create(&(tid[i]), NULL, &runThread, NULL)){
            fprintf(stderr, "Error creating thread : [%s]\n", strerror(errno));
            //free memory
            free(tid);
            exit(1);
        }
    }

    //join all threads
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

    long billion = 1000000000;
    long total_time = billion*(endtime.tv_sec - starttime.tv_sec) + endtime.tv_nsec-starttime.tv_nsec;

    free(tid);

    if(opt_sync=='m'){
        if(pthread_mutex_destroy(&lock) != 0){
            fprintf(stderr, "Error destroying lock\n");
            exit(1);
        }
    }
    
    
    long operations = threads*iterations*2;
    long avg_time = total_time/operations;


    print_output(operations, total_time, avg_time);
    
    exit(0);
}
