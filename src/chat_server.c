/* receveur portReceveur */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <arpa/inet.h>
#include <string.h>

#define TRUE 1
#define MAX_CLIENT 100
#define BUFFER_MAX 80

#include "helpers.h"

int main(int argc, char const *argv[])
{
  int socketAccueil,recu,envoye,i;
	struct sockaddr_in adresseLocale, client, adresseClient[MAX_CLIENT];
	int lgadresseLocale, lgadresseClient, lgClient;
	int nbtour = 1;

	char description[BUFFER_MAX];
	long offreInitial;

	char c;

	int port;

	if (argc != 2) {
		printf("Usage: %s <port>\n", argv[0]);
		return EXIT_FAILURE;
	}
	if (sscanf(argv[1], "%d", &port) != 1) {
		printf("Tu rentres un nombre ? STP\n");
		return EXIT_FAILURE;
	}




	/* creation de la socket */
	if ((socketAccueil = socket(AF_INET,SOCK_DGRAM,0)) == -1)
	{
		perror("socket");
		exit(1);
	}

	/* preparation de l'adresse locale : port + toutes les @ IP */
	adresseLocale.sin_family = AF_INET;
	adresseLocale.sin_port = htons(port);
	adresseLocale.sin_addr.s_addr = htonl(INADDR_ANY);

	/* attachement de la socket a` l'adresse locale */
	lgadresseLocale = sizeof(adresseLocale);
	if (bind(socketAccueil, (struct sockaddr *) &adresseLocale, lgadresseLocale) == -1)
	{
		perror("bind");
		exit(1);
	}

	/* boucle accueil */
	c = 'o';
	while((c == 'o' || c == 'O')&& nbClient < MAX_CLIENT)
	{
		lgadresseClient = sizeof(adresseClient[nbClient]);
		recu = recvfrom(socketAccueil,description,BUFFER_MAX,0,(struct sockaddr *) adresseClient + nbClient, &lgadresseClient);
		nbClient++;
		printf("Nouveau client, on continue ? : [O/n]");
		if((c = mgetchar('\n')) == '\n'){
			c='o';
		}
		/*scanf("%c",out);
		  if(out == 10){
		  out = 'o';
		  }*/
		fflush(stdout);
		fflush(stdin);
	}

	/*lancement vente*/

	printf("Saisir la description de vente ?\n");
	scanf("%s",description);
	fflush(stdout);
	printf("Saisir prix initial :\n");
	scanf("%ld",&offreInitial);
	fflush(stdin);

	/* creation de la socket de vente */
	if ((socketVente = socket(AF_INET,SOCK_DGRAM,0)) == -1)
	{
		perror("socket");
		exit(1);
	}

	for(i = 0; i < nbClient; i++){
		lgadresseClient = sizeof(adresseClient[i]);
		if ((envoye = sendto(socketVente,description,strlen(description)+1,0,(struct sockaddr *) &adresseClient[i], lgadresseClient)) != strlen(description)+1)
		{
			perror("sendto descr");
			close(socketVente);
			exit(1);
		}
		lgadresseClient = sizeof(adresseClient[i]);
		if ((envoye = sendto(socketVente,&offreInitial,sizeof(long),0,(struct sockaddr *) &adresseClient[i], lgadresseClient)) != sizeof(long))
		{
			perror("sendto entier");
			close(socketVente);
			exit(1);
		}
	}

	c = 'o';

	fflush(stdout);
	fflush(stdin);
	system("clear");
	while((c == 'o' || c == 'O')&& nbClient < MAX_CLIENT){

		fflush(stdout);
		fflush(stdin);

		lgClient = sizeof(client);
		recu = recvfrom(socketVente,&offreInitial,sizeof(long),0,(struct sockaddr *) &client, &lgClient);
		printf("Nouveau prix : %lu, on continue ? : [O/n]\n", offreInitial);
		if(nbtour == 1)c=getchar();
		if((c = mgetchar('\n')) == '\n'){
			c='o';
		}

		nbtour++;

		fflush(stdout);
		fflush(stdin);

		for(i = 0; i < nbClient; i++){
			lgadresseClient = sizeof(adresseClient[i]);
			if ((envoye = sendto(socketVente,&offreInitial,sizeof(long),0,(struct sockaddr *) &adresseClient[i], lgadresseClient)) != sizeof(long))
			{
				perror("sendto entier");
				close(socketVente);
				exit(1);
			}
		}

	}
	close(socketAccueil);
}
