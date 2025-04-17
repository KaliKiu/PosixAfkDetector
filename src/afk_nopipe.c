#define _POSIX_C_SOURCE 199309L
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <stdint.h>

#define AFK_TIMEOUT 20.0f  // Seconds before user is considered AFK
#define THREAD_SLEEP_MS 100*1000 // Sleep interval in milliseconds


static struct{
    struct timespec start,end;
    float elapsed;
    int is_afk;
    int is_thread_running;
}afk_check={
    .elapsed =0,
    .is_afk=0,
    .is_thread_running=0
};

static struct{
    pthread_mutex_t mutex;
    int should_exit;
}afk_tc={
    .should_exit=0,
};
static struct{
    pthread_mutex_t mutex;
    int signal;
}afk_signal={
    .signal=0
}


void afk_input(){

}
//should_exit 
void reset_afk(){
    clock_gettime(CLOCK_MONOTONIC, &afk_check.start);
    afk_check.is_afk =0;
    afk_check.elapsed =0;
}


void* afk_check_thread(void*arg){
    reset_afk(); 
    while(1){
        clock_gettime(CLOCK_MONOTONIC,&afk_check.end);
        pthread_mutex_lock(&afk_tc.mutex);
        afk_check.elapsed = (afk_check.end.tv_sec-afk_check.start.tv_sec) + (afk_check.end.tv_nsec -afk_check.start.tv_nsec)/1e9;
        
        if(afk_check.elapsed >20 && afk_check.is_afk ==0){
            printf("~~You're afk~~\n\n");
            afk_check.is_afk=1;
        }else if(afk_tc.should_exit==1 &&afk_check.is_afk==1){
             printf("\n~~You were afk for %.3f Seconds\n\n", afk_check.elapsed);
             printf("gambashell>\n");
             reset_afk();
        }else if(afk_tc.should_exit){
             reset_afk();
        }
        pthread_mutex_unlock(&afk_tc.mutex);
        usleep(THREAD_SLEEP_MS); // Sleep for 100 milliseconds (adjust as needed)
    }
    return NULL;
}
void* afk_counter_thread(void*arg){
    int buffer[1];
    pthread_t thread;

    if(pthread_create(&thread,NULL,afk_check_thread,&afk_check.start)){
            perror("Error: Failed to create thread");
            exit(1);
        }
    pthread_detach(thread);
    //thread loop that measures the time interval between not typing
    while(1){
        pthread_mutex_lock(&afk_tc.mutex);
        afk_tc.should_exit = 0;
        pthread_mutex_unlock(&afk_tc.mutex);
        //waits for changes on pipefd[0]
        pthread_mutex_lock(&afk_signal.mutex);
        while(!afk_signal.signal){
            usleep(THREAD_SLEEP_MS);
        }
        afk_signal.signal=0;
        pthread_mutex_unlock(&afk_signal.mutex);
        pthread_mutex_lock(&afk_tc.mutex);
        afk_tc.should_exit=1;
        pthread_mutex_unlock(&afk_tc.mutex);
         
            
        usleep(THREAD_SLEEP_MS);    
    }
    return NULL;
}

void afk_init(){
    printf("starting...");
    // Initialize mutex
    if (pthread_mutex_init(&afk_tc.mutex, NULL) != 0) {
        perror("Mutex initialization failed");
        exit(1);
    }
    if (pthread_mutex_init(&afk_input.mutex, NULL) != 0) {
        perror("Mutex initialization failed");
        exit(1);
    }

    pthread_t thread;
    if(pthread_create(&thread,NULL,afk_counter_thread,NULL)){
        perror("Error: Failed to create thread");
        exit(1);
    }
    pthread_detach(thread);

    while(afk_check.is_thread_running=0){
        usleep(THREAD_SLEEP_MS);
    }
        
   
}