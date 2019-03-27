//CLIENT
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> //host struct
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>//use select() for multiplexing
#include <sys/fcntl.h> // for non-blocking

#include "helpers.h"

#define MAX_LENGTH 1024

void error(char *message)
{
    perror(message);
    exit(1);
}

int main(int argc, char **argv)
{
  fd_set original_socket;
  fd_set original_stdin;
  fd_set readfds;
  fd_set writefds;
  struct timeval tv;
  int numfd;
	char *delim = "\n";

  int socket_fd,bytes_recieved;
  unsigned int address_length;
  struct sockaddr_in server_address;
  struct hostent *host;
  char send_data[MAX_LENGTH],recieve_data[MAX_LENGTH];
	char *buf;

  int port;

  // Check les arguemnts
  if (argc != 3) {
    printf("Usage: %s <hostname> <port>\n", argv[0]);
    return EXIT_FAILURE;
  }
  if (sscanf(argv[2], "%d", &port) != 1) {
    printf("port is a number\n");
    return EXIT_FAILURE;
  }

  if ((host = gethostbyname(argv[1])) == NULL)
	{
		perror("gethostbyname");
		close(socketAccueil);
		exit(2);
	}


  if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
  {
    error("socket()");
  }

  int flags = fcntl(socket_fd, F_GETFL);
  flags |= O_NONBLOCK;
  fcntl(socket_fd, F_SETFL, flags);
  //fcntl(socket_fd, F_SETFL, O_NONBLOCK); //set socket to non-blocking




  // since we got s2 second, it's the "greater", so we use that for
  // the n param in select()
  numfd = socket_fd + 1;

  // wait until either socket has data ready to be recv()d (timeout 10.5 secs)
  tv.tv_sec = 10;
  tv.tv_usec = 500000;

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

				read(STDIN_FILENO, send_data, MAX_LENGTH); //input the name with a size limit of MAX_LENGTH
				buf =strtok(send_data,delim);


        if ((strcmp(buf , "q") == 0) || strcmp(buf , "Q") == 0) //if user quits, then send an invisible message to server to quit also
        {
          sendto(socket_fd, buf, MAX_LENGTH, 0, (struct sockaddr *)&server_address, sizeof(struct sockaddr));
          break;
        }

        else
        {
          sendto(socket_fd, buf, MAX_LENGTH, 0, (struct sockaddr *)&server_address, sizeof(struct sockaddr));
        }

      }
       else printf("\nOOPS! What happened? CLIENT");
    } //end else
  } // end while

  close (socket_fd);
}
