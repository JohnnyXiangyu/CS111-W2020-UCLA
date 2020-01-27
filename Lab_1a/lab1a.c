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

static char* error_message = "usage\n  --shell open a bash child process and pipe message to/from it";
static char* shell_path = "";
int s = 0;
int from_shell[2];
int to_shell[2];
int rc = 0;
int pid = 0;
int shell_open = 1;

int from_closed = 0; // flag if pipes are closed
int to_closed = 0;


void checkRC(int alert) {
    // function that check return code
    if (rc == alert) {
        fprintf(stderr, "ERROR: system call failure\r\n");
        exit(1);
    }
}


void sigHandler() {
    shell_open = 0;
}


int main(int argc, char** argv) {
    // precess args using getopt
    struct option options[5] = {
        {"shell", no_argument, 0, 's'},
        {0, 0, 0, 0}
    };
    int temp = 0;
    while ((temp = getopt_long(argc, argv, "-", options, NULL)) != -1) {
        switch (temp) {
          case 1:
            fprintf(stderr, "Bad non-option argument: %s\n%s\n", argv[optind-1], error_message);
            exit(1);
            break;
          case 's':
            s = 1;
            break;
          case '?':
            fprintf(stderr, "%s\r\n", error_message);
            exit(1);
            break;
        }
    }

    shell_path = "/bin/bash"; // TODO: see how the announcement goes

    // react to options
    if (s) {
        rc = pipe(from_shell);
        checkRC(-1);
        rc = pipe(to_shell);
        checkRC(-1);
        pid = fork();
        if (pid < 0) {
            fprintf(stderr, "ERROR: fork() return non-zero\r\n");
            exit(1);
        }
        if (pid == 0) { // child process
            // step 1: dup file descriptors and close some pipes
            rc = close(0); // stdin
            checkRC(-1);
            rc = close(1); // stdout
            checkRC(-1);
            rc = close(2); // stderr
            checkRC(-1);
            rc = dup2(to_shell[0], 0);
            checkRC(-1);
            rc = dup2(from_shell[1], 1);
            checkRC(-1);
            rc = dup2(from_shell[1], 2);
            checkRC(-1);
            rc = close(to_shell[1]); // close ends of the pipes not needed in child process
            checkRC(-1);
            rc = close(from_shell[0]);
            checkRC(-1);

            // step 2: open a shell
            execl(shell_path, "bash", (const char*)NULL);
            // only when execl() fails will these lines run
            fprintf(stderr, "ERROR: execl() failed\r\n");
            exit(1);
        }
        else { // close ends of pipes not needed in main process
            rc = close(from_shell[1]);
            checkRC(-1);
            rc = close(to_shell[0]);
            checkRC(-1);
        }
    }

    signal(SIGPIPE, sigHandler);

    // get current setting and create a modified new setting
    struct termios old_setting, new_setting;
    rc = tcgetattr(0, &old_setting);
    checkRC(-1);
    new_setting = old_setting;
    new_setting.c_iflag = ISTRIP;
    new_setting.c_oflag = 0;
    new_setting.c_lflag = 0;
    // apply changes TODO: in the man page it says this call will return success 
        // whenever at least 1 change has been made, need to further check
    rc = tcsetattr(0, TCSANOW, &new_setting);
    checkRC(-1);


    // input
    char key_buf[256];
    char shell_buf[256];
    // prepare to poll()
    struct pollfd fds[2];
    fds[0].fd = 0;
    fds[1].fd = from_shell[0];
    fds[0].events = POLLIN;
    fds[1].events = POLLIN;

    // mark continue/termination
    int cont = 1; 
    while (cont) {
        int key_in = 1; int shell_in = 0;
        // poll stdin and from_shell[0]
        if (s) {
            key_in = 0;
            rc = poll(fds, 2, 0);
            checkRC(-1);
            if (rc > 0) {
                key_in = fds[0].revents & POLLIN;
                shell_in = fds[1].revents & POLLIN;
                if (fds[1].revents & POLLERR || fds[1].revents & POLLHUP) { // shell close
                    shell_open = 0;
                }
            }
        }

        if (key_in) { // if can read keyboard
            int count = read(0, key_buf, 256);
            int i = 0;
            for (i = 0; i < count; i++) {
                if (key_buf[i] == '\r' || key_buf[i] == '\n') { // endl handling
                    write(1, "\r\n", 2);
                    if (s) {
                        rc = write(to_shell[1], "\n", 1);
                        checkRC(-1);
                    }
                }
                else if (key_buf[i] == 0x04) { // ^D
                    if (!s) { cont = 0; } // if --shell is not give, just exit
                    else {
                        rc = close(to_shell[1]);
                        checkRC(-1);
                        to_closed = 1;
                        write(1, "^D", 2);
                    }
                }
                else if (key_buf[i] == 0x03 && s) { // ^C
                    rc = kill(pid, SIGINT);
                    checkRC(-1);
                    rc = write(1, "^C", 2);
                    checkRC(-1);
                }
                else { // normal character
                    rc = write(1, key_buf+i, 1);
                    checkRC(-1);
                    if (s) {
                        rc = write(to_shell[1], key_buf+i, 1);
                        checkRC(-1);
                    }
                }
            }
        }
        if (shell_in) { // if can read from shell
            int count = read(fds[1].fd, shell_buf, 256);
            int i = 0;
            for (i = 0; i < count; i++) {
                if (shell_buf[i] == '\n') {
                    rc = write(1, "\r\n", 2);
                    checkRC(-1);
                }
                else if (shell_buf[i] == 0x04) { // shell close
                    rc = close(from_shell[0]);
                    checkRC(-1);
                    from_closed = 1;
                }
                else {
                    rc = write(1, shell_buf+i, 1);
                    checkRC(-1);
                }
            }
        }

        // close shell routine
        if (s && shell_open == 0) {
            // close remaining pipes
            if (!to_closed) { close(to_shell[1]); }
            if (!from_closed) { close(from_shell[0]); }
            // waitpid
            int status = 0;
            rc = waitpid(pid, &status, 0);
            checkRC(-1);
            fprintf(stdout, "SHELL EXIT SIGNAL=%d STATUS=%d\r\n", status&0x007f, (status&0xff00)>>8);
            cont = 0;
        }
    }

    // restore changes & close child process
    rc = tcsetattr(0, TCSANOW, &old_setting);
    checkRC(-1);
    return 0;
}
