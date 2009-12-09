#include "blockers.h"

struct Block {
    int screen_width, screen_height;
    int row, col;
    int start, end;
    int *cols;
    struct Block *block;
};

struct Score {
    int bombs_caught, bombs_missed, points;
};

// linked list, to keep track of all the threads
struct ThreadPool {
	pthread_t tid;
	struct ThreadPool *next;
};

Score score;
ThreadPool threadPool;

pthread_mutex_t mutex;
int speed;

int main(void) {
    // int pid = getpid();
    // printf("PID = %d\n", pid);
    // getchar();
    
    initscr();
    cbreak(); noecho(); keypad(stdscr, TRUE);
    
    int scr_width, scr_height;
    getmaxyx(stdscr, scr_height, scr_width);
    int columns[scr_width];
    int arr_index;
    for(arr_index = 0; arr_index < scr_width; ++arr_index)
    	columns[arr_index] = 0;
    
    start_color();
    
    assume_default_colors(COLOR_WHITE, COLOR_WHITE);
    
    init_pair(1, COLOR_RED, COLOR_RED);
    init_pair(2, COLOR_WHITE, COLOR_WHITE);
    init_pair(3, COLOR_BLUE, COLOR_BLUE);
    init_pair(4, COLOR_BLACK, COLOR_WHITE);
    init_pair(5, COLOR_BLACK, COLOR_BLACK);
    
    int block_start = 1;
    int block_end = block_start + 20;
    int block_row = scr_height - 1;
    
    int index;
    
    SET_COLOR_RED;
    for(index = block_start; index < 20; ++index) { move(block_row, index); delch(); insch(' '); }
    
    SET_COLOR_BLACK;
    for(index = 0; index <= scr_width; ++index) { move(1, index); delch(); insch(' '); }
    
    // hide cursor. doesn't work?.
    curs_set(0);
    refresh();
    // pthread_attr_t attr;
    
    pthread_mutex_init(&mutex, NULL);
    
    Block *block = malloc(sizeof(Block)); // freed by another thread
    block->start = 1;
    block->end = (block->start) + 20;
    block->row = scr_height - 1;
    block->screen_height = scr_height;
    block->screen_width = scr_width;
    block->cols = columns;
    
    // initialize score information
    score.bombs_caught = 0;
    score.bombs_missed = 0;
    score.points = 0;
    displayScore();
    
    speed = 1000; // 1 second
    
    pthread_t block_thread;
    threadPool.tid = block_thread;
    threadPool.next = NULL;
    pthread_create(&block_thread, NULL, blockMovement, (void *) block);
    
    pthread_t timer_thread;
    pthread_create(&timer_thread, NULL, changeSpeed, (void *) 0);
    addThread(timer_thread);
    
    pthread_t bomb_thread;
    pthread_create(&bomb_thread, NULL, drop_bombs, (void *) block);
    addThread(bomb_thread);
    // void *status;
    // wait for the user to press q to actually quit
    pthread_join(block_thread, NULL);
    // free(score);    
    pthread_mutex_destroy(&mutex);
    pthread_exit(NULL);
}

void *changeSpeed(void *arg) {
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	while(1) {
		pthread_mutex_lock(&mutex);
		--speed;
		pthread_mutex_unlock(&mutex);
		napms(500);
	}
}

void displayScore() {
    SET_BLACK_ON_WHITE;
    mvprintw(0, 0, "Bombs caught: %04d --- Bombs missed: %04d --- Score: %04d",
    	score.bombs_caught, score.bombs_missed, score.points);
}

void increaseScore() {
    score.bombs_caught++;
    calculateScore();
}

void decreaseScore() {
    score.bombs_missed++;
    calculateScore();
}

void calculateScore() {
	score.points = (score.bombs_caught * 2) + (score.bombs_missed * -7);
}

void *blockMovement(void *arg) {
    Block *block = (Block *) arg;
    
    int is_last_move_right = 1;
    int ch;
    // timeout(-1);
    while(1) {
        ch = getch();
        switch(ch) {
            case 'q':
                pthread_mutex_lock(&mutex);
                cancelAllThreads();
                free(block);
                endwin();
                pthread_mutex_unlock(&mutex);
                pthread_exit((void *) 0);
            case 'p': // a hack to pause the game
            	pthread_mutex_lock(&mutex);
            	getch();
            	pthread_mutex_unlock(&mutex);
            case KEY_RIGHT:
                if(!is_last_move_right) { ++(block->end); is_last_move_right = 1; }
                pthread_mutex_lock(&mutex);
                moveBlockToRight(block->row, &(block->start), &(block->end), block->screen_width);
                pthread_mutex_unlock(&mutex);
                is_last_move_right = 1;
                break;
            case KEY_LEFT:
                if(is_last_move_right) { --(block->end); is_last_move_right = 0; }
                pthread_mutex_lock(&mutex);
                moveBlockToLeft(block->row, &(block->start), &(block->end));
                pthread_mutex_unlock(&mutex);
                break;
        }
    }
}

void moveBlockToRight(int row, int *start, int *end, int screen_width) {
    if(*end >= screen_width) return;
    SET_COLOR_WHITE;
    move(row, *start);
    delch(); insch(' ');
    (*start)++;
    
    SET_COLOR_RED;
    move(row, *end);
    delch(); insch(' ');
    (*end)++;
    return;
}

void moveBlockToLeft(int row, int *start, int *end) {
    if(*start < 1) return;
    SET_COLOR_RED;
    (*start)--;
    move(row, *start);
    delch(); insch(' ');
    
    SET_COLOR_WHITE;
    move(row, *end);
    delch(); insch(' ');
    (*end)--;
    move(row, *end);
    return;
}

void *drop_bombs(void *arg) {
    Block *block;
    block = (Block *) arg;
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    int rand_wait_time;
    int bomb_position;
    Block *bomb;
    srand(time(NULL));
    while(1) {
        pthread_t *pid = malloc(sizeof(pthread_t));
        bomb = malloc(sizeof(Block));
        while(1) {
        	bomb->col = (rand() % block->screen_width);
        	pthread_mutex_lock(&mutex);
        	if(block->cols[bomb->col] == 0) {
        		block->cols[bomb->col] = 1;
        		break;
        	}
        	pthread_mutex_unlock(&mutex);
        }
        pthread_mutex_unlock(&mutex);
        bomb->row = 2;
        bomb->screen_height = block->screen_height;
        bomb->screen_width = block->screen_width;
        bomb->block = block;
        pthread_mutex_lock(&mutex);
        bomb->cols = block->cols;
        rand_wait_time = (rand() % 4 + 1) * speed; // wait is between 1 and 3 seconds
        pthread_create(pid, NULL, create_bomb, (void *) bomb);
        pthread_mutex_unlock(&mutex);
        pthread_testcancel();
        napms(rand_wait_time);
    }
}

void delete_bomb(Block *bomb) {
	int index;
    SET_COLOR_WHITE;
    for(index = 2; index < bomb->screen_height - 1; ++index) {
        move(index, bomb->col);
        delch(); insch(' ');
    }
}

void *create_bomb(void *arg) {
    Block *bomb;
    bomb = (Block *) arg;
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    pthread_mutex_lock(&mutex);
    addThread(pthread_self());
    pthread_mutex_unlock(&mutex);
    while(bomb->row <= bomb->screen_height) {
        if(bomb->row == bomb->screen_height - 1) {
            pthread_mutex_lock(&mutex);
            if(bomb->col >= bomb->block->start && bomb->col <= bomb->block->end) {
                increaseScore();
                displayScore();
            } else {
                flash();
                decreaseScore(score);
                displayScore();
            }
            delete_bomb(bomb);
            pthread_mutex_unlock(&mutex);
            break; 
        }
        pthread_mutex_lock(&mutex);
        SET_COLOR_BLUE;
        move(bomb->row, bomb->col);
        delch(); insch(' ');
        (bomb->row)++;
        SET_COLOR_RED;
        pthread_mutex_unlock(&mutex);
        refresh();
        pthread_testcancel();
        napms(150);
    }
    
    pthread_mutex_lock(&mutex);
    bomb->cols[bomb->col] = 0;
    removeThread(pthread_self());
    pthread_mutex_unlock(&mutex);
    
    free(bomb);
    pthread_exit((void *) 0);
}

void addThread(pthread_t tid) {
	ThreadPool *thread;
	thread = &threadPool;
	while(thread->next != NULL) {
		thread = thread->next;
	}
	ThreadPool *newThread = malloc(sizeof(ThreadPool));
	newThread->tid = tid;
	newThread->next = NULL;
	thread->next = newThread;
}

void removeThread(pthread_t tid) {
	ThreadPool *target, *trail, *lead;
	trail = &threadPool;
	lead = trail->next;
	while(lead->tid != tid) {
		if(lead->next == NULL) break;
		trail = lead;
		lead = lead->next;
	}
	target = lead;
	lead = lead->next;
	free(target);
	trail->next = lead;
}

void cancelAllThreads() {
	ThreadPool *thread, *cancelled;
	thread = threadPool.next;
	while(thread->next != NULL) {
		pthread_cancel(thread->tid);
		cancelled = thread;
		thread = thread->next;
		free(cancelled);
	}
	pthread_cancel(thread->tid);
	free(thread);
}

// void *displayTime(void *arg) {
//     int seconds = 0;
//     
// }
