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

/* Socket du serveur*/
static int sock;

/* Handler de SIGINT (Ctrl+C)*/
static void handler(int signum)
{
	/* On ferme la soket*/
	close(sock);

	/* On dit au revoir*/
	fprintf(stderr, "Receiving the signal %d\n", signum);
	printf("Bye !\n");

	/* On quitte le programme */
	exit(EXIT_SUCCESS);
}

int main(int argc, char const *argv[])
{
	int port;

	struct sigaction sa;

	socklen_t addr_server_len;
	struct sockaddr_in addr_server;
	char buf[BUF_SOCK];

	struct hostent *hote;

	unsigned int send;

	int len;

	/* informations sur les tickets envoyé à Concert */
	char *tickets = malloc(sizeof(int));



	/* Vérification nombre d'argument */
	if (argc != 3) {
		printf("Usage: %s <port> <serveur>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	/* Vérification si l'arguement est bien un entier entre 1 et 65535 */
	scanf_port(argv[1], port);



	/* Récupération de l'adresse IP du serveur (à partir du nom) */
	if ((hote = gethostbyname(argv[2])) == NULL) {
		handle_error();
	}

	/* Création de la socket serveur */
	if ((sock = socket(AF_INET, SOCK_STREAM,0)) == -1) {
		handle_error();
	}

	/* Préparation de l'adresse du serveur */
	addr_server.sin_family = AF_INET;
	addr_server.sin_port = htons(port);
	bcopy(hote->h_addr, &addr_server.sin_addr, hote->h_length);


	addr_server_len = sizeof(addr_server);
	/* Demande de connexion au serveur */
	if ((connect(sock, (struct sockaddr *) &addr_server, addr_server_len)) == -1) {
		handle_error();
	}

	/* On s'occupe du signal SIGINT (Ctrl+C) */
	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGINT, &sa, NULL) == -1) {
		handle_error();
	}



	/* Le serveur a accepté la connexion */
	printf("Connecté au site Concert\n");
	fflush(stdout);


	while(1){



		/* lecture des informations encoyé par l'application concert */
		if ((len = read(sock, buf, (sizeof(char)*BUF_SOCK))) == -1) {
			printf_warning(strerror(errno));
			goto loop_end;
		}
		printf("%s\n",buf);

		/* informations entré par le consommateur */
		scanf("%s", tickets);

		if ((send = write(sock, tickets, sizeof(int))) !=  sizeof(int))
		{
			/* Si la fonction a été interrompu par signal, on ignore l'erreur*/
			if (errno == EINTR) continue;
			/* Sinon l'erreur est plus grave on stop le programme*/
			handle_error();
		}


	}
loop_end:
	close(sock);

	return 0;

}
