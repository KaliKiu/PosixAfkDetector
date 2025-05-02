#ifndef THREADS_H
#define THREADS_H
#include <pthread.h>  
#include <time.h>      
//TEST
//functions

void afk_status();
void afk_set_timeout(int seconds);
void afk_start(int pipefd_afk[0]);
void afk_input();
#endif 