#include <stdio.h>
#include <signal.h>
#include <math.h>
#include <getopt.h>
#include <string.h>
#include <time.h>
#include <mraa/gpio.h>
#include <mraa/aio.h>
#include <stdlib.h>
#include <sys/poll.h>

sig_atomic_t volatile on_flag = 1;
sig_atomic_t volatile run_flag = 1;

static char* error_message =
    "usage\n  --period=# sample period (s)\n"
    "  --scale=C/F use Celsius or Fahrenheit (default F)\n"
    "  --log=FILENAME log sensor reading into file\n"
    "  --debug activate debug mode";

/* pin configurations */
const int senPin = 1; /* sensor pin */
const int butPin = 60; /* button pin */

mraa_gpio_context button; /* global variable of button pin */
mraa_aio_context t_sensor; /* global variable of sensor pin */

/* constants defined by groove temp sensor */
const int B = 4275;
const float R0 = 100000.0;

/* arguments */
char scale = 'F'; /* wise people use Celcius, default F */
int debug_flag = 0; /* flag debug option, default 0 */
int period = 1; /* time between reading, default 1 */
int log_flag = 0; /* whether to write into a file, default 0 */
static char* log_filename = ""; /* file name to write into */
FILE* log_file = NULL; /* log target of fprintf */

/* buffering stdin reading */
char read_buf[1024]; /* buffer for reading stdin */
char swap_buf[1024]; /* buffer for temporarily store contents of read_buf */
int processed_byte = 0; /* indicating where did last search end, should always point to a \n */


/* separate function that prints formatted time */
void printTime() {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    printf("%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
    if (log_flag) {
        fprintf(log_file, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
    }
}


/* separate function that prints formatted temperature INT.DEC */
void printTemp(float in_temp) {
    in_temp *= 10;
    printf("%d.%d", (int) in_temp / 10, (int) in_temp % 10);
    if (log_flag) {
        fprintf(log_file, "%d.%d", (int) in_temp / 10, (int) in_temp % 10);
    }
}


/* wrap up function: only called after initializing pins */
void finalize() {
    mraa_gpio_close(button);
    mraa_aio_close(t_sensor);

    /* print final message */
    printTime();
    printf(" SHUTDOWN\n");

    /* close open file */
    if (log_file != NULL) {
        fprintf(log_file, " SHUTDOWN\n");
        fclose(log_file);
    }
}


/* handler for interruption (button) */
void intHandler() {
    if (on_flag) {
        on_flag = 0;

        /* wrap and exit */
        finalize();
        exit(0);
    }
}


/* convert analog input to celcius temperature */
float convertTemp(int a) {
    float R = 1023.0/ (float) a -1.0;
    R = R0*R;
    float temperature = 1.0/(log(R/R0)/B+1/298.15)-273.15;

    if (scale == 'F') {
        temperature = (temperature * 9/5) + 32; 
    }

    return temperature;
}


/* parse read_buf and execute the commands */
int parseReadBuf() {
    if (debug_flag) { fprintf(stderr, "parsing\n"); }

    /* loop read_buffer and replace all newlines with zero-byte */
    int i = 0;
    int max = strlen(read_buf);
    processed_byte = 0;
    for (i = 0; i < max; i++) {
        if (read_buf[i] == '\n') {
            read_buf[i] = '\0';
            processed_byte = i;
        }
    }

    /* loop each segment of read_buf */
    i = 0;
    while (i < processed_byte) {
        if (debug_flag) { fprintf(stderr, "parse loop %d\n", i); }
        int cur_len = strlen(&read_buf[i]);

        if (strcmp("SCALE=F", &read_buf[i]) == 0) {
            scale = 'F';
            if (debug_flag) { fprintf(stderr, "F\n"); }
        }
        else if (strcmp("SCALE=C", &read_buf[i]) == 0) {
            scale = 'C';
            if (debug_flag) { fprintf(stderr, "C\n"); }
        }
        else if (strcmp("STOP", &read_buf[i]) == 0) {
            run_flag = 0;
            if (debug_flag) { fprintf(stderr, "SP\n"); }
        }
        else if (strcmp("START", &read_buf[i]) == 0) {
            run_flag = 1;
            if (debug_flag) { fprintf(stderr, "ST\n"); }
        }
        else if (strcmp("OFF", &read_buf[i]) == 0) {
            on_flag = 0;
            if (debug_flag) { fprintf(stderr, "O\n"); }
        }
        else if (cur_len >=7 && strncmp("PERIOD=", &read_buf[i], 7) == 0) {
            int new_period = atoi(&read_buf[i] + 7);
            period = new_period;
            if (debug_flag) { fprintf(stderr, "P\n"); }
        }

        if (log_flag && log_file != NULL) {
            fprintf(log_file, "%s\n", &read_buf[i]);
        }

        i += cur_len + 1;
    }

    return i;
}


/* read input from stdin and append to read_buf */
int m_read() {
    if (debug_flag) { fprintf(stderr, "reading\n"); }

    int cur_bytes = strlen(read_buf);
    int avail_bytes = 1023 - cur_bytes;
    int new_bytes = read(STDIN_FILENO, &read_buf[cur_bytes], avail_bytes);
    if (debug_flag) { fprintf(stderr, "read %d bytes\n", new_bytes); }

    if (new_bytes == -1) {
        fprintf(stderr, "ERROR: read() return -1, exiting...\n");
        finalize();
        exit(2);
    }
    else if (new_bytes > 0) {
        read_buf[cur_bytes + new_bytes] = '\0';
        parseReadBuf();
        cur_bytes = new_bytes - processed_byte - 1;
        // strcpy(swap_buf, &read_buf[processed_byte + 1]);
        strncpy(swap_buf, &read_buf[processed_byte+1], cur_bytes);
        swap_buf[cur_bytes] = '\0';
        strcpy(read_buf, swap_buf);
        read_buf[cur_bytes] = '\0';
    }

    if (debug_flag) { fprintf(stderr, "on_flag: %d, run_flag: %d\n", on_flag, run_flag); }

    return cur_bytes;
}


int main(int argc, char **argv) {
    /* initialize read_buf to empty string */
    read_buf[0] = '\0';

    /* precess args */
    struct option options[7] = {
        {"period", required_argument, 0, 'p'},
        {"scale", required_argument, 0, 's'},
        {"log", required_argument, 0, 'l'},
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
          case 'p':
            period = atoi(optarg);
            break;
          case 's':
            if (strcmp("F", optarg) == 0) {
                scale = 'F';
            }
            else if (strcmp("C", optarg) == 0) {
                scale = 'C';
            }
            else {
                fprintf(stderr, "unrecognized scale option: %s, exiting...", optarg);
                exit(1);
            }
            break;
          case 'l':
            log_flag = 1;
            log_filename = optarg;
            break;
          case 'd':
            debug_flag = 1;
            break;
          case '?':
            fprintf(stderr, "%s\r\n", error_message);
            exit(1);
            break;
        }
    }
    if (log_flag) {
        log_file = fopen(log_filename, "w+");
        if (log_file == NULL) {
            fprintf(stderr, "ERROR: opening file %s failed, exiting...\n", log_filename);
            exit(1);
        }
    }

    // prepare to poll()
    struct pollfd fds;
    fds.fd = STDIN_FILENO;
    fds.events = POLLIN;

    button = mraa_gpio_init(butPin); /* register a gpio context, for pin 60, name button */
    t_sensor = mraa_aio_init(senPin); /* register a AIO context, for pin 1, name t_sensor */
    
    mraa_gpio_dir(button, MRAA_GPIO_IN); /* configure buzzer to output pin */

    /* register a button handler */
    signal(SIGINT, intHandler); // should sigint be registered?
    mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &intHandler, NULL);

    while (on_flag) {
        if (run_flag) {
            /* read in analog input */
            int analog_in = mraa_aio_read(t_sensor);
            /* convert raw input to temperature C */
            float cur_temp = convertTemp(analog_in);

            printTime();
            printf(" ");
            if (log_flag) { fprintf(log_file, " "); }
            printTemp(cur_temp);
            printf("\n");
            if (log_flag) { fprintf(log_file, "\n"); }
        }

        /* read and execute stdin */
        int rc = poll(&fds, 1, 0);
        if (rc == -1) {
            fprintf(stderr, "ERROR: poll() failed, exiting...\n");
            finalize();
            exit(1);
        }
        else if (rc > 0) {
            if (fds.revents & POLL_IN) { /* if there's input from stdin */
                m_read();
            }
        }

        /* wait for some time */
        sleep(period);
    }

    /* close context */
    finalize();

    return 0;
}