#include <stdio.h>
#include <signal.h>
#include <math.h>
#include <getopt.h>
#include <string.h>
#include <time.h>
#include <mraa/gpio.h>
#include <mraa/aio.h>

sig_atomic_t volatile run_flag = 1;
int debug_flag = 0; /* flag debug option */

/* pin configurations */
const int senPin = 1; /* sensor pin */
const int butPin = 60; /* button pin */

/* constants defined by groove temp sensor */
const int B = 4275;
const float R0 = 100000.0;


/* configurable globals */
int sample_rate = 1; /* time between reading, default 1 */


void printTime() {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    printf("%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
}


/* handler for interruption (button) */
void intHandler() {
    run_flag = 0;
    printTime();
    printf(" OFF\n");
}

/* convert analog input to celcius temperature */
float convertToCelcius(int a) {
    float R = 1023.0/ (float) a -1.0;
    R = R0*R;
    float temperature = 1.0/(log(R/R0)/B+1/298.15)-273.15;

    return temperature;
}


int main(int argc, char **argv) {
    printf("this is the test version\n");


    /* precess args */
    // struct option options[7] = {
    //     {"period", required_argument, 0, 'p'},
    //     {"scale", required_argument, 0, 's'},
    //     {"log", required_argument, 0, 'l'},
    //     {"debug", no_argument, 0, 'd'},
    //     {0, 0, 0, 0}
    // };
    // int temp = 0;
    // while ((temp = getopt_long(argc, argv, "-", options, NULL)) != -1) {
    //     switch (temp) {
    //       case 1:
    //         fprintf(stderr, "Bad non-option argument: %s\n%s\n", argv[optind - 1], error_message);
    //         exit(1);
    //         break;
    //       case 't': // --threads=#
    //         num_thr = atoi(optarg);
    //         break;
    //       case 'i': // --iterations=#
    //         num_itr = atoi(optarg);
    //         break;
    //       case 'd': // --debug
    //         debug_flag = 1;
    //         break;
    //       case 'y':
    //         opt_yield = -1;
    //         yield_arg_str = optarg;
    //         break;
    //       case 's':
    //         if (strcmp("m", optarg) == 0) {
    //             sync = 'm';
    //             sync_type = "m";
    //         }
    //         else if (strcmp("s", optarg) == 0) {
    //             sync = 's';
    //             sync_type = "s";
    //         }
    //         else {
    //             fprintf(stderr, "ERROR: unrecognized sync option %s provided, exiting...\n", optarg);
    //             exit(1);
    //         }
    //         break;
    //       case 'l':
    //         num_lst = atoi(optarg);
    //         break;
    //       case '?':
    //         fprintf(stderr, "%s\r\n", error_message);
    //         exit(1);
    //         break;
    //     }
    // }

    /* register a gpio context, for pin 60, name button */
    mraa_gpio_context button;
    button = mraa_gpio_init(butPin);

    /* register a AIO context, for pin 1, name t_sensor */
    mraa_aio_context t_sensor;
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
        float cur_temp = convertToCelcius(analog_in);

        printTime();
        printf(" %d", (int) cur_temp);
        printf("\n");

        /* wait for some time */
        sleep(sample_rate);
    }

    /* close context */
    mraa_gpio_close(button);
    mraa_aio_close(t_sensor);

    return 0;
}