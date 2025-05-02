/*Author: KaliKiu*/


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

#define AFK_TIMEOUT 20.0f
#define THREAD_SLEEP_US 100*1000

static struct {
    pthread_mutex_t mutex;
    int timeout;
} afk_cfg = {
    .timeout = 20,
};

static struct {
    struct timespec start, end;
    float elapsed;
    int is_running;
} afk_tm = {
    .is_running = 0,
};

static struct {
    pthread_mutex_t mutex;
    int input_recv;
    int is_afk;
} afk_st = {
    .input_recv = 0,
    .is_afk = 0,
};

static struct {
    pthread_mutex_t mutex;
    int input;
} afk_sig = {
    .input = 0
};

void afk_set_timeout(int seconds) {
    pthread_mutex_lock(&afk_cfg.mutex);
    afk_cfg.timeout = seconds;
    pthread_mutex_unlock(&afk_cfg.mutex);
}

int afk_status() {
    pthread_mutex_lock(&afk_st.mutex);
    int result = afk_st.is_afk;
    pthread_mutex_unlock(&afk_st.mutex);
    return result;
}

void afk_input() {
    pthread_mutex_lock(&afk_sig.mutex);
    afk_sig.input = 1;
    pthread_mutex_unlock(&afk_sig.mutex);
}

void afk_reset() {
    clock_gettime(CLOCK_MONOTONIC, &afk_tm.start);
    pthread_mutex_lock(&afk_st.mutex);
    afk_st.is_afk = 0;
    pthread_mutex_unlock(&afk_st.mutex);
    afk_tm.elapsed = 0;
    usleep(THREAD_SLEEP_US);
}

void* afk_loop(void* arg) {
    afk_reset(); 
    while (1) {
        clock_gettime(CLOCK_MONOTONIC, &afk_tm.end);

        pthread_mutex_lock(&afk_st.mutex);
        pthread_mutex_lock(&afk_cfg.mutex);

        afk_tm.elapsed = (afk_tm.end.tv_sec - afk_tm.start.tv_sec) + 
            (afk_tm.end.tv_nsec - afk_tm.start.tv_nsec) / 1e9;

        if (afk_tm.elapsed > afk_cfg.timeout && afk_st.is_afk == 0) {
            printf("~~You're afk~~\n\n");
            afk_st.is_afk = 1;
        } else if (afk_st.input_recv == 1 && afk_st.is_afk == 1) {
            printf("\n~~You were afk for %.3f Seconds\n\n", afk_tm.elapsed);
            printf("gambashell>\n");
            afk_reset();
        } else if (afk_st.input_recv == 1) {
            afk_reset();
        }

        pthread_mutex_unlock(&afk_cfg.mutex);
        pthread_mutex_unlock(&afk_st.mutex);

        usleep(THREAD_SLEEP_US);
    }
    return NULL;
}

void* afk_monitor(void* arg) {
    pthread_t thread;

    if (pthread_create(&thread, NULL, afk_loop, NULL)) {
        perror("Error: Failed to create thread");
        exit(1);
    }
    pthread_detach(thread);

    afk_tm.is_running = 1;

    while (1) {
        pthread_mutex_lock(&afk_st.mutex);
        afk_st.input_recv = 0;
        pthread_mutex_unlock(&afk_st.mutex);

        pthread_mutex_lock(&afk_sig.mutex);
        while (!afk_sig.input) {
            pthread_mutex_unlock(&afk_sig.mutex);
            usleep(THREAD_SLEEP_US);
            pthread_mutex_lock(&afk_sig.mutex);
        }
        afk_sig.input = 0;
        pthread_mutex_unlock(&afk_sig.mutex);

        pthread_mutex_lock(&afk_st.mutex);
        afk_st.input_recv = 1;
        pthread_mutex_unlock(&afk_st.mutex);

        usleep(THREAD_SLEEP_US * 2);
    }
    return NULL;
}

void afk_start() {
    printf("starting...");

    if (pthread_mutex_init(&afk_cfg.mutex, NULL) != 0 ||
        pthread_mutex_init(&afk_st.mutex, NULL) != 0 ||
        pthread_mutex_init(&afk_sig.mutex, NULL) != 0) {
        perror("Mutex initialization failed");
        exit(1);
    }

    pthread_t thread;
    if (pthread_create(&thread, NULL, afk_monitor, NULL)) {
        perror("Error: Failed to create thread");
        exit(1);
    }
    pthread_detach(thread);

    while (afk_tm.is_running == 0) {
        usleep(THREAD_SLEEP_US);
    }
}
