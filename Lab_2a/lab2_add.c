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
char sync = 0;

/* option error message */
static char *error_message =
    "usage\n  --threads=# number of threads to run\n"
    "  --iterations=# number of iterations to run\n"
    "  --debug activate debug mode";


int m_clock_gettime(clockid_t id, struct timespec *tp) {
    int rc = 0;
    rc = clock_gettime(id, tp);
    if (rc != 0) {
        fprintf(stderr, "clock_gettime() return non-zero, exiting ...\n");
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

void *unsafeRoutine(void *vargp) {
    long long i = (long long) vargp;
    for (i = 0; i < num_itr; i++) {
        add(&counter, 1);
    }
    for (i = 0; i < num_itr; i++) {
        add(&counter, -1);
    }
    pthread_exit(0);
}

void *mutexRoutine(void * vargp) {
    long long i = (long long) vargp;
    for (i = 0; i < num_itr; i++) {
        add(&counter, 1);
    }
    for (i = 0; i < num_itr; i++) {
        add(&counter, -1);
    }
    pthread_exit(0);
}


void *spinRoutine(void * vargp) {
    long long i = (long long) vargp;
    for (i = 0; i < num_itr; i++) {
        add(&counter, 1);
    }
    for (i = 0; i < num_itr; i++) {
        add(&counter, -1);
    }
    pthread_exit(0);
}


void *atomRoutine(void * vargp) {
    long long i = (long long) vargp;
    for (i = 0; i < num_itr; i++) {
        add(&counter, 1);
    }
    for (i = 0; i < num_itr; i++) {
        add(&counter, -1);
    }
    pthread_exit(0);
}


int main(int argc, char **argv) {
    static char* sync_type = "none";
    void* (*thread_routine)(void*) = unsafeRoutine;

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
                    fprintf(stderr, "unrecognized sync option provided, exiting ...\n");
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
          case 'm':
            thread_routine = mutexRoutine;
            break;
          case 's':
            thread_routine = spinRoutine;
            break;
          case 'c':
            thread_routine = atomRoutine;
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
        pthread_create(&tid[i], NULL, thread_routine, NULL);
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
