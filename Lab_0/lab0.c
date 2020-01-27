#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

// global variables that flag given arguments
unsigned char i = 0;
static char* in = "";
unsigned char o = 0;
static char* out = "";
unsigned char s = 0;
unsigned char c = 0;

static char* error_message = "usage\n  --input=filename ... use the specified file as standard input.\n  --output=filename ... create the specified file and use it as standard output.\n  --segfault ... force a segmentation fault.\n  --catch ... catch the segmentation fault.";

void makeSegFaut(void) {
    char* a = 0;
    printf("%c\n", *a);
}

void sigHandler(int arg) {
    fprintf(stderr, "segmentation fault caught: %d\n", arg);
    exit(4);
}

int main(int argc, char** argv) {
    // precess args
    struct option options[5] = {
        {"input", required_argument, 0, 'i'},
        {"output", required_argument, 0, 'o'},
        {"segfault", no_argument, 0, 's'},
        {"catch", no_argument, 0, 'c'},
        {0, 0, 0, 0}
    };

    // parse
    int temp = 0;
    while ((temp = getopt_long(argc, argv, "-", options, NULL)) != -1) {
        switch (temp) {
          case 1:
            fprintf(stderr, "Bad non-option argument: %s\n%s\n", argv[optind-1], error_message);
            exit(1);
            break;
          case 'i': 
            i = 1;
            in = optarg;
            break;
          case 'o':
            o = 1;
            out = optarg;
            break;
          case 's':
            s = 1;
            break;
          case 'c':
            c = 1;
            break;
          case '?':
            fprintf(stderr, "%s\n", error_message);
            exit(1);
            break;
        }
    }


    // step 1: redirect
    if (i) {
        int fd = open(in, O_RDONLY);
        if (fd == -1) {
            fprintf(stderr, "failed opening input file: %s\n", in);
            fprintf(stderr, "%s\n", strerror(errno));
            exit(2);
        }
        if (fd >= 0) {
            close(0);
            dup(fd);
            close(fd);
        }
    }
    if (o) {
        int fd = creat(out, 0666); // important: will not pass if 0666 not given
        if (fd == -1) {
            fprintf(stderr, "failed creating/opening output file: %s\n", out);
            fprintf(stderr, "%s\n", strerror(errno));
            exit(3);
        }
        if (fd >= 0) {
            close(1);
            dup(fd);
            close(fd);
        }
    }

    // step 2: register signal handler
    if (c)
        signal(SIGSEGV, sigHandler);

    //step 3: cause seg fault
    if (s)
        makeSegFaut(); 

    // step 4: copy
    // a while loop taking characters from given streams
    int temp_char = 0;
    int r = 0;
    while ((r = read(0, &temp_char, 1)) != 0) {
        if (r == -1) {
            break;
        }
        else if (r == 1) {
            write(1, &temp_char, 1);
        }
    }

    exit(0);
}