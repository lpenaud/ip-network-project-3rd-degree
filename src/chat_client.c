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
#define BUFFER_MAX 80

#include "helpers.h"

int main(int argc, char const *argv[])
{
  int socketAccueil,socketChat,envoye,recu;

  struct sockaddr_in adresseClient;
  int lgadresseClient;
  struct hostent *hote;

  char description[BUFFER_MAX];
  long offreInitial;

  char out;

  char str[BUFFER_MAX];

  int port;

  system("clear");

  // Check les arguemnts
  if (argc != 3) {
    printf("Usage: %s <hostname> <port>\n", argv[0]);
    return EXIT_FAILURE;
  }
  if (sscanf(argv[2], "%d", &port) != 1) {
    printf("port is a number\n");
    return EXIT_FAILURE;
  }

  /* creation de la socket */
  if ((socketAccueil = socket(AF_INET,SOCK_DGRAM,0)) == -1)
  {
    perror("socket");
    exit(1);
  }


  /* recherche de l'@ IP de la machine distante */
  if ((hote = gethostbyname(argv[1])) == NULL)
  {
    perror("gethostbyname");
    close(socketAccueil);
    exit(2);
  }

  /* preparation de l'adresse locale : port + toutes les @ IP */
  adresseClient.sin_family = AF_INET;
  adresseClient.sin_port = htons(port);
  bcopy(hote->h_addr, &adresseClient.sin_addr, hote->h_length);


  /* attachement de la socket a` l'adresse locale */


  strcpy(str,"Hello");

  /* join accueil */
  lgadresseClient = sizeof(adresseClient);
  if ((envoye = sendto(socketAccueil,str,strlen(str)+1,0,(struct sockaddr *) &adresseClient, lgadresseClient)) != strlen(str)+1)
  {
    perror("sendto");
    close(socketAccueil);
    exit(1);
  }


  /* Reception des infor de la vente */
  lgadresseClient = sizeof(adresseClient);
  recu = recvfrom(socketAccueil,description,BUFFER_MAX,0,(struct sockaddr *) &adresseClient, &lgadresseClient);
  recu = recvfrom(socketAccueil,&offreInitial,sizeof(long),0,(struct sockaddr *) &adresseClient, &lgadresseClient);
  fflush(stdout);
  fflush(stdin);
  system("clear");
  printf("Informations vente :\n");
  printf("Descritpion : %s \n", description);
  printf("Descritpion : %lu \n", offreInitial);
  printf("Nouvelle offre de prix :\n");
  scanf("%ld",&offreInitial);



  while(1){

    if ((envoye = sendto(socketAccueil,&offreInitial,sizeof(long),0,(struct sockaddr *) &adresseClient, lgadresseClient)) != sizeof(long))
    {
      perror("sendto");
      close(socketAccueil);
      exit(1);
    }
    lgadresseClient = sizeof(adresseClient);
    recu = recvfrom(socketAccueil, &offreInitial, sizeof(long), 0, (struct sockaddr *) &adresseClient, &lgadresseClient);
    printf("Nouvelle offre de prix :\n");
    scanf("%ld",&offreInitial);


  }

  close(socketAccueil);
}
