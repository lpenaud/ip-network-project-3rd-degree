#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

#include "helpers.h"

static int sock;
static int *children, maxchild;

static void handler(int signum)
{
    int i, st;
    fprintf(stderr, "Receiving the signal %d\n", signum);

    // Close sockets
    close(sock);

    for (i = 0; i < maxchild; i++) {
        waitpid(children[i], &st, 0);
    }
    free(children);

    printf("Bye !\n");
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    int port;
    char buf_log[80], buf[BUF_SOCK];
    struct sockaddr_in addr_client;
    socklen_t addr_client_len;
    int sock_client, len, len_write;

    int nbchild, childst;
    int pid, ticket;
    int sock_places;

    struct sigaction sa;

    if (argc != 5) {
        printf("Usage: %s <port> <number of child process> <hostname places> <port places>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (sscanf(argv[2], "%d", &maxchild) != 1 || maxchild < 0 || maxchild > sysconf(_SC_CHILD_MAX)) {
        sprintf(buf_log, "The number of processes must be included between 0 and %ld", sysconf(_SC_CHILD_MAX));
        printf_err_exit(buf_log);
    }
    if ((children = (int *) malloc(sizeof(int) * maxchild)) == NULL)
        handle_error();

    for (nbchild = 0; nbchild < maxchild; nbchild++) {
        children[nbchild] = -1;
    }
    nbchild = 0;

    scanf_port(argv[1], port);
    if ((sock = listen_new_socket(AF_INET, SOCK_STREAM, 0, port, 10)) < 0) {
        handle_error();
    }

    scanf_port(argv[4], port);
    if ((sock_places = connect_new_socket(AF_INET, SOCK_STREAM, 0, argv[3], port)) < 0) {
        handle_error();
    }

    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
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

        if (nbchild + 1 > maxchild) {
            pid = wait(&childst);
            pid = find_index(children, maxchild, pid);
        } else {
            pid = nbchild++;
        }
        switch (children[pid] = fork()) {
            case -1:
                handle_error();
            case 0:
                pid = getpid();
                sprintf(buf_log, "[%d] I take care of the new client", pid);
                printf_info(buf_log);

                // Receive nb places
                if ((len = read(sock_client, buf, BUF_SOCK)) == -1) {
                    sprintf(buf_log, "[%d] %s", pid, strerror(errno));
                    close(sock_client);
                    printf_err_exit(buf_log);
                }
                if (sscanf(buf, "%d", &ticket) != 1 && ticket < 0) {
                    sprintf(buf_log, "[%d] I received : \"%s\", I exit", pid, buf);
                    write(sock_client, buf, len);
                    close(sock_client);
                    printf_err_exit(buf_log);
                }
                sprintf(buf_log, "[%d] I received %d", pid, ticket);
                len = sprintf(buf, "%d", -ticket) + 1;

                // Ask tickets
                sprintf(buf_log, "[%d] I write %s", pid, buf);
                printf_info(buf_log);
                if ((len_write = write(sock_places, buf, len)) != len) {
                    if (len_write == -1) {
                        printf_warning(strerror(errno));
                    }
                    sprintf(buf_log, "%d bytes were sent out of %d", len, len_write);
                    printf_warning(buf_log);
                    sprintf(buf, "0");
                }
                if ((len = read(sock_places, buf, BUF_SOCK)) == -1) {
                    printf_warning(strerror(errno));
                    len = sprintf(buf, "-1") + 1;
                }
                printf_info("UI");

                // Send tickets
                if ((len_write = write(sock_client, buf, len)) != len) {
                    if (len_write == -1) {
                        sprintf(buf_log, "[%d] %s", pid, strerror(errno));
                        printf_err_exit(buf_log);
                    }
                    sprintf(buf_log, "[%d] %d bytes were sent out of %d\n", pid, len_write, len);
                    printf_warning(buf_log);
                }
                close(sock_client);
                return 0;
            default:
                close(sock_client);
        }
    }

    return 0;
}
