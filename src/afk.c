#define _POSIX_C_SOURCE 199309L
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>

int is_thread_running;
struct timespec start;
int should_exit;
int is_afk;
struct timespec end;
float elapsed;


int reset_afk(){
    clock_gettime(CLOCK_MONOTONIC, &start);
    is_afk =0;
    elapsed =0;
    return 0;
}

void* afk_check_thread(void*arg){
    should_exit=0;
    is_afk=0;
    reset_afk(); 
    while(1){
        clock_gettime(CLOCK_MONOTONIC,&end);
        elapsed = (end.tv_sec-start.tv_sec) + (end.tv_nsec -start.tv_nsec)/1e9;
        if(elapsed >20 && is_afk ==0){
            printf("You're afk\n");
            is_afk=1;
        }else if(elapsed>20&&should_exit==1 &&is_afk==1){
             printf("You were afk for %.3f Seconds\n", elapsed);
             reset_afk();
        }else if(should_exit){
             reset_afk();
        }
        usleep(100 * 1000);  // Sleep for 100 milliseconds (adjust as needed)
    }
}
void* afk_counter_thread(void*arg){
    is_thread_running =1;
    pthread_t thread_id = pthread_self();
    printf("Afk_thread_started: %d\n",(void*)thread_id);

    int read_pipe = *(int*)arg;
    int buffer[1];
    double elapsed;
    pthread_t thread;
    if(pthread_create(&thread,NULL,afk_check_thread,&start)){
            perror("Error: Failed to create thread");
            exit(1);
        }
    pthread_detach(thread);
    //thread loop that measures the time interval between not typing
    while(1){
        should_exit = 0;
        void* thread_result;
        
        //waits for changes on pipefd[0]
        ssize_t bytes_read = read(read_pipe,buffer,sizeof(buffer)); //does this block?? if there is no input?
            if(bytes_read == -1){
                perror("read failed");
            }
                should_exit=1;
            //check if read return is NULL? Only if non blocking read()
            
        sleep(1);    
    }
}

void afk_counter(int pipefd_afk[0]){
    is_thread_running=0;
    pthread_t thread;
    if(pthread_create(&thread,NULL,afk_counter_thread,&pipefd_afk[0])){
        perror("Error: Failed to create thread");
        exit(1);
    }
    pthread_detach(thread);
    while(is_thread_running==0){
        sleep(1);
    }
}