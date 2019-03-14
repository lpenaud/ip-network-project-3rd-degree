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



#define MAX_LENGTH 1024
#define MAX_CLIENT 100

/* Select() params
 * int select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
 * FD_SET(int fd, fd_set *set);
 * FD_CLR(int fd, fd_set *set);
 * FD_ISSET(int fd, fd_set *set);
 * FD_ZERO(fd_set *set);
*/

int nbClient = 0;

void error(char *message)
{
    perror(message);
    exit(1);
}

int main(int argc, char **argv)
{
  fd_set readfds;
  struct timeval tv;
  int numfd;
	char *delim = "\n";

  int socket_fd, bytes_read;
  int lgadresseClient;
  char recieve_data[MAX_LENGTH],send_data[MAX_LENGTH];
  struct sockaddr_in server_address , client_address[MAX_CLIENT],client_address_temp;
	char *buf;

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

  if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
  {
      error("socket()");
  }

	int flags = fcntl(socket_fd, F_GETFL, 0);
	fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);


  // since we got s2 second, it's the "greater", so we use that for
  // the n param in select()
  numfd = socket_fd + 1;


  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);
  server_address.sin_addr.s_addr = INADDR_ANY;
  bzero(&(server_address.sin_zero),8);


  if (bind(socket_fd,(struct sockaddr *)&server_address, sizeof(struct sockaddr)) == -1)
  {
      error("bind()");
  }

  //lgadresseClient = sizeof(struct sockaddr);

  printf("\nUDP_Server Waiting for client to respond...\n");
  fflush(stdout);
  printf("Type (q or Q) at anytime to quit\n");


  while (1)
  {
		// clear the set ahead of time
		FD_ZERO(&readfds);
		fflush(stdout);
		fflush(stdin);
		// add our descriptors to the set (0 - stands for STDIN)
	  FD_SET(socket_fd, &readfds);
	  FD_SET(0, &readfds);
    choixClient = 0;


    int recieve = select(numfd, &readfds, NULL,/*NULL,*/ NULL, NULL);

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

        if (FD_ISSET(/*socket_fd*/0, &readfds)) //if set to write
        //else
        {

					FD_CLR(0, &readfds);
          read(STDIN_FILENO, send_data, MAX_LENGTH); //input the name with a size limit of MAX_LENGTH
          buf = strtok(send_data,delim);

          printf("Quel client : %d\n", (nbClient+1)/2);
          scanf("%d", &choixClient);
          choixClient--;
          if (choixClient == 0){
            choixClient++;
          }else{
            choixClient = choixClient*2;
          }

          lgadresseClient = sizeof(client_address[choixClient]);
        
	        if ((strcmp(buf , "q") == 0) || strcmp(buf , "Q") == 0) //if user quits, then send an invisible message to server to quit also
	        {
						sendto(socket_fd, buf, MAX_LENGTH, 0, (struct sockaddr *)&client_address[choixClient], lgadresseClient);
            break;
          }
          sendto(socket_fd, buf, MAX_LENGTH, 0,(struct sockaddr *)&client_address[choixClient], lgadresseClient);

        } else if (FD_ISSET(socket_fd, &readfds)) //if set to read
        {
          choixClient = -1;
          FD_CLR(socket_fd, &readfds);
          lgadresseClient = sizeof(client_address[nbClient]);
          bytes_read = recvfrom(socket_fd, recieve_data, MAX_LENGTH, 0, (struct sockaddr *)&client_address_temp, &lgadresseClient); //block call, will wait till client enters something, before proceeding
          recieve_data[bytes_read] = '\0'; //add null to the end of the buffer
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

          printf("\nNuméro client : %d \n(%s , %d) said: %s\n",(choixClient+1)/2,inet_ntoa(client_address[choixClient].sin_addr),ntohs(client_address[choixClient].sin_port),recieve_data);
        }
        else printf("\nOOPS! What happened? SERVER");
    		} //end else
  }//end while

  close (socket_fd);
  return 0;
}
