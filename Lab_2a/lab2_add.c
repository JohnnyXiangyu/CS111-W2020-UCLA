#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <time.h>

/* muti-threading objective variable */
long long counter = 0;

/* global variables */
int num_thr = 1;    /* number of threads */
int num_itr = 1;    /* number of iterations */
int debug_flag = 0; /* flag debug mode */

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
    *pointer = sum;
}

void *threadRoutine(void *vargp) {
}

int main(int argc, char **argv) {
    // precess args using getopt
    struct option options[5] = {
        {"threads", required_argument, 0, 't'},
        {"iterations", required_argument, 0, 'i'},
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
            num_thr = optarg;
            break;
        case 'i': // --iterations=#
            num_itr = optarg;
            break;
        case 'd': // --debug
            debug_flag = 1;
            break;
        case '?':
            fprintf(stderr, "%s\r\n", error_message);
            exit(1);
            break;
        }
    }

    /* note the start time of program (using real_time) */
    struct timespec start_time;
    m_clock_gettime(CLOCK_REALTIME, &start_time);

    /* start some threads */
    pthread_t *tids = (pthread_t *)malloc(sizeof(pthread_t) * num_thr);
}
