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
    printf("\b\bRéception du signal %d\nAu revoir !\n", signum);
    exit(EXIT_SUCCESS);
}

int main(int argc, char const *argv[])
{
    ushort port;

    socklen_t lg_addr_client;
    struct sockaddr_in addr_client;
    char buf_log[80];
    int nb;

    struct sigaction sa;

    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (sscanf(argv[1], "%hu", &port) != 1) {
        printf_err_exit("Le port doit être un entier entre 1 et 65000");
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
        lg_addr_client = sizeof(addr_client);
        if (accept(sock, (struct sockaddr *) &addr_client, &lg_addr_client) == -1) {
            if (errno == EINTR) continue;
            handle_error();
        }
        sprintf(buf_log, "Un client s'est connecté\t%s", inet_ntoa(addr_client.sin_addr));
        printf_info(buf_log);

        if (read(sock, &nb, sizeof(int)) != sizeof(int)) {
            sprintf(buf_log, "Problème lors de la réception\n");
            printf_warning(buf_log);
        }
    }

    close(sock);
    return 0;
}
