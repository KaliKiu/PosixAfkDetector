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
#include <stdbool.h>
#include <signal.h>

// Shared variables
static struct {
    struct timespec start;
    bool is_thread_running;
    bool should_exit;
    bool is_afk;
    bool terminate_threads;
    float elapsed;
    pthread_mutex_t mutex;
    pthread_t afk_thread;
    pthread_t counter_thread;
} afk_state = {
    .is_thread_running = false,
    .should_exit = false,
    .is_afk = false,
    .terminate_threads = false,
    .elapsed = 0
};

// Constants
#define AFK_TIMEOUT 20.0f  // Seconds before user is considered AFK
#define THREAD_SLEEP_MS 100 // Sleep interval in milliseconds

int reset_afk() {
    pthread_mutex_lock(&afk_state.mutex);
    clock_gettime(CLOCK_MONOTONIC, &afk_state.start);
    afk_state.is_afk = false;
    afk_state.elapsed = 0;
    pthread_mutex_unlock(&afk_state.mutex);
    return 0;
}

void* afk_check_thread(void* arg) {
    struct timespec end;
    
    while (1) {
        // Check if we should terminate
        pthread_mutex_lock(&afk_state.mutex);
        if (afk_state.terminate_threads) {
            pthread_mutex_unlock(&afk_state.mutex);
            break;
        }
        
        // Calculate elapsed time
        clock_gettime(CLOCK_MONOTONIC, &end);
        afk_state.elapsed = (end.tv_sec - afk_state.start.tv_sec) + 
                           (end.tv_nsec - afk_state.start.tv_nsec) / 1e9;
        
        // Check AFK status
        if (afk_state.elapsed > AFK_TIMEOUT && !afk_state.is_afk) {
            printf("You're afk\n");
            afk_state.is_afk = true;
        } else if (afk_state.elapsed > AFK_TIMEOUT && afk_state.should_exit && afk_state.is_afk) {
            printf("You were afk for %.3f Seconds\n", afk_state.elapsed);
            afk_state.should_exit = false;
            clock_gettime(CLOCK_MONOTONIC, &afk_state.start);
            afk_state.is_afk = false;
        } else if (afk_state.should_exit) {
            afk_state.should_exit = false;
            clock_gettime(CLOCK_MONOTONIC, &afk_state.start);
            afk_state.is_afk = false;
        }
        pthread_mutex_unlock(&afk_state.mutex);
        
        usleep(THREAD_SLEEP_MS * 1000);
    }
    
    return NULL;
}

void* afk_counter_thread(void* arg) {
    int read_pipe = *(int*)arg;
    int buffer[1];
    
    pthread_mutex_lock(&afk_state.mutex);
    afk_state.is_thread_running = true;
    pthread_mutex_unlock(&afk_state.mutex);
    
    printf("Afk_thread_started: %lu\n", (unsigned long)pthread_self());
    
    if (pthread_create(&afk_state.afk_thread, NULL, afk_check_thread, NULL) != 0) {
        perror("Error: Failed to create afk check thread");
        return NULL;
    }
    
    while (1) {
        // Check if we should terminate
        pthread_mutex_lock(&afk_state.mutex);
        if (afk_state.terminate_threads) {
            pthread_mutex_unlock(&afk_state.mutex);
            break;
        }
        pthread_mutex_unlock(&afk_state.mutex);
        
        // Read from pipe (will block until data available)
        ssize_t bytes_read = read(read_pipe, buffer, sizeof(buffer));
        if (bytes_read == -1) {
            perror("Read from pipe failed");
            continue;
        }
        
        // Activity detected, signal to reset AFK status
        pthread_mutex_lock(&afk_state.mutex);
        afk_state.should_exit = true;
        pthread_mutex_unlock(&afk_state.mutex);
        
        sleep(1);
    }
    
    return NULL;
}

void afk_counter_cleanup() {
    pthread_mutex_lock(&afk_state.mutex);
    afk_state.terminate_threads = true;
    pthread_mutex_unlock(&afk_state.mutex);
    
    // Wait for threads to terminate
    pthread_join(afk_state.counter_thread, NULL);
    pthread_join(afk_state.afk_thread, NULL);
    
    // Destroy mutex
    pthread_mutex_destroy(&afk_state.mutex);
}

void afk_counter(int pipefd_afk[2]) {
    // Initialize mutex
    if (pthread_mutex_init(&afk_state.mutex, NULL) != 0) {
        perror("Mutex initialization failed");
        exit(1);
    }
    
    // Reset state
    pthread_mutex_lock(&afk_state.mutex);
    afk_state.is_thread_running = false;
    afk_state.terminate_threads = false;
    reset_afk();
    pthread_mutex_unlock(&afk_state.mutex);
    
    // Create counter thread
    if (pthread_create(&afk_state.counter_thread, NULL, afk_counter_thread, &pipefd_afk[0]) != 0) {
        perror("Error: Failed to create thread");
        pthread_mutex_destroy(&afk_state.mutex);
        exit(1);
    }
    
    // Wait for thread to start
    while (1) {
        pthread_mutex_lock(&afk_state.mutex);
        bool running = afk_state.is_thread_running;
        pthread_mutex_unlock(&afk_state.mutex);
        
        if (running) {
            break;
        }
        usleep(500000);  // Sleep for 0.5 seconds
    }
    
    // Register cleanup function to be called on program exit
    atexit(afk_counter_cleanup);
}