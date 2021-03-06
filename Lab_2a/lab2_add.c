#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <time.h>
#include <string.h>

/* muti-threading objective variable */
long long counter = 0;

/* global variables */
long long num_thr = 1;    /* number of threads */
long long num_itr = 1;    /* number of iterations */
int debug_flag = 0; /* flag debug mode */
int opt_yield = 0; /* flag yield */
char sync = 0; /* flag which kind of sync */
void (*add_func)(long long*, long long); /* container variable for addition func */

/* global locks */
pthread_mutex_t mutex;
volatile int spin_lock = 0;

/* option error message */
static char *error_message =
    "usage\n  --threads=# number of threads to run\n"
    "  --iterations=# number of iterations to run\n"
    "  --debug activate debug mode";


/* safe wrap of clock_gettime */
int m_clock_gettime(clockid_t id, struct timespec *tp) {
    int rc = 0;
    rc = clock_gettime(id, tp);
    if (rc != 0) {
        fprintf(stderr, "ERROR: clock_gettime() return %d, exiting...\n", rc);
        exit(1);
    }
    else
        return rc;
}

/* safe wrap of pthread_mutex_init */
int m_pthread_mutex_init(pthread_mutex_t *__mutex, const pthread_mutexattr_t *__mutexattr) {
    int rc = pthread_mutex_init(__mutex, __mutexattr);
    if (rc != 0) {
        fprintf(stderr, "ERROR: pthread_mutex_init() returned %d, exiting...\n", rc);
        exit(1);
    }
    else 
        return rc;
}

/* safe wrap of pthread_create */
int m_pthread_create(pthread_t *__restrict__ __newthread, const pthread_attr_t *__restrict__ __attr, 
                     void *(*__start_routine)(void *), void *__restrict__ __arg) {
    int rc = pthread_create(__newthread, __attr, __start_routine, __arg);
    if (rc != 0) {
        fprintf(stderr, "ERROR: pthread_create() returned %d, exiting...\n", rc);
        exit(1);
    }
    else 
        return rc;
}

/* safe wrap of pthread_join */
int m_pthread_join(pthread_t __th, void **__thread_return) {
    int rc = pthread_join(__th, __thread_return);
    if (rc != 0) {
        fprintf(stderr, "ERROR: pthread_join() returned %d, exiting...\n", rc);
        exit(1);
    }
    else 
        return rc;
}

int m_pthread_mutex_lock(pthread_mutex_t *__mutex) {
    int rc = pthread_mutex_lock(__mutex);
    if (rc != 0) {
        fprintf(stderr, "ERROR: pthread_mutex_lock() returned %d, exiting...\n", rc);
        exit(1);
    }
    else 
        return rc;
}

int m_pthread_mutex_unlock(pthread_mutex_t *__mutex) {
    int rc = pthread_mutex_unlock(__mutex);
    if (rc != 0) {
        fprintf(stderr, "ERROR: pthread_mutex_unlock() returned %d, exiting...\n", rc);
        exit(1);
    }
    else 
        return rc;
}


void add(long long *pointer, long long value) {
    long long sum = *pointer + value;
    if (opt_yield)
        sched_yield();
    *pointer = sum;
}

void mutexAdd(long long *pointer, long long value) {
    m_pthread_mutex_lock(&mutex);
    long long sum = *pointer + value;
    if (opt_yield)
        sched_yield();
    *pointer = sum;
    m_pthread_mutex_unlock(&mutex);
}

void spinLockAdd(long long *pointer, long long value) {
    while (__sync_lock_test_and_set(&spin_lock, 1));
    long long sum = *pointer + value;
    if (opt_yield)
        sched_yield();
    *pointer = sum;
    __sync_lock_release(&spin_lock);
}

void atomAdd(long long *pointer, long long value) {
    long long old;
    do {
        old = *pointer;
        if (opt_yield)
            sched_yield();
    } while (__sync_val_compare_and_swap(pointer, old, old + value) != old);
}

void *threadRoutine(void *vargp) {
    long long i = (long long) vargp;
    for (i = 0; i < num_itr; i++) {
        add_func(&counter, 1);
    }
    for (i = 0; i < num_itr; i++) {
        add_func(&counter, -1);
    }
    pthread_exit(0);
}


int main(int argc, char **argv) {
    static char* sync_type = "none";
    add_func = add;

    // precess args using getopt
    struct option options[6] = {
        {"threads", required_argument, 0, 't'},
        {"iterations", required_argument, 0, 'i'},
        {"yield", no_argument, 0, 'y'},
        {"sync", required_argument, 0, 's'},
        {"debug", no_argument, 0, 'd'},
        {0, 0, 0, 0}
    };
    int temp = 0;
    while ((temp = getopt_long(argc, argv, "-", options, NULL)) != -1) {
        switch (temp) {
        case 1:
            fprintf(stderr, "Bad non-option argument: %s\n%s\n", argv[optind - 1], error_message);
            exit(1);
            break;
          case 't': // --threads=#
            num_thr = atoi(optarg);
            break;
          case 'i': // --iterations=#
            num_itr = atoi(optarg);
            break;
          case 'd': // --debug
            debug_flag = 1;
            break;
          case 'y':
            opt_yield = 1;
            break;
          case 's':
            if (optarg) {
                if (strcmp(optarg, "m") == 0) {
                    sync = 'm';
                    sync_type = "m";
                }
                else if (strcmp(optarg, "s") == 0) {
                    sync = 's';
                    sync_type = "s";
                }
                else if (strcmp(optarg, "c") == 0) {
                    sync = 'c';
                    sync_type = "c";
                }
                else {
                    fprintf(stderr, "unrecognized sync option provided, exiting...\n");
                    exit(1);
                }
            }
            break;
          case '?':
            fprintf(stderr, "%s\r\n", error_message);
            exit(1);
            break;
        }
    }

    /* initialize synchronization options */
    if (sync) {
        switch (sync) {
          case 'm': /* mutex */
            add_func = mutexAdd;
            m_pthread_mutex_init(&mutex, NULL);
            break;
          case 's':
            add_func = spinLockAdd;
            break;
          case 'c':
            add_func = atomAdd;
            break;
        }
    }

    /* note the start time of program (using real_time) */
    struct timespec start_time;
    m_clock_gettime(CLOCK_REALTIME, &start_time);

    /* start some threads */
    pthread_t *tid = (pthread_t *)malloc(sizeof(pthread_t) * num_thr);
    long long i = 0;
    for (i = 0; i < num_thr; i++) {
        m_pthread_create(&tid[i], NULL, threadRoutine, NULL);
    }
    for (i = 0; i < num_thr; i++) {
        pthread_join(tid[i], NULL);
    }

    /* note end time */
    struct timespec end_time;
    m_clock_gettime(CLOCK_REALTIME, &end_time);

    /* output */
    long ops = num_thr*num_itr*2;
    long long run_time = end_time.tv_nsec - start_time.tv_nsec;
    run_time += (end_time.tv_sec - start_time.tv_sec) * 1000000000;
    if (opt_yield) {
        printf("add-yield-%s,%lld,%lld,%ld,%lld,%lld,%lld\n", sync_type, num_thr, num_itr, ops, 
              run_time, run_time / ops, counter);
    }
    else {
        printf("add-%s,%lld,%lld,%ld,%lld,%lld,%lld\n", sync_type, num_thr, num_itr, ops, 
              run_time, run_time / ops, counter);
    }

    free(tid);
}
