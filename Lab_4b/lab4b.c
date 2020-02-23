#include <stdio.h>
#include <signal.h>
#include <math.h>
#include <getopt.h>
#include <string.h>
#include <time.h>
#include <mraa/gpio.h>
#include <mraa/aio.h>
#include <stdlib.h>

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
void freePins() {
    mraa_gpio_close(button);
    mraa_aio_close(t_sensor);
}


/* handler for interruption (button) */
void intHandler() {
    if (run_flag) {
        run_flag = 0;
        printTime();
        printf(" OFF\n");
        if (log_flag) {
            fprintf(log_file, " OFF\n");
        }

        /* wrap and exit */
        freePins();
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


int main(int argc, char **argv) {
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
    }

    /* register a gpio context, for pin 60, name button */
    button = mraa_gpio_init(butPin);

    /* register a AIO context, for pin 1, name t_sensor */
    t_sensor = mraa_aio_init(senPin);
    
    /* configure buzzer to output pin */
    mraa_gpio_dir(button, MRAA_GPIO_IN);

    /* register a button handler */
    signal(SIGINT, intHandler);
    mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &intHandler, NULL);

    while (run_flag) {
        /* readin analog input */
        int analog_in = mraa_aio_read(t_sensor);
        /* convert raw input to temperature C */
        float cur_temp = convertTemp(analog_in);

        printTime();
        printf(" ");
        if (log_flag) { fprintf(log_file, " "); }
        printTemp(cur_temp);
        printf("\n");
        if (log_flag) { fprintf(log_file, "\n"); }

        /* wait for some time */
        sleep(period);
    }

    /* close context */
    freePins();

    return 0;
}