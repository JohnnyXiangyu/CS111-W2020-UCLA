#define _DEFAULT_SOURCE 10 // otherwise h_addr won't work

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/poll.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <strings.h>
#include <fcntl.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <zlib.h>

static char* error_message = "usage\n  --port=# client app to connect to localhost port#";
int rc = 0;
int server_closed = 0; // flag if the server is closed
int file_open = 0; // flag if file is open

int sockfd = -1; // open socket

FILE* log_file = NULL; // file pointer to log

int portno = 0; // port number
int p = 0; // mark --port
int l = 0; // flag need to log
static char* log_path = ""; // path to logfile
int c = 0; // flag need compression

z_stream from_server; // decompress stream
z_stream to_server; // compress stream


// check sys call return code
void checkRC(int alert, int is_syscall) { 
    if (rc == alert) {
        if (is_syscall) {
            fprintf(stderr, "ERROR: system call failure\r\n%s\r\n", strerror(errno));
        }
        else {
            fprintf(stderr, "ERROR: zlib failure\r\n");
        }
        if (file_open) {
            fclose(log_file);
        }
        exit(1);
    }
}


void sigHandler() {
    server_closed = 1;
}


void error(char *msg) {
    perror(msg);
    exit(1);
}


// initialize given z_stream struct
void zlib_init(z_stream* stream) {
    stream->zalloc = Z_NULL;
    stream->zfree = Z_NULL;
    stream->opaque = Z_NULL;
}


int main(int argc, char** argv) {

    // precess args using getopt
    struct option options[5] = {
        {"port", required_argument, 0, 'p'},
        {"log", required_argument, 0, 'l'},
        {"compress", no_argument, 0, 'c'},
        {0, 0, 0, 0}
    };
    int temp = 0;
    while ((temp = getopt_long(argc, argv, "-", options, NULL)) != -1) {
        switch (temp) {
          case 1:
            fprintf(stderr, "Bad non-option argument: %s\n%s\n", argv[optind-1], error_message);
            exit(1);
            break;
          case 'p': // --port=#
            p = 1; // marks --port given
            portno = atoi(optarg);
            break;
          case 'l': // --log=""
            l = 1;
            log_path = optarg;
            // log_file = fopen(log_path, "w+");
            // file_open = 1;
            break;
          case 'c': // --compress
            c = 1;
            // zlib_init(&from_server);
            // zlib_init(&to_server);
            // rc = inflateInit(&from_server);
            // if (rc != Z_OK) { checkRC(rc, 0); }
            // rc = deflateInit(&to_server, Z_DEFAULT_COMPRESSION);
            // if (rc != Z_OK) { checkRC(rc, 0); }
            break;
          case '?':
            fprintf(stderr, "%s\r\n", error_message);
            exit(1);
            break;
        }
    }

    if (!p) {
        fprintf(stderr, "--port=# OPTION IS REQUIRED!\n");
        exit(1);
    }

    if (c) {
        zlib_init(&from_server);
        zlib_init(&to_server);
        rc = inflateInit(&from_server);
        if (rc != Z_OK) { checkRC(rc, 0); }
        rc = deflateInit(&to_server, Z_DEFAULT_COMPRESSION);
        if (rc != Z_OK) { checkRC(rc, 0); }
    }

    if (l) {
        log_file = fopen(log_path, "w+");
        file_open = 1;
    }

    struct sockaddr_in serv_addr;
    struct hostent *server;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    server = gethostbyname("localhost");
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    // get current setting and create a modified new setting
    struct termios old_setting, new_setting;
    rc = tcgetattr(0, &old_setting);
    checkRC(-1, 1);
    new_setting = old_setting;
    new_setting.c_iflag = ISTRIP;
    new_setting.c_oflag = 0;
    new_setting.c_lflag = 0;
    // apply changes TODO: in the man page it says this call will return success 
        // whenever at least 1 change has been made, need to further check
    rc = tcsetattr(0, TCSANOW, &new_setting);
    checkRC(-1, 1);

    signal(SIGPIPE, sigHandler);

    // input
    char key_buf[256], sock_buf[256];
    char zlib_buf[1024]; // buffer for compression/decompression

    // prepare to poll()
    struct pollfd fds[2];
    fds[0].fd = 0;
    fds[1].fd = sockfd;
    fds[0].events = POLLIN;
    fds[1].events = POLLIN;

    // mark continue/termination
    int cont = 1; 
    while (cont && !server_closed) {
        int key_in = 1; int sock_in = 0;
        // poll stdin and sockfd
        key_in = 0;
        rc = poll(fds, 2, 0);
        checkRC(-1, 1);
        if (rc > 0) {
            key_in = fds[0].revents & POLLIN;
            sock_in = fds[1].revents & POLLIN;
            if (fds[1].revents & POLLERR || fds[1].revents & POLLHUP) { // socket close
                cont = 0;
            }
        }

        if (key_in) { // if can read keyboard
            char* write_buf = key_buf; // container of buffer to write from
            bzero(key_buf, 256);
            rc = read(0, key_buf, 256);
                checkRC(-1, 1);
            int count = rc;
            int i = 0;
            for (i = 0; i < count; i++) {
                if (key_buf[i] == '\r' || key_buf[i] == '\n') { // endl handling
                    rc = write(1, "\r\n", 2);
                        checkRC(-1, 1);
                }
                else if (key_buf[i] == 0x04) { // ^D
                    rc = write(1, "^D", 2);
                        checkRC(-1, 1);
                }
                else if (key_buf[i] == 0x03) { // ^C
                    rc = write(1, "^C", 2);
                        checkRC(-1, 1);
                }
                else { // normal character
                    rc = write(1, key_buf+i, 1);
                        checkRC(-1, 1);
                }
            }

            // compress
            if (c) {
                // logic here: client doesn't process any character, just send
                bzero(zlib_buf, 1024);
                to_server.avail_in = count;
                to_server.avail_out = 1024;
                to_server.next_in = (unsigned char*) key_buf;
                to_server.next_out = (unsigned char*) zlib_buf;
                while (to_server.avail_in > 0) {
                    deflate(&to_server, Z_SYNC_FLUSH);
                }
                count = 1024 - to_server.avail_out;
                write_buf = zlib_buf;
            }
            // write
            rc = write(sockfd, write_buf, count);
            if (l) {
                fprintf(log_file, "SENT %d bytes: %s\n", count, write_buf);
            }
        }
        if (sock_in) { // if can read socket
            char* read_buf = sock_buf; // container of right buffer to read
            bzero(sock_buf, 256);
            rc = read(fds[1].fd, sock_buf, 256);
                checkRC(-1, 1);
            int count = rc;
            if (count == 0) cont = 0;

            if (l) {
                fprintf(log_file, "RECEIVED %d bytes: %s\n", count, sock_buf);
            }
            
            // decompress
            if (c) {
                read_buf = zlib_buf;
                bzero(zlib_buf, 1024);
                from_server.avail_in = count;
                from_server.avail_out = 1024;
                from_server.next_in = (unsigned char*) sock_buf;
                from_server.next_out = (unsigned char*) zlib_buf;
                while (from_server.avail_in > 0) {
                    inflate(&from_server, Z_SYNC_FLUSH);
                }
                read_buf = zlib_buf;
                count = 1024 - from_server.avail_out;
            }
            // write
            int i;
            for (i = 0; i < count; i++) {
                if (read_buf[i] == '\n') {
                    rc = write(1, "\r\n", 2);
                    checkRC(-1, 1);
                }
                else {
                    rc = write(1, read_buf+i, 1);
                    checkRC(-1, 1);
                }
            }
        }
    }

    // restore changes
    rc = tcsetattr(0, TCSANOW, &old_setting);
        checkRC(-1, 1);

    //close file descriptors
    rc = close(sockfd);
        checkRC(-1, 1);
    if (l) {
        rc = fclose(log_file);
        if (rc < 0) { checkRC(rc, 1); }
    }
    if (c) {
        deflateEnd(&to_server);
        inflateEnd(&from_server);
    }
    return 0;
}
