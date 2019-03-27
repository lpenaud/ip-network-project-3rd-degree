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

#define MAX_CONN 10

struct process {
    pid_t pid;
    int ticket, categorie, sock_client;
    char buf[BUF_SOCK], buf_log[BUF_LOG];
};

static void handler(int signum)
{
    fprintf(stderr, "Receiving the signal %d\n", signum);
}

int ask_ticket(struct process *process, char *hostname, const ushort port)
{
    const int prices[CAT_MAX] = { 50, 30, 20 };
    const int sock_places = connect_new_socket(AF_INET, SOCK_STREAM, 0, hostname, port);
    int cat, nticket, sticket, rticket, tticket, len, len_write;

    if (sock_places < 0) {
        sprintf(process->buf_log, "[%d] %s", process->pid, strerror(errno));
        return -1;
    }

    do {
        // Receive nb places from achat app
        if ((len = read(process->sock_client, process->buf, BUF_SOCK)) == -1) {
            sprintf(process->buf_log, "[%d] %s", process->pid, strerror(errno));
            return -1;
        }
        if (sscanf(process->buf, "%d %d %d", &cat, &nticket, &sticket) != 3
            || cat < CAT_MIN || cat > CAT_MAX
            || nticket < 0
            || sticket < 0
            || (nticket == 0 && sticket == 0)) {
            sprintf(process->buf_log, "[%d] I received : \"%s\", I exit", process->pid, process->buf);
            return -1;
        }
        sprintf(process->buf_log, "[%d] I received %s from achat app", process->pid, process->buf);
        printf_info(process->buf_log);

        // Ask for ticket to places app
        tticket = -(nticket + sticket);
        len = sprintf(process->buf, "%d %d", cat, tticket) + 1;
        if ((len_write = write(sock_places, process->buf, len)) != len) {
            if (len_write == -1)
                sprintf(process->buf_log, "[%d] %s", process->pid, strerror(errno));
            else
                sprintf(process->buf_log, "[%d] %d bytes were sent out of %d", process->pid, len, len_write);
            return -1;
        }
        sprintf(process->buf_log, "[%d] I send %s to places app", process->pid, process->buf);
        printf_info(process->buf_log);

        // Read response from places app
        if ((len = read(sock_places, process->buf, BUF_SOCK)) == -1) {
            sprintf(process->buf_log, "[%d] %s", process->pid, strerror(errno));
            return -1;
        }
        if (sscanf(process->buf, "%d", &rticket) != 1) {
            sprintf(process->buf_log, "[%d] I received \"%s\" from places app", process->pid, process->buf);
            return -1;
        }
        sprintf(process->buf_log, "[%d] I received %d from places app", process->pid, rticket);
        printf_info(process->buf_log);

        // Send tickets to achat app
        len = sprintf(process->buf, "%d", -(rticket)) + 1;
        if ((len_write = write(process->sock_client, process->buf, len)) != len) {
            if (len_write == -1)
                sprintf(process->buf_log, "[%d] %s", process->pid, strerror(errno));
             else
                sprintf(process->buf_log, "[%d] %d bytes were sent out of %d\n", process->pid, len_write, len);
            return -1;
        }
    } while(tticket != rticket);

    close(sock_places);
    process->ticket = tticket;
    process->categorie = cat--;
    return prices[cat] * nticket + ((prices[cat] - (prices[cat] * 20 / 100)) * sticket);
}

int fork_job(struct process *process, char *hostname, const ushort port)
{
    int price;

    sprintf(process->buf_log, "[%d] I take care of the new client", process->pid);
    printf_info(process->buf_log);

    if ((price = ask_ticket(process, hostname, port)) == -1) {
        return -1;
    }
    sprintf(process->buf_log, "[%d] %d €", process->pid, price);
    printf_info(process->buf_log);

    //TODO: payment

    close(process->sock_client);
    return 0;
}

int main(int argc, char *argv[])
{
    int port, sock;
    struct sockaddr_in addr_client;
    socklen_t addr_client_len;

    struct process process;
    int nbchild, st;

    struct sigaction sa;

    if (argc != 4) {
        printf("Usage: %s <port> <hostname places> <port places>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    scanf_port(argv[1], port);
    if ((sock = listen_new_socket(AF_INET, SOCK_STREAM, 0, port, MAX_CONN)) < 0) {
        handle_error();
    }

    // Scan places port
    scanf_port(argv[3], port);

    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_INTERRUPT;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        handle_error();
    }

    nbchild = 0;
    for(;;) {
        addr_client_len = sizeof(addr_client);
        if ((process.sock_client = accept(sock, (struct sockaddr *) &addr_client, &addr_client_len)) == -1) {
            if (errno == EINTR) goto end;
            handle_error();
        }
        sprintf(process.buf_log, "%s has connected", inet_ntoa(addr_client.sin_addr));
        printf_info(process.buf_log);

        nbchild++;
        if (nbchild > MAX_CONN) {
            nbchild--;
        }
        switch (process.pid = fork()) {
            case -1:
                handle_error();
            case 0:
                process.pid = getpid();
                process.ticket = -1;
                process.categorie = -1;
                if (fork_job(&process, argv[2], port) == -1)
                    printf_err_exit(process.buf_log);
                exit(EXIT_SUCCESS);
            default:
                close(process.sock_client);
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
