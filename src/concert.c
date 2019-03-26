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

static int *children, maxchild;

static void handler(int signum)
{
    fprintf(stderr, "Receiving the signal %d\n", signum);
}

int ask_ticket(const int sock_client, char buf[BUF_SOCK], char buf_log[BUF_LOG], int *categorie, int *ticket, char *hostname, const ushort port)
{
    const pid_t pid = getpid();
    const int prices[CAT_MAX] = { 50, 30, 20 };
    const int sock_places = connect_new_socket(AF_INET, SOCK_STREAM, 0, hostname, port);
    int cat, nticket, sticket, rticket, tticket, len, len_write;

    if (sock_places < 0) {
        sprintf(buf_log, "[%d] %s", pid, strerror(errno));
        return -1;
    }

    do {
        // Receive nb places from achat app
        if ((len = read(sock_client, buf, BUF_SOCK)) == -1) {
            sprintf(buf_log, "[%d] %s", pid, strerror(errno));
            return -1;
        }
        if (sscanf(buf, "%d %d %d", &cat, &nticket, &sticket) != 3
            || cat < CAT_MIN || cat > CAT_MAX
            || nticket < 0
            || sticket < 0
            || (nticket == 0 && sticket == 0)) {
            sprintf(buf_log, "[%d] I received : \"%s\", I exit", pid, buf);
            return -1;
        }
        sprintf(buf_log, "[%d] I received %s from achat app", pid, buf);
        printf_info(buf_log);

        // Ask for ticket to places app
        tticket = -(nticket + sticket);
        //TODO: Sent categorie
        len = sprintf(buf, "%d", tticket) + 1;
        if ((len_write = write(sock_places, buf, len)) != len) {
            if (len_write == -1)
                sprintf(buf_log, "[%d] %s", pid, strerror(errno));
            else
                sprintf(buf_log, "[%d] %d bytes were sent out of %d", pid, len, len_write);
            return -1;
        }
        sprintf(buf_log, "[%d] I send %s to places app", pid, buf);
        printf_info(buf_log);

        // Read response from places app
        if ((len = read(sock_places, buf, BUF_SOCK)) == -1) {
            sprintf(buf_log, "[%d] %s", pid, strerror(errno));
            return -1;
        }
        if (sscanf(buf, "%d", &rticket) != 1) {
            sprintf(buf_log, "[%d] I received \"%s\" from places app", pid, buf);
            return -1;
        }
        sprintf(buf_log, "[%d] I received %d from places app", pid, rticket);
        printf_info(buf_log);

        // Send tickets to achat app
        len = sprintf(buf, "%d", -(rticket)) + 1;
        if ((len_write = write(sock_client, buf, len)) != len) {
            if (len_write == -1)
                sprintf(buf_log, "[%d] %s", pid, strerror(errno));
             else
                sprintf(buf_log, "[%d] %d bytes were sent out of %d\n", pid, len_write, len);
            return -1;
        }
    } while(tticket != rticket);

    close(sock_places);
    *ticket = tticket;
    *categorie = cat--;
    return prices[cat] * nticket + ((prices[cat] - (prices[cat] * 20 / 100)) * sticket);
}

int fork_job(int sock_client, char buf[BUF_SOCK], char buf_log[BUF_LOG], char *hostname, const ushort port)
{
    const pid_t pid = getpid();
    int cat, ticket, price;

    sprintf(buf_log, "[%d] I take care of the new client", pid);
    printf_info(buf_log);

    if ((price = ask_ticket(sock_client, buf, buf_log, &cat, &ticket, hostname, port)) == -1) {
        return -1;
    }
    sprintf(buf_log, "[%d] %d €", pid, price);
    printf_info(buf_log);

    //TODO: payment

    close(sock_client);
    return 0;
}

int main(int argc, char *argv[])
{
    char buf_log[BUF_LOG], buf[BUF_SOCK];
    int port, sock;
    struct sockaddr_in addr_client;
    socklen_t addr_client_len;
    int sock_client;

    int nbchild, childst;
    int pid;

    struct sigaction sa;

    if (argc != 5) {
        printf("Usage: %s <port> <number of child process> <hostname places> <port places>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (sscanf(argv[2], "%d", &maxchild) != 1 || maxchild < 0 || maxchild > sysconf(_SC_CHILD_MAX)) {
        sprintf(buf_log, "The number of subprocesses must be included between 0 and %ld", sysconf(_SC_CHILD_MAX));
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

    // Scan places port
    scanf_port(argv[4], port);

    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_INTERRUPT;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        handle_error();
    }

    for(;;) {
        addr_client_len = sizeof(addr_client);
        if ((sock_client = accept(sock, (struct sockaddr *) &addr_client, &addr_client_len)) == -1) {
            if (errno == EINTR) goto end;
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
                if (fork_job(sock_client, buf, buf_log, argv[3], port) == -1) {
                    printf_err_exit(buf_log);
                }
                exit(EXIT_SUCCESS);
            default:
                close(sock_client);
        }
    }

end:
    close(sock);
    do {
        wait(&st);
    } while (errno != ECHILD);
    printf("Bye !");
    exit(EXIT_SUCCESS);
}
