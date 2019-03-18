#ifndef HELPERS_H
#define HELPERS_H

#define BUF_SOCK    20
#define ANSI_RESET  "\x1b[0m"
#define ANSI_BOLD   "\x1b[1m"
#define ANSI_RED    "\x1b[31m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_CYAN   "\x1b[36m"
#define ANSI_BLUE   "\x1b[94m"

#define printf_err_exit(msg) \
    do {\
        fprintf(stderr, ANSI_BOLD ANSI_RED "ERREUR" ANSI_RESET " - ""%s:%d - %s\n", __FILE__, __LINE__, msg); \
        exit(EXIT_FAILURE); \
    } while (0)
#define printf_warning(msg) \
    fprintf(stderr, ANSI_BOLD ANSI_YELLOW "WARNING" ANSI_RESET " - ""%s:%d - %s\n", __FILE__, __LINE__, msg)
#define handle_error() printf_err_exit(strerror(errno))
#define printf_info(msg) \
    fprintf(stderr, ANSI_BOLD ANSI_BLUE "INFO" ANSI_RESET " - %s\n", msg)
#define scanf_port(arg, p) \
    if (sscanf(arg, "%d", &p) != 1 || p < 1 || p > 65535) \
        printf_err_exit("The port must be a integer between 1 and 65535")

typedef unsigned short ushort;

/*
    Affiche les informations sur les cartes réseaux de famille AF_INET(6)
    Retourne :
        - 0 En cas de succès
        - 1 Si la famille n'est pas supportée
        - Un nombre négatif correspondant à une erreur (voir getnameinfo et getifaddrs)
*/
int display_any_address(const int family, const ushort port);

/*
    Arguments
        - char fin  - caractère de fin de saisi
    Retourne :
        - le dernier caractère écrit avant le caractère de fin
        - Le caractère de fin si l'utilisateur a directement écrit celui-ci
*/
char mgetchar(char fin);

/*
    Envoie une socket avec la primitive sendto un buffer à plusieurs hôtes
    Arguments
        - int sock                  - descripteur de ficher de la socket
        - void *buf                 - buf a envoyer
        - int lg                    - longueur de buf
        - struct sockaddr_in *addrs - tableau d'adresses
        - size_t len                - nombre d'adresses
    Retroune
        -  0 en cas de succès
        - -1 en cas d'erreur
*/
int boucle_send(int sock, void *buf, int lg, struct sockaddr_in *addrs, size_t len);

/*
    Créer une nouvelle socket
    L'attache sur un port donné et sur tout les cartes réseaux disponibles
    Fait écouter la socket pour un nombre de connexions donné
    Arguments
        - int domain    - domaine de la socket exemple : AF_INET
        - int type      - type de la socket exemple : SOCK_STREAM
        - int protocol  - protocol de la socket exemple : 0
        - ushort port   - Numéro de port
        - int nb_conn   - Nombre de connexion maximale
*/
int listen_new_socket(int domain, int type, int protocol, ushort port, int nb_conn);

int find_index(int *array, size_t sz, int val);

int connect_new_socket(int domain, int type, int protocol, char *hostname, ushort port);

#endif
