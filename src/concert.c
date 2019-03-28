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
#include <sys/mman.h>


#include "helpers.h"

#define MAX_CONN 10

struct process {
    pid_t pid;
    int ticket, categorie, sock_client, timeout;
    char buf[BUF_SOCK], buf_log[BUF_LOG];
    char *host_places;
    ushort port_places;
    int *prices;
};

static void handler(int signum)
{
    fprintf(stderr, "Receiving the signal %d\n", signum);
}

void fork_exit(struct process *p, int interrupt)
{
    int len;
    int fd;
    if (interrupt != 0) {
        sprintf(p->buf_log, "[%d] Timeout", p->pid);
        len = sprintf(p->buf, "Timeout\n") + 1;
        write(p->sock_client, p->buf, len);
        if (p->ticket != -1) {
            fd = connect_new_socket(AF_INET, SOCK_STREAM, 0, p->host_places, p->port_places);
            if (fd == -1) {
                sprintf(p->buf_log, "[%d] %s", p->pid, strerror(errno));
                goto end;
            }
            len = sprintf(p->buf, "%d %d\n", p->categorie, p->ticket) + 1;
            write(fd, p->buf, len);
            close(fd);
        }
    }

end:
    close(p->sock_client);
}

int ask_ticket(struct process *process)
{
    int cat, nticket, sticket, rticket, tticket, len, len_write;

    const int sock_places = connect_new_socket(AF_INET, SOCK_STREAM, 0, process->host_places, process->port_places);

    if (sock_places < 0) {
        sprintf(process->buf_log, "[%d] %s", process->pid, strerror(errno));
        return -1;
    }

    do {
        process->ticket = -1;
        // Receive nb places from achat app
        alarm(process->timeout);
        if ((len = read(process->sock_client, process->buf, BUF_SOCK)) == -1) {
            if (errno == EINTR)
                fork_exit(process, 1);
            else
                sprintf(process->buf_log, "[%d] %s", process->pid, strerror(errno));
            return -1;
        }
        printf("JE SUIS LÀ\n");
        alarm(0);
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
        tticket = nticket + sticket;
        len = sprintf(process->buf, "%d %d\n", cat, -tticket) + 1;
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
        rticket = -rticket;
        len = sprintf(process->buf, "%d\n", rticket) + 1;
        if ((len_write = write(process->sock_client, process->buf, len)) != len) {
            if (len_write == -1)
                sprintf(process->buf_log, "[%d] %s", process->pid, strerror(errno));
             else
                sprintf(process->buf_log, "[%d] %d bytes were sent out of %d\n", process->pid, len_write, len);
            return -1;
        }
    } while(tticket != rticket);

    process->ticket = tticket;
    process->categorie = cat--;
    return process->prices[cat] * nticket + ((process->prices[cat] - (process->prices[cat] * 20 / 100)) * sticket);
}

int payment(struct process *process, const int price)
{
    int len, len_write;

    len = sprintf(process->buf, "%d\n", price) + 1;
    sprintf(process->buf_log, "[%d] I write : %s to the customer", process->pid, process->buf);
    printf_info(process->buf_log);
    if ((len_write = write(process->sock_client, process->buf, len)) != len) {
        if (len_write == -1)
            sprintf(process->buf_log, "[%d] %s", process->pid, strerror(errno));
         else
            sprintf(process->buf_log, "[%d] %d bytes were sent out of %d\n", process->pid, len_write, len);
        return -1;
    }



    alarm(process->timeout);
    if ((len = read(process->sock_client, process->buf, BUF_SOCK)) == -1) {
        if (errno == EINTR)
            fork_exit(process, 1);
        else
            sprintf(process->buf_log, "[%d] %s", process->pid, strerror(errno));
        return -1;
    }
    alarm(0);
    sprintf(process->buf_log, "[%d] I read %s", process->pid, process->buf);
    printf_info(process->buf_log);

    return 0;
}

int fork_job(struct process *process)
{
    int price;

    sprintf(process->buf_log, "[%d] I take care of the new client", process->pid);
    printf_info(process->buf_log);

    if ((price = ask_ticket(process)) == -1) {
        return -1;
    }
    sprintf(process->buf_log, "[%d] %d €", process->pid, price);
    printf_info(process->buf_log);
    sleep(1);

    if (payment(process, price) == -1) {
        return -1;
    }

    fork_exit(process, 0);
    return 0;
}

void change_prices(int *prices)
{
    int i, price;
    printf("Prix :\n");
    for (i = 0; i < CAT_MAX; i++) {
        printf("\tCatégorie n°%d : %d\n", i + 1, prices[i]);
    }
    printf("Saisir numéro de la catégorie : ");
    scanf("%d", &i);
    getchar();
    if (i < CAT_MIN || i > CAT_MAX) {
        printf("Choix non reconnu\n");
    }
    i--;
    printf("Saisir nouveau prix (%d) : ", prices[i]);
    if (scanf("%d", &price) != 1 || price < 0) {
        printf("Veuillez saisir un prix valide\n");
    }
    getchar();
    prices[i] = price;
}

int main(int argc, char *argv[])
{
    int port, sock;
    struct sockaddr_in addr_client;
    socklen_t addr_client_len;

    struct process process;
    int nbchild, st;

    struct sigaction sa;

    if (argc != 5) {
        printf("Usage: %s <port> <hostname places> <port places> <timeout>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    scanf_port(argv[1], port);
    if ((sock = listen_new_socket(AF_INET, SOCK_STREAM, 0, port, MAX_CONN)) < 0) {
        handle_error();
    }

    // Scan places port
    scanf_port(argv[3], port);
    process.host_places = argv[2];
    process.port_places = port;

    process.prices = mmap(
        NULL,
        sizeof(int) * CAT_MAX,
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS,
        -1,
        0
    );
    process.prices[0] = 50;
    process.prices[1] = 30;
    process.prices[2] = 20;

    if (sscanf(argv[4], "%d", &process.timeout) != 1 || process.timeout < 0) {
        sprintf(process.buf_log, "Timeout must be upper than 0");
        printf_err_exit(process.buf_log);
    }

    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_INTERRUPT;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        handle_error();
    }

    nbchild = 0;
    addr_client_len = sizeof(addr_client);
    port = atoi(argv[1]);

    if ((process.pid = fork()) == 0) {
        for (;;) {
            printf("Pour quitter saissez : 'q'\n");
            printf("Appuyer sur une touche pour changer les prix\n");
            process.buf[0] = getchar();
            if (process.buf[0] == -1 || process.buf[0] == 'q') {
                kill(getppid(), SIGINT);
                exit(EXIT_SUCCESS);
            }
            change_prices(process.prices);
            system("clear");
            display_any_address(AF_INET, port);
        }
    } else if (process.pid == -1) {
        handle_error();
    }

    for(;;) {
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
                if (sigaction(SIGALRM, &sa, NULL) == -1) {
                    handle_error();
                }
                process.pid = getpid();
                if (fork_job(&process) == -1)
                    printf_err_exit(process.buf_log);
                sprintf(process.buf_log, "[%d] Bye !", process.pid);
                printf_info(process.buf_log);
                exit(EXIT_SUCCESS);
            default:
                close(process.sock_client);
        }
    }

end:
    close(sock);
    printf("Appuyer sur 'q'\n");
    printf("Wait subprocess...\n");
    do {
        wait(&st);
    } while (errno != ECHILD);
    munmap(process.prices, sizeof(int) * CAT_MAX);
    printf("Bye !\n");
    exit(EXIT_SUCCESS);
}
