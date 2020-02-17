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
long long num_lst = 1;    /* number of sub-lists */
long long num_elements = 0;

int debug_flag = 0;    /* flag debug mode */
int opt_yield;    /* yield options */
static char* yield_arg_str = "";
static char* yield_type = "none";
char sync = 0;
static char* sync_type = "none";

SortedList_t* head;    /* array of head nodes */
SortedListElement_t* elements;    /* all elements */
char** keys;    /* all keys (used for deletion) */
pthread_t* tid = NULL;    /* threads */

pthread_mutex_t* sub_mutexes = NULL; /* mutex for each sub list */
volatile int *sub_spin_locks = NULL; /* spin locks for each sub list */

long long* lock_ops_arr = NULL; /* array, # lock operation happened in each thread */
long long* lock_time_arr = NULL; /* array, nanoseconds spent on waiting mutex for each thread */

long long total_lock_ops = 0; /* int, count of lock ops for final report */
long long total_lock_time = 0; /* int, count of lock time for final report */

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
        fprintf(stderr, "ERROR: malloc() return NULL, exiting...\n");
        exit(1);
    }
    else
        return rc;
}

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

/* safe wrap of pthread_mutex_lock */
int m_pthread_mutex_lock(pthread_mutex_t *__mutex) {
    int rc = pthread_mutex_lock(__mutex);
    if (rc != 0) {
        fprintf(stderr, "ERROR: pthread_mutex_lock() returned %d, exiting ...\n", rc);
        exit(1);
    }
    else 
        return rc;
}

/* safe wrap of pthread_mutex_unlock */
int m_pthread_mutex_unlock(pthread_mutex_t *__mutex) {
    int rc = pthread_mutex_unlock(__mutex);
    if (rc != 0) {
        fprintf(stderr, "ERROR: pthread_mutex_unlock() returned %d, exiting ...\n", rc);
        exit(1);
    }
    else 
        return rc;
}

/* safe wrap of pthread_mutex_init */
int m_pthread_mutex_init(pthread_mutex_t *__mutex, const pthread_mutexattr_t *__mutexattr) {
    int rc = pthread_mutex_init(__mutex, __mutexattr);
    if (rc != 0) {
        fprintf(stderr, "ERROR: pthread_mutex_init() returned %d, exiting ...\n", rc);
        exit(1);
    }
    else 
        return rc;
}

/* free all allocated memory: please only call after everything is setup */
void* freeAll() {
    int i;
    for (i = 0; i < num_elements; i++) {
        free(keys[i]); /* keys */
    }
    free(head); /* heads' array */
    free(keys);
    free(elements); /* all nodes */
    free(tid); /* all threads */
    free(sub_mutexes); /* mutexes for subroutines */
    return 0;
}


/* segfault handler */
void memoryFucked(int arg) {
    fprintf(stderr, "f**k, there's segfault: %d\n", arg);
    freeAll();
    exit(0);
}


// /* debug mode only: print the whole list starting from head */
// void printList() {
//     SortedListElement_t* temp = &head;
//     do {
//         fprintf(stderr, "%ld\n  prev:%ld\n  next:%ld\n", (long)temp, (long)temp->prev, (long)temp->next);
//         temp = temp->next;
//     } while (temp != &head);
//     fprintf(stderr, "!\n");
// }


long long hashKey(const char* new_key) {
    if (new_key) {
        int new_hash = (int) new_key[0];
        new_hash = new_hash % num_lst;
        return new_hash;
    }
    else {
        return -1;
    }
}

long long hashNode(SortedList_t* new_node) {
    const char* new_key = new_node -> key;
    return hashKey(new_key);
}


/* thread routine 
   in total 2 locks */
void* threadRoutine(void* vargp) {
    long long my_id = (long long) vargp;
    long long start = my_id * num_itr;
    long long end = (my_id + 1) * num_itr;
    int i = 0;

    for (i = start; i < end; i++) {
        long long new_hash = hashNode(&(elements[i]));
        SortedList_t* this_head = &(head[new_hash]);

        /* lock up */
        if (sync) {
            /* start time */
            struct timespec before_lock;
            m_clock_gettime(CLOCK_REALTIME, &before_lock);

            if (sync == 'm')
                m_pthread_mutex_lock(&(sub_mutexes[new_hash]));
            if (sync == 's') {
                while (__sync_lock_test_and_set(&sub_spin_locks[new_hash], 1));
            }

            /* end time */
            struct timespec after_lock;        
            m_clock_gettime(CLOCK_REALTIME, &after_lock);
            /* calculate time elapsed */
            long long lock_time = after_lock.tv_nsec - before_lock.tv_nsec;
            lock_time += (after_lock.tv_sec - before_lock.tv_sec) * 1000000000;
            /* add this time to counter array */
            if (lock_time_arr && lock_ops_arr) { /* only if these 2 have been initialized */
                lock_time_arr[my_id] += lock_time;
                lock_ops_arr[my_id] += 1;
            }
        }

        SortedList_insert(this_head, &elements[i]);
        
        /* unlock */
        if (sync == 'm') 
            m_pthread_mutex_unlock(&(sub_mutexes[new_hash]));
        if (sync == 's')
            __sync_lock_release(&sub_spin_locks[new_hash]);
    }
    // if (debug_flag) { printList(); } 
    if (SortedList_length(head) == -1) {
        fprintf(stderr, "ERROR: SortedList_length() return -1, exiting...\n");
        exit(2);
    }

    for (i = start; i < end; i++) {
        long long old_hash = hashKey(keys[i]);
        SortedListElement_t* temp = SortedList_lookup(&(head[old_hash]), keys[i]);

        /* lock up */
        if (sync) {
            /* start time */
            struct timespec before_lock;
            m_clock_gettime(CLOCK_REALTIME, &before_lock);

            if (sync == 'm')
                m_pthread_mutex_lock(&(sub_mutexes[old_hash]));
            if (sync == 's') {
                while (__sync_lock_test_and_set(&sub_spin_locks[old_hash], 1));
            }

            /* end time */
            struct timespec after_lock;        
            m_clock_gettime(CLOCK_REALTIME, &after_lock);
            /* calculate time elapsed */
            long long lock_time = after_lock.tv_nsec - before_lock.tv_nsec;
            lock_time += (after_lock.tv_sec - before_lock.tv_sec) * 1000000000;
            /* add this time to counter array */
            if (lock_time_arr && lock_ops_arr) { /* only if these 2 have been initialized */
                lock_time_arr[my_id] += lock_time;
                lock_ops_arr[my_id] += 1;
            }
        }

        if (SortedList_delete(temp)) {
            fprintf(stderr, "ERROR: SortedList_delete() return 1, exiting...\n");
            exit(2);
        }

        /* unlock */
        if (sync == 'm') 
            m_pthread_mutex_unlock(&(sub_mutexes[old_hash]));
        if (sync == 's')
            __sync_lock_release(&sub_spin_locks[old_hash]);
    }
    
    pthread_exit(0);
}


int main(int argc, char **argv) {
    long long i; /* variable names reserved for looping */
    opt_yield = 0;

    /* precess args */
    struct option options[7] = {
        {"threads", required_argument, 0, 't'},
        {"iterations", required_argument, 0, 'i'},
        {"yield", required_argument, 0, 'y'},
        {"debug", no_argument, 0, 'd'},
        {"sync", required_argument, 0, 's'},
        {"list", required_argument, 0, 'l'},
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
          case 's':
            if (strcmp("m", optarg) == 0) {
                sync = 'm';
                sync_type = "m";
            }
            else if (strcmp("s", optarg) == 0) {
                sync = 's';
                sync_type = "s";
            }
            else {
                fprintf(stderr, "ERROR: unrecognized sync option %s provided, exiting...\n", optarg);
                exit(1);
            }
            break;
          case 'l':
            num_lst = atoi(optarg);
            break;
          case '?':
            fprintf(stderr, "%s\r\n", error_message);
            exit(1);
            break;
        }
    }

    /* initialize mutex counter and mutex time counter */
    if (sync != 0) {
        lock_ops_arr = m_malloc(sizeof(long long) * num_thr);
        lock_time_arr = m_malloc(sizeof(long long) * num_thr);
        /* initial value */
        for (i = 0; i < num_thr; i++) {
            lock_ops_arr[i] = 0;
            lock_time_arr[i] = 0;
        }
    }
    /* initialize mutex */
    if (sync == 'm') {
        sub_mutexes = m_malloc(sizeof(pthread_mutex_t) * num_lst);
        for (i = 0; i < num_lst; i ++) {
            m_pthread_mutex_init(&(sub_mutexes[i]), NULL);
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
            i ++;
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

    /* initialize list and mutex array */
    head = m_malloc(sizeof(SortedList_t) * num_lst);
    for (i = 0; i < num_lst; i ++) {
        /* mutex */
        head[i].next = &(head[i]);
        head[i].prev = &(head[i]);
        head[i].key = NULL;
    }

    /* allocate all elements */
    elements = (SortedListElement_t*) m_malloc(sizeof(SortedListElement_t) * num_elements);
    keys = (char**) malloc(sizeof(char*) * num_elements);
    /* give each element a key */
    for (i = 0; i < num_elements; i++) {
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
        m_pthread_create(&(tid[i]), NULL, threadRoutine, (void*) i);
    }
    /* join and calculate final report of "time per lock"*/
    for (i = 0; i < num_thr; i++) {
        m_pthread_join(tid[i], NULL);
        if (lock_ops_arr && lock_time_arr) {
            total_lock_ops += lock_ops_arr[i];
            total_lock_time += lock_time_arr[i];
        }
    }

    /* take end time */
    struct timespec end_time;
    m_clock_gettime(CLOCK_REALTIME, &end_time);

    /* report */
    long long ops = num_thr * num_itr * 3;
    long long run_time = end_time.tv_nsec - start_time.tv_nsec;
    run_time += (end_time.tv_sec - start_time.tv_sec) * 1000000000;
    printf("list-%s-%s,%lld,%lld,%lld,%lld,%lld,%lld", yield_type, sync_type, 
            num_thr, num_itr, num_lst, ops, run_time, run_time / ops);
    /* time per lock */
    if (lock_ops_arr && lock_time_arr) {
        printf(",%lld\n", total_lock_time/total_lock_ops);
        /* free the 2 arrays here since 
           this is the last time checking for them */
        free(lock_time_arr);
        free(lock_ops_arr);
    }
    else {
        printf("\n");
    }

    if (debug_flag) {
        printf("total lock time: %lld, total lock ops: %lld\n", 
                total_lock_time, total_lock_ops);
    }

    /* delete routine for always-need-to-deletes */
    freeAll(); // I miss using delete
}
