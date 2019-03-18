#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // gethostname
#include <strings.h>    // bcopy
#include <arpa/inet.h>  // inet_ntoa
#include <ifaddrs.h>
#include <netdb.h>
#include <errno.h>

#include "helpers.h"

// Affiche les informations sur les cartes réseaux de famille AF_INET(6)
int display_any_address(const int family, const ushort port)
{
    struct ifaddrs *ifaddr, *ifa;
    char addr[NI_MAXHOST];
    int sa_family, s = 0;

    if (family != AF_INET && family != AF_INET6) {
        printf("Family accepted: AF_INET or AF_INET6\n");
        return -1;
    }

    // Obtention de la liste des cartes réseaux
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return errno;
    }

    printf("Server listening :\n");
    // On parcourt la liste des cartes réseaux
    for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;

        // Si la carte réseau est de la bonne famille
        if ((sa_family = ifa->ifa_addr->sa_family) == family) {
            // Obtention des informations de la carte
            s = getnameinfo(ifa->ifa_addr,
                sa_family == AF_INET
                    ? sizeof(struct sockaddr_in)
                    : sizeof(struct sockaddr_in6),
                addr,
                NI_MAXHOST,
                NULL,
                0,
                NI_NUMERICHOST
            );
            // Si l'obtention des informations a échoué
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                goto err;
            }
            /* Sinon on affiche les informations sur ce format :
                - On <nom>:     <adresse ip>:<port>
            */
            printf("\t- On %s:\t%s%s:%s%hu%s\n",
                ifa->ifa_name,
                ANSI_CYAN,
                addr,
                ANSI_BOLD,
                port,
                ANSI_RESET
            );
        }
    }

err:
    // On libère la mémoire des cartes réseaux
    freeifaddrs(ifaddr);
    return s;
}

// Permet d'éviter des bogues de buffer si on est dans une boucle
char mgetchar(char fin) {
    char c = fin, tmp;
    while ((tmp = getchar()) != fin) {
        c = tmp;
    }
    return c;
}

// Permet d'envoyer un buffer à plusieurs hôtes
int boucle_send(int sock, void *buf, int lg, struct sockaddr_in *addrs, size_t len)
{
    int etat;
    size_t i;

    for (i = 0; i < len; i++) {
        etat = sendto(
            sock,
            buf,
            lg,
            0,
            (struct sockaddr *) addrs + i,
            sizeof(addrs[i])
        );
        if (etat != lg) return -1;
    }
    return 0;
}

/*
    Créer une nouvelle socket
    l'attache sur un port donné et sur tout les cartes réseaux disponibles
    Fait écouter la socket pour un nombre de connexions donné

    Retourne le descripteur de fichier de la socket
*/
int listen_new_socket(int domain, int type, int protocol, ushort port, int nb_conn)
{
    int sock;
    struct sockaddr_in addrLocale;

    // Création de la socket
    if ((sock = socket(domain, type, protocol)) == -1)
        return -1;

    // Préparation de l'adresse locale
    addrLocale.sin_family = domain;
    addrLocale.sin_port = htons(port);
    addrLocale.sin_addr.s_addr = htonl(INADDR_ANY);
    // Attache la socket sur `port` port
    if (bind(sock, (struct sockaddr *) &addrLocale, sizeof(addrLocale)) == -1) {
        close(sock);
        return -2;
    }
    // La socket écoute pour `nb_conn` connexion
    if (listen(sock, nb_conn) != 0) {
        close(sock);
        return -3;
    }
    // On affiche les informations des cartes réseau compatible
    display_any_address(addrLocale.sin_family, port);

    // On retourne le descripteur de fichier
    return sock;
}

int find_index(int *array, size_t sz, int val)
{
    size_t i;
    for (i = 0; i < sz; i++) {
        if (array[i] == val) {
            return i;
        }
    }
    return -1;
}

int connect_new_socket(int domain, int type, int protocol, char *hostname, ushort port)
{
    int sock;
    struct sockaddr_in addr;
    struct hostent *hote;

    if ((hote = gethostbyname(hostname)) == NULL) {
        return -1;
    }

    if ((sock = socket(domain, type, protocol)) == -1) {
        return -2;
    }

    addr.sin_family = domain;
    addr.sin_port = htons(port);
    bcopy(hote->h_addr, &addr.sin_addr, hote->h_length);

    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        return -3;
    }

    return sock;
}
