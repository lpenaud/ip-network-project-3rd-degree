#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // gethostname
#include <strings.h>    // bcopy
#include <arpa/inet.h>  // inet_ntoa
#include <ifaddrs.h>
#include <netdb.h>

#include "helpers.h"

int display_hostname(void)
{
    char name[BUF_MAX];
    if (gethostname(name, BUF_MAX) == -1) {
        return -1;
    }
    printf("hostname: %s\n", name);
    return 0;
}

int display_ip_from_str(char *name)
{
    struct hostent *h;
    struct in_addr och;

    if ((h = gethostbyname(name)) == NULL) {
        return -1;
    }

    bcopy(h->h_addr_list[0], &och, h->h_length);
    printf("address: %u\nadresse pointée %s\n", och.s_addr, inet_ntoa(och));

    return 0;
}

int display_ip(void)
{
    char name[BUF_MAX];

    if (gethostname(name, BUF_MAX) == -1) {
        return -1;
    }

    return display_ip_from_str(name);
}

int display_any_address(const int family, const ushort port)
{
    struct ifaddrs *ifaddr, *ifa;
    char addr[NI_MAXHOST];
    int sa_family, s = 0;

    if (family != AF_INET && family != AF_INET6) {
        printf("Family accepted: AF_INET or AF_INET6\n");
        return -1;
    }

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return -1;
    }

    printf("Server listening :\n");
    for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;

        if ((sa_family = ifa->ifa_addr->sa_family) == family) {
            /* Get IP adress */
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
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                goto err;
            }
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
    freeifaddrs(ifaddr);
    return s;
}

char mgetchar(char fin) {
    char c = fin, tmp;
    while ((tmp = getchar()) != fin) {
        c = tmp;
    }
    return c;
}

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

int listen_new_socket(int domain, int type, int protocol, ushort port, int nb_conn)
{
    int sock;
    struct sockaddr_in addrLocale;

    if ((sock = socket(domain, type, protocol)) == -1)
        return -1;

    addrLocale.sin_family = domain;
    addrLocale.sin_port = htons(port);
    addrLocale.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, (struct sockaddr *) &addrLocale, sizeof(addrLocale)) == -1) {
        close(sock);
        return -2;
    }
    if (listen(sock, nb_conn) != 0) {
        close(sock);
        return -3;
    }
    display_any_address(addrLocale.sin_family, port);

    return sock;
}
