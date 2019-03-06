#ifndef HELPERS_H
#define HELPERS_H

#define TRUE        1
#define FALSE       0
#define BUF_MAX     256
#define ANSI_BOLD   "\x1b[1m"
#define ANSI_RED    "\x1b[31m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_CYAN   "\x1b[36m"
#define ANSI_RESET  "\x1b[0m"

#define printf_err_exit(msg) \
    do {\
        fprintf(stderr, ANSI_BOLD ANSI_RED "ERREUR" ANSI_RESET " - ""%s:%d - %s\n", __FILE__, __LINE__, msg); \
        exit(EXIT_FAILURE); \
    } while (0)
#define printf_warning(msg) \
    fprintf(stderr, ANSI_BOLD ANSI_YELLOW "ERREUR" ANSI_RESET " - ""%s:%d - %s\n", __FILE__, __LINE__, msg)
#define handle_error() printf_err_exit(strerror(errno))

typedef unsigned short ushort;
typedef unsigned int uint;

int display_hostname(void);
int display_ip_from_str(char *name);
int display_ip(void);
int display_any_address(const int family, const ushort port);
char mgetchar(char fin);
int boucle_send(int sock, void *buf, int lg, struct sockaddr_in *addrs, size_t len);
int listen_new_socket(int domain, int type, int protocol, ushort port, int nb_conn);

#endif
