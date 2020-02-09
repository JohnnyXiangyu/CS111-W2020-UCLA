#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include "SortedList.h"


/* global variables */
long long num_thr = 1;    /* number of threads */
long long num_itr = 1;    /* number of iterations */
long long num_elements = 0;
int opt_yield;
int debug_flag = 0; /* flag debug mode */
static char* yield_arg_str = "";
static char* yield_type = "none";
static char* sync_type = "none";

SortedList_t head; /* head node */
SortedListElement_t* elements; /* all elements */
char** keys; /* all keys (used for deletion) */
pthread_t* tid = NULL; /* threads */

pthread_mutex_t mutex; /* shared mutex */
volatile int lock = 0; /* shared spin lock */

/* option error message */
static char *error_message =
    "usage\n  --threads=# number of threads to run\n"
    "  --iterations=# number of iterations to run\n"
    "  --debug activate debug mode";

/* safe wrap of clock_gettime */
void* m_malloc(size_t __size) {
    void* rc = 0;
    rc = malloc(__size);
    if (rc == NULL) {
        fprintf(stderr, "ERROR: malloc() return NULL, exiting ...\n");
        exit(1);
    }
    else
        return rc;
}

int m_clock_gettime(clockid_t id, struct timespec *tp) {
    int rc = 0;
    rc = clock_gettime(id, tp);
    if (rc != 0) {
        fprintf(stderr, "ERROR: clock_gettime() return %d, exiting ...\n", rc);
        exit(1);
    }
    else
        return rc;
}

// /* safe wrap of pthread_mutex_init */
// int m_pthread_mutex_init(pthread_mutex_t *__mutex, const pthread_mutexattr_t *__mutexattr) {
//     int rc = pthread_mutex_init(__mutex, __mutexattr);
//     if (rc != 0) {
//         fprintf(stderr, "ERROR: pthread_mutex_init() returned %d, exiting ...\n", rc);
//         exit(1);
//     }
//     else 
//         return rc;
// }

/* safe wrap of pthread_create */
int m_pthread_create(pthread_t *__restrict__ __newthread, const pthread_attr_t *__restrict__ __attr, 
                     void *(*__start_routine)(void *), void *__restrict__ __arg) {
    int rc = pthread_create(__newthread, __attr, __start_routine, __arg);
    if (rc != 0) {
        fprintf(stderr, "ERROR: pthread_create() returned %d, exiting ...\n", rc);
        exit(1);
    }
    else 
        return rc;
}

/* safe wrap of pthread_join */
int m_pthread_join(pthread_t __th, void **__thread_return) {
    int rc = pthread_join(__th, __thread_return);
    if (rc != 0) {
        fprintf(stderr, "ERROR: pthread_join() returned %d, exiting ...\n", rc);
        exit(1);
    }
    else 
        return rc;
}

// /* safe wrap of pthread_mutex_lock */
// int m_pthread_mutex_lock(pthread_mutex_t *__mutex) {
//     int rc = pthread_mutex_lock(__mutex);
//     if (rc != 0) {
//         fprintf(stderr, "ERROR: pthread_mutex_lock() returned %d, exiting ...\n", rc);
//         exit(1);
//     }
//     else 
//         return rc;
// }

// /* safe wrap of pthread_mutex_unlock */
// int m_pthread_mutex_unlock(pthread_mutex_t *__mutex) {
//     int rc = pthread_mutex_unlock(__mutex);
//     if (rc != 0) {
//         fprintf(stderr, "ERROR: pthread_mutex_unlock() returned %d, exiting ...\n", rc);
//         exit(1);
//     }
//     else 
//         return rc;
// }


/* free all allocated memory: please only call after everything is setup */
void* m_free() {
    int i;
    for (i = 0; i < num_elements; i++) {
        free(keys[i]); /* keys */
    }
    free(keys);
    free(elements); /* all nodes */
    free(tid); /* all threads */
    return 0;
}


/* segfault handler */
void memoryFucked(int arg) {
    fprintf(stderr, "f**k, there's segfault: %d\n", arg);
    m_free();
    exit(0);
}


/* thread routine */
void* threadRoutine(void* vargp) {
    long long my_id = (long long) vargp;
    long long start = my_id * num_itr;
    long long end = (my_id + 1) * num_itr;

    //TODO: not compelete
    return NULL;
}


int main(int argc, char **argv) {
    long long i; /* variable names reserved for looping */
    opt_yield = 0;

    /* precess args */
    struct option options[5] = {
        {"threads", required_argument, 0, 't'},
        {"iterations", required_argument, 0, 'i'},
        {"yield", required_argument, 0, 'y'},
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
            opt_yield = -1;
            yield_arg_str = optarg;
            break;
          case '?':
            fprintf(stderr, "%s\r\n", error_message);
            exit(1);
            break;
        }
    }

    /* construct opt_yield bit mask */
    if (opt_yield) {
        opt_yield = 0;
        char temp = 0; i = 0;
        while (temp != '\0' || i == 0) {
            temp = ((char*) yield_arg_str)[i];
            switch (temp) {
                case 'i':
                opt_yield = opt_yield | INSERT_YIELD;
                break;
                case 'd':
                opt_yield = opt_yield | DELETE_YIELD;
                break;
                case 'l':
                opt_yield = opt_yield | LOOKUP_YIELD;
                break;
            }
        }
    }

    /* construct sync type and yield type */
    switch (opt_yield) {
      case (INSERT_YIELD):
        yield_type = "i";
        break;
      case (DELETE_YIELD):
        yield_type = "d";
        break;
      case (LOOKUP_YIELD):
        yield_type = "l";
        break;
      case (INSERT_YIELD | DELETE_YIELD):
        yield_type = "id";
        break;
      case (INSERT_YIELD | LOOKUP_YIELD):
        yield_type = "il";
        break;
      case (DELETE_YIELD | LOOKUP_YIELD):
        yield_type = "dl";
        break;
      case (INSERT_YIELD | DELETE_YIELD | LOOKUP_YIELD):
        yield_type = "idl";
        break;
    }

    num_elements = num_thr * num_itr;

    /* create empty list */
    head.next = &head;
    head.prev = &head;
    head.key = NULL;

    /* allocate all elements */
    elements = (SortedListElement_t*) m_malloc(sizeof(SortedListElement_t) * num_elements);
    keys = (char**) malloc(sizeof(char*) * num_elements);
    for (i = 0; i < num_elements; i++) {
        /* give each element a key */
        keys[i] = (char*) m_malloc(sizeof(char) * 256);
        int j;
        for (j = 0; j < 256; j++) {
            keys[i][j] = rand() % 94 + 33;
        }
        elements[i].key = keys[i];
    }

    /* deal with segfault */
    signal(SIGSEGV, memoryFucked);

    /* take start time */
    struct timespec start_time;
    m_clock_gettime(CLOCK_REALTIME, &start_time);

    /* start threads */
    tid = (pthread_t*) m_malloc(sizeof(pthread_t) * num_thr);
    /* create */
    for (i = 0; i < num_thr; i++) {
        m_pthread_create(&tid[i], NULL, threadRoutine, (void*) i);
    }
    /* join */
    for (i = 0; i < num_thr; i++) {
        m_pthread_join(tid[i], NULL);
    }

    /* take end time */
    struct timespec end_time;
    m_clock_gettime(CLOCK_REALTIME, &end_time);

    /* report */ // TODO
    long long ops = num_thr * num_itr * 3;
    long long run_time = end_time.tv_nsec - start_time.tv_nsec;
    run_time += (end_time.tv_sec - start_time.tv_sec) * 1000000000;
    printf("list-%s-%s,%lld,%lld,1,%lld,%lld,%lld\n", yield_type, sync_type, num_thr, num_itr, 
            ops, run_time, run_time / ops);

    /* delete routine */
    m_free(); // I miss using delete
}
