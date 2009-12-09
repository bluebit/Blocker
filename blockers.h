#ifndef BLOCKERS_H
#define BLOCKERS_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <curses.h>
#include <pthread.h>
#include <unistd.h>

#define SET_COLOR_RED       attron(COLOR_PAIR(1))
#define SET_COLOR_WHITE     attron(COLOR_PAIR(2))
#define SET_COLOR_BLUE      attron(COLOR_PAIR(3))
#define SET_BLACK_ON_WHITE  attron(COLOR_PAIR(4))
#define SET_COLOR_BLACK     attron(COLOR_PAIR(5))

// Block-related functions
void moveBlockToRight(int row, int *start, int *end, int screen_width);
void moveBlockToLeft(int row, int *start, int *end);

// Bomb-related functions
void *drop_bombs(void *arg);
void *blockMovement(void *arg);
void *create_bomb(void *arg);

// Score-related functions
void displayScore();
void increaseScore();
void decreaseScore();
void calculateScore();
void *changeSpeed(void *arg);

// ThreadPool-related functions
void addThread(pthread_t tid);
void removeThread(pthread_t tid);
void cancelAllThreads();

typedef struct Block Block;
typedef struct ThreadPool ThreadPool;
typedef struct Score Score;

#endif
