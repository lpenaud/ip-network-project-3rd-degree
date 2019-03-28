//SERVER
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>//use select() for multiplexing
#include <sys/fcntl.h> // for non-blocking

#include "helpers.h"

#define MAX_LENGTH 1024
#define MAX_CLIENT 100

/* Select() params
 * int select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
 * FD_SET(int fd, fd_set *set);
 * FD_CLR(int fd, fd_set *set);
 * FD_ISSET(int fd, fd_set *set);
 * FD_ZERO(fd_set *set);
*/

/* nombre client permet de savoir a qui repondre*/
int nbClient = 0;

int main(int argc, char **argv)
{
  fd_set readfds;
  int numfd;
	char *delim = "\n";

  int socket_fd, bytes_read;
  socklen_t lgadresseClient;
  char recieve_data[MAX_LENGTH],send_data[MAX_LENGTH];
  struct sockaddr_in server_address , client_address[MAX_CLIENT],client_address_temp;
	char *buf;

  /* choix de l'operateur a qui repondre*/
  int choixClient, i;


  int port;

	if (argc != 2) {
		printf("Usage: %s <port>\n", argv[0]);
		return EXIT_FAILURE;
	}
	if (sscanf(argv[1], "%d", &port) != 1) {
		printf("Tu rentres un nombre ? STP\n");
		return EXIT_FAILURE;
	}

  /*creation socket local*/
  if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
  {
      handle_error();
  }

  /*Passage en mode read non blockant*/
	int flags = fcntl(socket_fd, F_GETFL, 0);
	fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);


  /* on utilise "le plus grand" descripteur et comme stdin vaut 0 on utilise
  le descripteur de la socket*/
  numfd = socket_fd + 1;


  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);
  server_address.sin_addr.s_addr = INADDR_ANY;
  bzero(&(server_address.sin_zero),8);

  /*accrochage de la socket*/
  if (bind(socket_fd,(struct sockaddr *)&server_address, sizeof(struct sockaddr)) == -1)
  {
      handle_error();
  }


  printf("\nUDP_Server Waiting for client to respond...\n");
  fflush(stdout);


  while (1)
  {
		// clear le descipteur
		FD_ZERO(&readfds);

		fflush(stdout);
		fflush(stdin);

		// ajout des descripteurs au set (0 - stands for STDIN)
	  FD_SET(socket_fd, &readfds);
	  FD_SET(0, &readfds);

    /*initialisation de choix client*/
    choixClient = 0;


    int recieve = select(numfd, &readfds, NULL,/*NULL,*/ NULL, NULL);

    if (recieve == -1)
    {
      handle_error();
    }
    else
    {

        // un ou les deux descripteur on des données
        if (FD_ISSET(0, &readfds)) //si stdin est "non vide"
        {


          read(STDIN_FILENO, send_data, MAX_LENGTH); /* on lit l'enntree courante
          donc la réponse de l'opérateur  */
          buf = strtok(send_data,delim);

          printf("Quel client : %d\n", (nbClient+2)/2);
          /* calcul car la comparaison de port plus bas est problématique
             donc des calcul sont obligatoir mais avec ça le client est client
             bien contacté
          */

          // choix du client a qui repondre
          scanf("%d", &choixClient);

          /* -- car on affiche le numéro +1*/
          choixClient--;

          /* Problème de comparaison */
          if (choixClient == 0){
            choixClient++;
          }else{
            choixClient = choixClient*2;
          }

          /* taille address */
          lgadresseClient = sizeof(client_address[choixClient]);

	        if ((strcmp(buf , "q") == 0) || strcmp(buf , "Q") == 0) // verification que l'utilisateur ne quitte pas
	        {
						sendto(socket_fd, buf, MAX_LENGTH, 0, (struct sockaddr *)&client_address[choixClient], lgadresseClient);
            break;
          }
          sendto(socket_fd, buf, MAX_LENGTH, 0,(struct sockaddr *)&client_address[choixClient], lgadresseClient);

        } else if (FD_ISSET(socket_fd, &readfds)) //lecture de la socket
        {
          choixClient = -1;
          FD_CLR(socket_fd, &readfds);
          lgadresseClient = sizeof(client_address[nbClient]);
          bytes_read = recvfrom(socket_fd, recieve_data, MAX_LENGTH, 0, (struct sockaddr *)&client_address_temp, &lgadresseClient);
          recieve_data[bytes_read] = '\0'; //ajouter \0 a la fin d'un buffer

          /* on regarde si le client n'est pas déjà connecté*/
          for(i = 0; i < nbClient; i++){
              if(client_address_temp.sin_port == client_address[i].sin_port){
                choixClient = i;
              }
          }
          if (choixClient == -1) {
            nbClient++;
            choixClient = nbClient;
            client_address[choixClient] = client_address_temp;
          }

          printf("\nNuméro client : %d \n(%s , %d) said: %s\n",(choixClient+2)/2,inet_ntoa(client_address[choixClient].sin_addr),ntohs(client_address[choixClient].sin_port),recieve_data);
        }
        else printf("\nOOPS! What happened? SERVER");
    		} //end else
  }//end while

  close (socket_fd);

  return 0;
}
