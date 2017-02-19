
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
//#define portNum 21234

#define BUFLEN 10240



#define GET_FILE 1
#define PUT_FILE  2
#define LIST_FILES 3
#define CLOSE_SOC 4

/*****************************************************
 * int strToInt(char *str) Configures String to Integer
 *****************************************************/
int strToInt(char *str)
{
int num = 0;
int i=0;

  for (i = 0; str[i] != '\0'; i++)
  {
      num = num * 10 + str[i] - '0';  // Converts String to integer
  }
  return num;
}

/*****************************************************
 * send_image: Transfers the file from Server to Client
 
 ARGUMENTS
 int sockfd - descriptor for socket created  
 char *filename - filename to send from Server to Client
 struct sockaddr_in servAddr  - contains the client informatiom
 *****************************************************/

int send_image(int sockfd, char *filename, struct sockaddr_in clientAddr){

   FILE *picture;                   // Reads the picture
   // size: stores the total size of the image, read_size: stores the return value of func recvfrm() 
   // Sequence number of the packer being sent
   // send_buffer: buffer to send the packet read_buffer: reads the message server
   size_t  read_size, stat; 
   int packet_index, size;
      struct timeval timeout = {2,0}; // determines the timeout
   char send_buffer[1024], read_buffer[256];
   packet_index = 1;

  fd_set fds;
  int buffer_fd, buffer_out, flags;
  socklen_t servlen = sizeof(clientAddr);  // new


  if (!(picture = fopen(filename, "r"))) 
  {
    perror("fopen");
    return -1;
  }    

   printf("Finding the size of the file using fseek\n"); 
   
   fseek(picture, 0, SEEK_END);
   size = ftell(picture);
   fseek(picture, 0, SEEK_SET);
   printf("Total file size is: %d\n",size);

   // char packet_0[16];
   // char imageSize[8];

   // memcpy(packet_0, "00000000", 8);
   // sprintf(imageSize, "%08lu", size);

   // printf("size very important %lu\n", size);

   // strcat(packet_0, imageSize);

   printf("Sending Picture Size from Server to Client\n");
   
   //sendto(sockfd, packet_0, 100, 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
   send(sockfd, (void *)&size, sizeof(int), 0);//, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
    if (stat < 0)
   {
    perror("Error sending size");
    exit(1);
   }

   // stat = send(sockfd, filename, 100,0);
   // if (stat < 0)
   // {
   //  perror("Error sending filename");
   //  exit(1);
   // }

   //Send Picture as Byte Array
   //printf("Waiting for Acknowledgment of size received .....\n");

    // FD_CLR(sockfd, &fds);
    // FD_ZERO(&fds);
    // FD_SET(sockfd,&fds);
    // int ola = setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
    //         sizeof(timeout));

    // if (ola < 0)
    // {
    //   printf("setsockopt failed \n");
    // }


    // while(1)
    // {
    //     // // Receiving the ack for packet size
    //     //   recvfrom(sockfd, &read_buffer, 255, 0, (struct sockaddr *)&clientAddr, &servlen);
    //     //   if (strcmp(read_buffer, "Got_it") == 0)
    //     //   {
    //     //     printf("ACK for PKT  Received is : %s \n", read_buffer);
    //     //     bzero(packet_0, sizeof(packet_0));
    //     //     //bzero(filename, sizeof(filename));
    //     //     break;
    //     //   }

    //       else
    //       {
    //         printf("TIME OUT here!!!\n");
    //         printf("Re sending size:\n");
    //         stat = sendto(sockfd, packet_0, 100, 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));

    //       //}
    //   }

  char sendBuf_withpcktSize[BUFLEN];

   while(!feof(picture)) {
      read_size = fread(send_buffer, 1, sizeof(send_buffer)-1, picture);
      //char packet_size[8];
      //sprintf(packet_size, "%08lu", packet_index);
      //strcpy(sendBuf_withpcktSize, packet_size);
      //memcpy(&sendBuf_withpcktSize[8], send_buffer, sizeof(send_buffer));


      //printf("Packet NUMBER: %lu\n",packet_index);
      //printf("Packet SIZE SENT: %lu\n",read_size);  
      

      //Send data through the socket 

      do{
        // stat = sendto(sockfd, sendBuf_withpcktSize, read_size + 8, 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
        // FD_CLR(sockfd, &fds);
        // FD_ZERO(&fds);
        // FD_SET(sockfd,&fds);
        // int ola = setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
        //         sizeof(timeout));

        // if (ola < 0)
        // {
        //   printf("setsockopt failed \n");
        // }


        // while(1)
        // {
        //   recvfrom(sockfd, &read_buffer, 255, 0, (struct sockaddr *)&clientAddr, &servlen);
        //   if (strcmp(read_buffer, "Done") == 0)
        //   {
        //     printf("ACK for PKT %lu Received is : %s \n", packet_index, read_buffer);
        //     break;
        //   }

          //else
          //{
            //printf("TIME OUT !!!n");
            //printf("Re sending PKT: %lu\n", packet_index);
            stat = send(sockfd, send_buffer, read_size, 0);//, (struct sockaddr *)&clientAddr, sizeof(clientAddr));

          //}
      //}
      }while (stat < 0);

      printf(" \n");
      printf(" \n");


      //Zero out our send buffer
      bzero(send_buffer, sizeof(send_buffer));
      //bzero(sendBuf_withpcktSize, sizeof(sendBuf_withpcktSize));
     }
}



/*****************************************************************
 * receive_image: Receives the file from Server to Client
 
 ARGUMENTS
 int sockfd - descriptor for socket created  
 struct sockaddr_in servAddr  - contains the server information
 socklen_t clientlen: length of the Server

 Returns 1 on completion
 ******************************************************************/

int receive_image(int sockfd, struct sockaddr_in clientAddr, socklen_t clientlen)
{ 

  int buffersize = 0, recv_size = 0, read_size, write_size,stat;
  int size = 0;
  char *imagearray;
  imagearray = malloc(300241);
  FILE *image;
  char filename[100];

  printf("Entering the receive_image function\n");

  read_size = 0;
  size = 1;
  // continue till the size of the file completes
  stat = 0;
  stat = recv(sockfd, &size, sizeof(int), 0);  // reading size of the image
  if (stat < 0)
  {
    perror("Error receiving Size");
  }
  printf("size of the file is %d\n", size);

  stat = recv(sockfd, filename, 100, 0);  // reading the filename 
  if (stat < 0)
  {
    perror("Error receiving filename");
  }

  char filename_received[100];
  bzero(filename_received,sizeof(filename_received));
  strncat(filename_received, filename, 100);
  strncat(filename_received, "_received",100);
  printf("Filename is received is %s\n", filename_received);
  
  // opeing an image in write mode
  image = fopen(filename_received, "w");
  // file open error
  if( image == NULL) {
    printf("Error has occurred. Image file could not be opened/ created\n");
    return -1; 
  }

  // receiving the file from client
  while(recv_size < size) 
  {

    read_size = recv(sockfd, imagearray, 300241, 0);
    printf("readsize is %d \n", read_size);

    write_size = fwrite(imagearray,1,read_size, image);
    printf("Written File size: %d\n",write_size); 
    recv_size += read_size;
    printf("Total received File size: %i\n",recv_size);
    printf(" \n");
    printf(" \n");

  }

  fclose(image);
  return 1;

}

/*****************************************************************
 * main() starts the operations based on the commands
 
 ARGUMENTS
 int argc     - argument count  
 char **argv  - [PORT NUMBER]
 ******************************************************************/

int main(int argc, char **argv)
{


  int sockfd, newsockfd, portno;
  socklen_t clientlen;
  char buffer[256];
  struct sockaddr_in servAddr, clientAddr;
  int n;
  if (argc < 2) {
     fprintf(stderr,"ERROR, no port provided\n");
     exit(1);
  }
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");
  bzero((char *) &servAddr, sizeof(servAddr));
  portno = atoi(argv[1]);
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = INADDR_ANY;
  servAddr.sin_port = htons(portno);
  if (bind(sockfd, (struct sockaddr *) &servAddr,
          sizeof(servAddr)) < 0) 
          error("ERROR on binding");
        printf("coming here\n");
  listen(sockfd,5);
  clientlen = sizeof(clientAddr);
  newsockfd = accept(sockfd, 
             (struct sockaddr *) &clientAddr, 
             &clientlen);
  if (newsockfd < 0) 
      error("ERROR on accept");


  int option;
  char putFileName[50];
  char getFileName[50];
  int stat;
  int n1;
  FILE *checkFile;
  
	for (;;) {

      n = recv(newsockfd, (void *)&option, sizeof(int), 0);    // Reading the option from user side
      if (n < 0)
      {
        perror("option receiving failed");
        exit(1);
      }          

    printf("Option %d received\n", option);

    switch(option)
    {
      // This command sends the file to Client 
      case GET_FILE:
          printf("receiving the file name\n");
          
          // rceiving the file name to send

          while(1)
          {


                // if (getFileName[0]>0 && getFileName[0] <5)
                // {
                //   //bzero(getFileName, sizeof(getFileName)); 
                // }
                //else
                //{
              stat = recv(newsockfd, getFileName, 50,0);
               if (stat < 0)
               {
                perror("Error sending filename");
                exit(1);
               }
                  printf("The file name asked for is %s\n", getFileName);
                  // sending the image

                  send_image(newsockfd, getFileName, clientAddr);  
                  printf("Exiting the GET_FILE\n");
                  break;
                //}
        }

          option = 10;
      break;

      // This command gets file from Client 
      case PUT_FILE:
        
            receive_image( newsockfd, clientAddr, clientlen); 
            printf("completed putfile case\n");
            option = 10;
            break;

      // This command send list of files in the current directory to client
      case LIST_FILES:
      // list of files
      while(1)
          {
                if (recvfrom(sockfd, &getFileName, 50, 0, (struct sockaddr *)&clientAddr, &clientlen)==-1)
                {
                  perror("option receiving failed");
                }

                if (getFileName[0]>0 && getFileName[0] <5)
                {
                  //bzero(getFileName, sizeof(getFileName));
                  
                }

                else
                {
                    printf("The file name asked for is %s\n", getFileName);

                  // sending the image
                  system("ls > list.txt"); // using the system command to store the lit of files
                  printf("Sending the ist of files on server side\n");
                  send_image(sockfd, "list.txt", clientAddr);
                  
                  printf("Exiting the GET_FILE\n");
                  break;
                }
        }

          option = 10;
      break;

      // This command closes the server socket and exits the program
      case CLOSE_SOC:
        printf("Closing the socket and exiting\n");
        close(sockfd);   // closing the socket and exiting
        exit(1);
        option = 10;
      break;

      default:
      break;

    }

	}

}
