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
typedef unsigned int uint;

int display_any_address(const int family, const ushort port);
char mgetchar(char fin);
int boucle_send(int sock, void *buf, int lg, struct sockaddr_in *addrs, size_t len);
int listen_new_socket(int domain, int type, int protocol, ushort port, int nb_conn);

#endif
