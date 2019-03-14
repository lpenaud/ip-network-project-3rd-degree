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
#include <sys/select.h>//use select() for multiplexing
#include <sys/fcntl.h> // for non-blocking

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

void help(const char * hostname, int port){

	fd_set readfds;
	int numfd;
	char *delim = "\n";

	int socket_fd, bytes_recieved;
	unsigned int address_length;
	struct sockaddr_in server_address;
	struct hostent *host;
	char send_data[BUF_LOG],recieve_data[BUF_LOG];
	char *buf;



	if ((host = gethostbyname(hostname)) == NULL)
	{
		perror("gethostbyname");
		exit(2);
	}


	if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		handle_error();
	}

	int flags = fcntl(socket_fd, F_GETFL);
	flags |= O_NONBLOCK;
	fcntl(socket_fd, F_SETFL, flags);
	//fcntl(socket_fd, F_SETFL, O_NONBLOCK); //set socket to non-blocking




	// since we got s2 second, it's the "greater", so we use that for
	// the n param in select()
	numfd = socket_fd + 1;



	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	server_address.sin_addr = *((struct in_addr *)host->h_addr);
	bzero(&(server_address.sin_zero),8);

	address_length = sizeof(struct sockaddr);
	printf("Type (q or Q) at anytime to quit\n");

	while (1)
	{
		FD_ZERO(&readfds);
		fflush(stdout);
		fflush(stdin);
		// add our descriptors to the set (0 - stands for STDIN)
		FD_SET(socket_fd, &readfds);
		FD_SET(0, &readfds);

		int recieve = select(numfd, &readfds, NULL, NULL, NULL);
		if (recieve == -1)
		{
			perror("select"); // error occurred in select()
		}
		else if (recieve == 0)
		{
			printf("Timeout occurred!  No data after 10.5 seconds.\n");
		}
		else
		{
			// one or both of the descriptors have data
			if (FD_ISSET(socket_fd, &readfds)) //if set to read
			{
				FD_CLR(socket_fd, &readfds);//clear the set
				bytes_recieved = recvfrom(socket_fd,recieve_data, sizeof(recieve_data),0,(struct sockaddr *)&server_address,&address_length);
				recieve_data[bytes_recieved]= '\0';

				if((strcmp(recieve_data , "q") == 0) || (strcmp(recieve_data , "Q") == 0)) //if client quit, then quit also
				{
					printf("\nServer has exited the chat.\n");
					break;
				}

				printf("\n(%s , %d) said: %s\n",inet_ntoa(server_address.sin_addr),ntohs(server_address.sin_port),recieve_data);
				//inet_ntoa returns an ip address ipv4 style, ex: 127.0.0.1, and ntohs returns the port in the converted byte ordering
			}

			else if (FD_ISSET(0/*socket_fd*/, &readfds)) //if set to write
				//else
			{
				FD_CLR(0, &readfds);

				read(STDIN_FILENO, send_data, BUF_LOG); //input the name with a size limit of MAX_LENGTH
				buf =strtok(send_data,delim);


				if ((strcmp(buf , "q") == 0) || strcmp(buf , "Q") == 0) //if user quits, then send an invisible message to server to quit also
				{
					sendto(socket_fd, buf, BUF_LOG, 0, (struct sockaddr *)&server_address, sizeof(struct sockaddr));
					break;
				}

				else
				{
					sendto(socket_fd, buf, BUF_LOG, 0, (struct sockaddr *)&server_address, sizeof(struct sockaddr));
				}

			}
			else printf("\nOOPS! What happened? CLIENT");
		} //end else
	} // end while

	close (socket_fd);


}



int main(int argc, char const *argv[])
{
	int port;


	struct sigaction sa;

	socklen_t addr_server_len;
	struct sockaddr_in addr_server;
	char buf[BUF_SOCK], buf_log[80];

	struct hostent *hote;

	unsigned int send, size;

	int len, res = 0, nbaskticket = 1;

	/* informations sur les tickets envoyé à Concert */
	char *tickets = malloc(sizeof(int));

debut:

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
	port = 5000;


	while(res < nbaskticket){

		/*----------CATEGORIE------------*/
		printf("Categories des tickets :\n[1] [2] [3]\n");
		/* informations entré par le consommateur */
		scanf("%s", buf);

		if(strcmp(buf,"help") == 0){
			help(argv[2] ,port);
			goto debut;
		}

		if (sscanf(buf, "%d", &res) != 1) {
			sprintf(buf_log, "I received : \"%s\", I ignore", buf);
			printf_warning(buf_log);
		}
		if(res < 0 && res > 4){
			sprintf(buf_log, "I received : \"%s\", I ignore", buf);
			printf_warning(buf_log);
		}
		strcpy(tickets, buf);

		/*----------TICKET NORMAL------------*/
		printf("Nombre tickets normal:\n");
		scanf("%s", buf);

		if(strcmp(buf,"help") == 0){
			help(argv[2] ,port);
			goto debut;
		}

		if (sscanf(buf, "%d", &res) != 1) {
			sprintf(buf_log, "I received : \"%s\", I ignore", buf);
			printf_warning(buf_log);
		}
		if(res < 0 ){
			sprintf(buf_log, "I received : \"%s\", I ignore", buf);
			printf_warning(buf_log);
		}
		nbaskticket += res - 1;
		strcat(tickets, " ");
		strcat(tickets, buf);

		/*----------TICKET ETUDIANT------------*/
		printf("Nombre tickets etudiant:\n");
		scanf("%s", buf);

		if(strcmp(buf,"help") == 0){
			help(argv[2] ,port);
			goto debut;
		}

		if (sscanf(buf, "%d", &res) != 1) {
			sprintf(buf_log, "I received : \"%s\", I ignore", buf);
			printf_warning(buf_log);
		}
		if(res < 0 ){
			sprintf(buf_log, "I received : \"%s\", I ignore", buf);
			printf_warning(buf_log);
		}

		nbaskticket += res;
		strcat(tickets, " ");
		strcat(tickets, buf);


		size = (sizeof(int)*3)+(sizeof(char)*3);
		if ((send = write(sock, tickets, size)) !=  size)
		{
			/* Si la fonction a été interrompu par signal, on ignore l'erreur*/
			if (errno == EINTR) continue;
			/* Sinon l'erreur est plus grave on stop le programme*/
			handle_error();
		}


		/* lecture des informations encoyé par l'application concert */
		if ((len = read(sock, buf, (sizeof(char)*BUF_SOCK))) == -1) {
			printf_warning(strerror(errno));
			goto loop_end;
		}
		sscanf(buf, "%d", &res);
		printf("%d\n", res);
	}



	if ((len = read(sock, buf, (sizeof(char)*BUF_SOCK))) == -1) {
		printf_warning(strerror(errno));
	}

	printf("Montant transaction : %s\n", buf);
	printf("Numero carte:\n");

	scanf("%s", buf);
	if(strcmp(buf,"help") == 0){
		help(argv[2] ,port);
		goto debut;
	}

	if ((send = write(sock, buf, sizeof(int))) !=  sizeof(int))
	{
		handle_error();
	}

loop_end:
	close(sock);

	return 0;

}
