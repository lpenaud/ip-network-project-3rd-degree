#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include "helpers.h"

static int sock;

static void handler(int signum) {
    close(sock);
    fprintf(stderr, "Receiving the signal %d\n", signum);
    printf("Bye !\n");
    exit(EXIT_SUCCESS);
}

int main(int argc, char const *argv[])
{
    ushort port;

    struct sigaction sa;

    socklen_t addr_client_len;
    struct sockaddr_in addr_client;
    char buf_log[80], buf[20];

    int sock_client, len, len_write;
    int res;
    int tickets = 300;

    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (sscanf(argv[1], "%hu", &port) != 1) {
        printf_err_exit("The port must be a integer between 1 and 65535");
    }

    if ((sock = listen_new_socket(AF_INET, SOCK_STREAM, 0, port, 10)) < 0) {
        handle_error();
    }

    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    // Restart functions if interrupted by handler
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        handle_error();
    }

    while (1) {
        addr_client_len = sizeof(addr_client);
        if ((sock_client = accept(sock, (struct sockaddr *) &addr_client, &addr_client_len)) == -1) {
            if (errno == EINTR) continue;
            handle_error();
        }
        sprintf(buf_log, "%s has connected", inet_ntoa(addr_client.sin_addr));
        printf_info(buf_log);

        if ((len = read(sock_client, buf, 20)) == -1) {
            handle_error();
        }
        if (sscanf(buf, "%d", &res) != 1) {
            sprintf(buf_log, "I received : \"%s\", I ignore", buf);
            printf_warning(buf_log);
            goto loop_end;
        }
        if (res > 0) {
            tickets += res;
        } else {
            if (tickets == 0) {
                res = 0;
            } else if ((tickets + res) < 0) {
                res = -tickets;
            } else {
                tickets += res;
            }
        }
        sprintf(buf_log, "I received : %d, number of ticket %d", res, tickets);
        printf_info(buf_log);

        len = sprintf(buf, "%d", res) + 1;
        if ((len_write = write(sock_client, buf, len)) != len) {
            if (len_write == -1) {
                sprintf(buf_log, "%d bytes were sent out of %d", len, len_write);
                printf_warning(buf_log);
            } else {
                printf_warning(strerror(errno));
            }
        }

loop_end:
        close(sock_client);
    }

    return 0;
}
