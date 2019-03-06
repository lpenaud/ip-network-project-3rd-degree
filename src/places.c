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

// Socket du serveur
static int sock;

// Handler de SIGINT (Ctrl+C)
static void handler(int signum)
{
    // On ferme la soket
    close(sock);

    // On dit au revoir
    fprintf(stderr, "Receiving the signal %d\n", signum);
    printf("Bye !\n");

    // On quitte le programme
    exit(EXIT_SUCCESS);
}

int main(int argc, char const *argv[])
{
    int port;

    struct sigaction sa;

    socklen_t addr_client_len;
    struct sockaddr_in addr_client;
    char buf_log[80], buf[BUF_SOCK];

    int sock_client, len, len_write;
    int res;
    int tickets = 300;

    // Vérification nombre d'argument
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // Vérification si l'arguement est bien un entier entre 1 et 65535
    scanf_port(argv[1], port);
    // Création de la socket + attache + écoute
    if ((sock = listen_new_socket(AF_INET, SOCK_STREAM, 0, port, 10)) < 0) {
        handle_error();
    }

    // On s'occupe du signal SIGINT (Ctrl+C)
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        handle_error();
    }

    // Boucle de traitement
    while (1) {
        // En attente d'un client
        addr_client_len = sizeof(addr_client);
        if ((sock_client = accept(sock, (struct sockaddr *) &addr_client, &addr_client_len)) == -1) {
            // Si la fonction a été interrompu par signal, on ignore l'erreur
            if (errno == EINTR) continue;
            // Sinon l'erreur est plus grave on stop le programme
            handle_error();
        }
        // La connexion a été accepté
        sprintf(buf_log, "%s has connected", inet_ntoa(addr_client.sin_addr));
        printf_info(buf_log);

        // Lecture de la socket du client et vérification s'il n'y a pas d'erreur
        if ((len = read(sock_client, buf, 20)) == -1) {
            printf_warning(strerror(errno));
            goto loop_end;
        }
        // Scanne de la socket pour vérifier s'il y a bien un entier
        if (sscanf(buf, "%d", &res) != 1) {
            sprintf(buf_log, "I received : \"%s\", I ignore", buf);
            printf_warning(buf_log);
            goto loop_end;
        }
        // Si l'entier en positif, libération des billets
        if (res > 0) {
            tickets += res;
        } else {
            // On vérifie la disponibilité des billets
            if (tickets == 0) {
                // Il y a plus de billet
                // on renvoie 0
                // on ne touche pas le nombre de billet
                res = 0;
            } else if ((tickets + res) < 0) {
                // Il y a plus assez de billet
                // on renvoie le nombre de billet disponible
                // sans toucher au nombre de billet disponible
                res = -tickets;
            } else {
                // Il y a suffisament de billet
                // On soustraits le nombre de billet disponible
                tickets += res;
            }
        }
        sprintf(buf_log, "I received : %d, number of ticket %d", res, tickets);
        printf_info(buf_log);

        // On prépare le buffer pour l'envoie
        // Longeur de la chaine + caractère null
        len = sprintf(buf, "%d", res) + 1;
        if ((len_write = write(sock_client, buf, len)) != len) {
            // Erreur lors de l'envoie
            if (len_write == -1) {
                printf_warning(strerror(errno));
            } else {
                // On a pas tout envoyé
                sprintf(buf_log, "%d bytes were sent out of %d", len, len_write);
                printf_warning(buf_log);
            }
        }

loop_end:
        // On ferme la socket du client
        close(sock_client);
    }

    return 0;
}
