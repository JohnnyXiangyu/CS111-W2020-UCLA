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
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <ctype.h>
#include <errno.h>
#include <openssl/ssl.h>

sig_atomic_t volatile on_flag = 1;
sig_atomic_t volatile run_flag = 1;

static char* error_message =
    "usage\n"
    "  --period=#       sample period (s)\n"
    "  --scale=C/F      use Celsius or Fahrenheit (default F)\n"
    "  --debug          activate debug mode\n"
    "  --log=FILENAME   [MANDATORY] log sensor reading into file\n"
    "  --host=HOSTNAME  [MANDATORY] log server host name\n"
    "  --id=ID          [MANDATORY] 9 digit id to send to server\n"
    "  PORTNUMBER       [MANDATORY] port number to connect to\n";

/* pin configurations */
const int senPin = 1;   /* sensor pin */
const int butPin = 60;  /* button pin */

mraa_gpio_context button;   /* global variable of button pin */
mraa_aio_context t_sensor;  /* global variable of sensor pin */

/* constants defined by groove temp sensor */
const int B = 4275;
const float R0 = 100000.0;

/* arguments */
char scale = 'F';               /* wise people use Celcius, default F */
int debug_flag = 0;             /* flag debug option, default 0 */
int period = 1;                 /* time between reading, default 1 */
int log_flag = 0;               /* whether to write into a file, default 0 */
static char* log_filename = ""; /* file name to write into */
FILE* log_file = NULL;          /* log target of fprintf */
static char* host_name = "";    /* host name given in arguments */
static char* id = "";           /* id given in arguments, must be 9 digits */
int port = -1;                  /* port number given in arguments */

/* socket structures */
struct hostent *host = NULL;    /* host server */
struct sockaddr_in host_addr;   /* socket address */
int sockfd = 0;                 /* socket file descriptor */

/* SSL structures */
SSL_CTX* context = NULL;         /* ssl contex */
SSL* ssl = NULL;                

/* buffering stdin reading */
char read_buf[1024];    /* buffer for reading stdin */
char swap_buf[1024];    /* buffer for temporarily store contents of read_buf */
int processed_byte = 0; /* indicating where did last search end, should always point to a \n */


/* declare my function wraps */
void printTime();
void printTimeAndTemp(float in_temp);
void finalize();
void intHandler();
float convertTemp(int a);
int parseReadBuf();
void m_write(char* new_str);
int m_read();
int m_socket(int __domain, int __type, int __protocol);
int m_connect(int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len);


/* separate function that prints formatted time */
void printTime(char* ext_str) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char time_str[100];
    sprintf(time_str, "%02d:%02d:%02d%s", tm.tm_hour, tm.tm_min, tm.tm_sec, ext_str);
    m_write(time_str);
    // dprintf(sockfd, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
    // if (log_flag) {
    //     fprintf(log_file, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
    // }
}


/* format and print a full-report string */
void printTimeAndTemp(float in_temp) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    // char time_str[200];
    // sprintf(time_str, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
    // m_write(time_str);
    in_temp *= 10;
    // char temp_str[100];

    char full_report[200];
    sprintf(full_report, "%02d:%02d:%02d %d.%d\n", 
        tm.tm_hour, 
        tm.tm_min, 
        tm.tm_sec, 
        (int) in_temp / 10, 
        (int) in_temp % 10
    );
    m_write(full_report);
}


/* wrap up function: only called after initializing pins */
void finalize() {
    mraa_gpio_close(button);
    mraa_aio_close(t_sensor);

    /* print final message */
    printTime(" SHUTDOWN\n");

    /* close open file */
    if (log_file != NULL) {
        fclose(log_file);
    }

    SSL_shutdown(ssl);
    SSL_free(ssl);
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
        int cur_len = strlen(&read_buf[i]);

        if (strcmp("SCALE=F", &read_buf[i]) == 0) {
            scale = 'F';
        }
        else if (strcmp("SCALE=C", &read_buf[i]) == 0) {
            scale = 'C';
        }
        else if (strcmp("STOP", &read_buf[i]) == 0) {
            run_flag = 0;
        }
        else if (strcmp("START", &read_buf[i]) == 0) {
            run_flag = 1;
        }
        else if (strcmp("OFF", &read_buf[i]) == 0) {
            on_flag = 0;
        }
        else if (cur_len >=7 && strncmp("PERIOD=", &read_buf[i], 7) == 0) {
            int new_period = atoi(&read_buf[i] + 7);
            period = new_period;
        }

        if (log_flag && log_file != NULL) {
            fprintf(log_file, "%s\n", &read_buf[i]);
        }
        if (debug_flag) {
            fprintf(stderr, "%s\n", &read_buf[i]);
        }

        i += cur_len + 1;
    }

    return i;
}


/* read input from stdin and append to read_buf */
int m_read() {
    int cur_bytes = strlen(read_buf);
    int avail_bytes = 1023 - cur_bytes;
    int new_bytes = SSL_read(ssl, &read_buf[cur_bytes], avail_bytes);
    if (debug_flag) { fprintf(stderr, "debug: read %d bytes\n", new_bytes); }

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

    if (debug_flag) { fprintf(stderr, "debug: on_flag: %d, run_flag: %d\n", on_flag, run_flag); }

    return cur_bytes;
}


/* write into log and server */
void m_write(char* new_str) {
    if (sockfd != 0) { /* send to server */
        char new_wrt[100];
        sprintf(new_wrt, "%s", new_str);
        SSL_write(ssl, new_wrt, strlen(new_wrt));
    }
    if (log_flag) { /* write in log */
        fprintf(log_file, "%s", new_str);
    }
    if (debug_flag) {
        fprintf(stderr, "%s", new_str);
    }
}


/* wrap socket() system call */
int m_socket(int __domain, int __type, int __protocol) {
    int m_sock = socket(__domain, __type, __protocol);
    if (m_sock < 0) {
        fprintf(stderr, "ERROR: socket() system call failed, exiting...\n");
        exit(2);
    }
    return m_sock;
}


/* wrap function for connect() call (really not necessary) */
int m_connect(int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len) {
    int rc = connect(__fd, __addr, __len);
    if (rc < 0) {
        fprintf(stderr, "ERROR: connect() call failed: %s\nexiting...\n", strerror(errno));
        exit(2);
    }
    else {
        return rc;
    }
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
        {"id", required_argument, 0, 'i'},
        {"host", required_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    int temp = 0;
    while ((temp = getopt_long(argc, argv, "-", options, NULL)) != -1) {
        switch (temp) {
          case 1:
            port = atoi(argv[optind - 1]);
            if (port < 0) {
                fprintf(stderr, "%s\r\n", error_message);
                exit(1);
            }
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
          case 'i':
            if (strlen(optarg) != 9) {
                fprintf(stderr, "%s\r\n", error_message);
                exit(1);
            }
            else {
                id = optarg;
            }
            break;
          case 'h':
            host_name = optarg;
            break;
          case '?':
            fprintf(stderr, "%s\r\n", error_message);
            exit(1);
            break;
        }
    }

    /* print out all received argument if debug is enabled */
    if (debug_flag) {
        fprintf(stderr, "scale: %c, period: %d, logfile: %s, host name: %s, id: %s, port: %d\n",
            scale,
            period,
            log_filename,
            host_name,
            id,
            port);
    }

    /* enforce id, host, and log */
    if (strcmp(id, "") == 0 || strcmp(host_name, "") == 0 || port == -1) {
        fprintf(stderr, "ERROR: required fields not specified\n%s\r\n", error_message);
        exit(1);
    }

    /* open log file */
    if (log_flag) {
        log_file = fopen(log_filename, "w+");
        if (log_file == NULL) {
            fprintf(stderr, "ERROR: opening file %s failed, exiting...\n", log_filename);
            exit(1);
        }
    }

    /* open socket */
    sockfd = m_socket(AF_INET, SOCK_STREAM, 0);
    if ((host = gethostbyname(host_name)) == NULL) {
        fprintf(stderr, "ERROR: host not found, exiting...\n");
    }
    bzero(&host_addr, sizeof(host_addr));
    host_addr.sin_family = AF_INET;
    // bcopy(&host_addr.sin_addr.s_addr, &(host->h_addr_list[0]), host->h_length);
    bcopy((char *)host->h_addr,
          (char *)&host_addr.sin_addr.s_addr,
          host->h_length);
    host_addr.sin_port = htons(port);
    m_connect(sockfd, (struct sockaddr *)&host_addr, sizeof(host_addr));

    /* initialize ssl structures */
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    if ((context = SSL_CTX_new(TLSv1_client_method())) == NULL) {
        fprintf(stderr, "SSL_CTX_new() return NULL, exiting...\n");
        exit(2);
    }
    if ((ssl = SSL_new(context)) == NULL) {
        fprintf(stderr, "SSL_new() return NULL, exiting...\n");
        exit(2);
    }
    if (!SSL_set_fd(ssl, sockfd)) {
        fprintf(stderr, "SSL_set_fd() return 0, exiting...\n");
        exit(2);
    }
    if (SSL_connect(ssl) != 1) {
        fprintf(stderr, "SSL_connect() return not 1, exiting...\n");
        exit(2);
    }

    /* before start, send and log id message */
    char initial_message[50];
    sprintf(&initial_message[0], "ID=%s\n", id);
    m_write(initial_message);

    /* prepare to poll() */
    struct pollfd fds;
    // fds.fd = STDIN_FILENO;
    fds.fd = sockfd;
    fds.events = POLLIN;

    button = mraa_gpio_init(butPin);    /* register a gpio context, for pin 60, name button */
    t_sensor = mraa_aio_init(senPin);   /* register a AIO context, for pin 1, name t_sensor */
    
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
            /* output report line */
            printTimeAndTemp(cur_temp);
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