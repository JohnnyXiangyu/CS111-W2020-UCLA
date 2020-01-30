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
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#include <zlib.h>

static char* error_message = "usage\n  --port=# start server app with given port number indicated by #\n  --log=FILENAME logging communication into the designated filename\n";
static char* shell_path = "/bin/bash";

int rc = 0; // global: sys call return code
int shell_open = 1; // global flag if shell is open
int client_open = 1;

int sockfd, clilen; // socket file descriptors
int newsockfd = -1;

int from_closed = 0; // flag if read closed
int to_closed = 0; // flag if write closed
int pid = 0; // child process pid
int from_shell[2]; // read pipe
int to_shell[2]; // write pipe
int portno = 0; // port number
// int p = 0; // if port is given
int c = 0; // flag need to compress

z_stream from_client; // decompress stream
z_stream to_client; // compress stream

// check sys call return code
void checkRC(int alert, int is_syscall) { 
    if (rc == alert) {
        if (is_syscall) {
            fprintf(stderr, "ERROR: system call failure\r\n%s\r\n", strerror(errno));
        }
        else {
            fprintf(stderr, "ERROR: zlib failure\r\n");
        }
        
        if (newsockfd >= 0) {
            close(newsockfd);
        }
        exit(1);
    }
}


void error(char *msg)
{
    perror(msg);
    exit(1);
}


void sigHandler() {
    shell_open = 0;
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
        {"compress", no_argument, 0, 'c'},
        {0, 0, 0, 0}
    };
    int temp = 0;
    while ((temp = getopt_long(argc, argv, "-", options, NULL)) != -1) {
        switch (temp) {
          case 1: // not recognized
            fprintf(stderr, "Bad non-option argument: %s\n%s\n", argv[optind-1], error_message);
            exit(1);
            break;
          case 'p': // --port=#
            portno = atoi(optarg);
            // p = 1; // mark given
            break;
          case 'c': // --compression
            c = 1;
            zlib_init(&from_client);
            zlib_init(&to_client);
            rc = inflateInit(&from_client);
            if (rc != Z_OK) { checkRC(rc, 0); }
            rc = deflateInit(&to_client, Z_DEFAULT_COMPRESSION);
            if (rc != Z_OK) { checkRC(rc, 0); }
            break;
          case '?': // error
            fprintf(stderr, "%s\r\n", error_message);
            exit(1);
            break;
        }
    }

    // if (!p) {
    //     fprintf(stderr, "--port=# OPTION IS REQUIRED!\n");
    //     exit(1);
    // }

    // open child process
    rc = pipe(from_shell);
    checkRC(-1, 1);
    rc = pipe(to_shell);
    checkRC(-1, 1);
    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "ERROR: fork() return non-zero\r\n");
        exit(1);
    }
    if (pid == 0) { // child process
        // step 1: dup file descriptors and close some pipes
        rc = close(0); // stdin
            checkRC(-1, 1);
        rc = close(1); // stdout
            checkRC(-1, 1);
        rc = close(2); // stderr
            checkRC(-1, 1);
        rc = dup2(to_shell[0], 0);
            checkRC(-1, 1);
        rc = dup2(from_shell[1], 1);
            checkRC(-1, 1);
        rc = dup2(from_shell[1], 2);
            checkRC(-1, 1);
        rc = close(to_shell[1]); // close ends of the pipes not needed in child process
            checkRC(-1, 1);
        rc = close(from_shell[0]);
            checkRC(-1, 1);

        // step 2: open a shell
        execl(shell_path, "bash", (const char*)NULL);
        // only when execl() fails will these lines run
        fprintf(stderr, "ERROR: execl() failed\r\n");
        exit(1);
    }
    else { // close ends of pipes not needed in main process
        rc = close(from_shell[1]);
            checkRC(-1, 1);
        rc = close(to_shell[0]);
            checkRC(-1, 1);
    }

    signal(SIGPIPE, sigHandler);


    // IMPORTED CODE: creating the socket
    struct sockaddr_in serv_addr, cli_addr;
    // initialize socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *)&serv_addr,
             sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, (unsigned int*) &clilen);
    if (newsockfd < 0)
        error("ERROR on accept");


    // input buffer
    char sock_buf[256], shell_buf[256], zlib_buf[1024];

    // prepare to poll()
    struct pollfd polls[2];
    polls[0].fd = newsockfd; // socket fd
    polls[1].fd = from_shell[0];
    polls[0].events = POLLIN;
    polls[1].events = POLLIN;

    // poll routine
    int cont = 1; // mark continue/termination
    while (cont) {
        int sock_in = 0; int shell_in = 0;
        // poll stdin and from_shell[0]
        rc = poll(polls, 2, 0);
            checkRC(-1, 1);
        if (rc > 0) {
            sock_in = polls[0].revents & POLLIN;
            shell_in = polls[1].revents & POLLIN;

            if (polls[1].revents & POLLERR || polls[1].revents & POLLHUP) {
                shell_open = 0; // if shell closed
            }
        }

        if (sock_in) { 
            // read socket
            char* read_buf = sock_buf; // container for the right buffer to read
            bzero(sock_buf, 256);
            rc = read(newsockfd, sock_buf, 256);
                checkRC(-1, 1);
            int count = rc; // how many bytes read from socket
            if (count == 0) { client_open = 0; }

            if (c) {
                bzero(zlib_buf, 1024);
                from_client.avail_in = count;
                from_client.avail_out = 1024;
                from_client.next_in = (unsigned char*) sock_buf;
                from_client.next_out = (unsigned char*) zlib_buf;
                while (from_client.avail_in > 0) {
                    inflate(&from_client, Z_SYNC_FLUSH);
                }
                read_buf = zlib_buf;
                count = 1024 - from_client.avail_out;
            }

            int i = 0;
            for (i = 0; i < count; i++) {
                if (read_buf[i] == '\r' || read_buf[i] == '\n') { // endl handling
                    rc = write(to_shell[1], "\n", 1);
                        checkRC(-1, 1);
                }
                else if (read_buf[i] == 0x04) { // ^D
                    rc = close(to_shell[1]);
                        checkRC(-1, 1);
                    to_closed = 1;
                }
                else if (read_buf[i] == 0x03) { // ^C
                    rc = kill(pid, SIGINT);
                        checkRC(-1, 1);
                }
                else { // normal character
                    rc = write(to_shell[1], read_buf+i, 1);
                        checkRC(-1, 1);
                }
            }
        }
        if (shell_in) { // if can read from shell
            bzero(shell_buf, 256);
            rc = read(polls[1].fd, shell_buf, 256);
                checkRC(-1, 1);
            int count = rc; // how many read from shell

            int i = 0;
            for (i = 0; i < count; i++) {
                if (shell_buf[i] == 0x04) { // shell close
                    rc = close(from_shell[0]);
                        checkRC(-1, 1);
                    from_closed = 1;
                    if (c) {
                        count = i+1;
                        break;
                    }
                }
                else if (shell_buf[i] == '\n' && !c) {
                    rc = write(newsockfd, "\n", 1);
                        checkRC(-1, 1);
                }
                else if (!c) {
                    rc = write(newsockfd, shell_buf+i, 1);
                        checkRC(-1, 1);
                }
            }
            if (c) {
                bzero(zlib_buf, 1024);
                to_client.avail_in = count;
                to_client.avail_out = 1024;
                to_client.next_in = (unsigned char*) shell_buf;
                to_client.next_out = (unsigned char*) zlib_buf;
                while (to_client.avail_in > 0) {
                    deflate(&to_client, Z_SYNC_FLUSH);
                }
                count = 1024 - to_client.avail_out;
                write(newsockfd, zlib_buf, count);
            }
        }

        // close shell routine
        if (shell_open == 0 || client_open == 0) {
            // close remaining pipes
            if (!to_closed) { close(to_shell[1]); }
            if (!from_closed) { close(from_shell[0]); }
            // waitpid
            int status = 0;
            rc = waitpid(pid, &status, 0);
                checkRC(-1, 1);
            fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\r\n", status&0x007f, (status&0xff00)>>8);
            cont = 0;
        }
    }

    // TODO: close socket?
    rc = close(newsockfd);
        checkRC(-1, 1);
    if (c) {
        deflateEnd(&to_client);
        inflateEnd(&from_client);
    }
    return 0;
}
