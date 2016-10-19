
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
//#define portNum 21234

#define BUFLEN 10240



#define GET_FILE '1'
#define PUT_FILE '2'
#define LIST_FILES '3'
#define CLOSE_SOC '4'

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
   size_t packet_index, size;
      struct timeval timeout = {2,0}; // determines the timeout
   char send_buffer[10240], read_buffer[256];
   packet_index = 1;

  fd_set fds;
  int buffer_fd, buffer_out, flags;
  socklen_t servlen = sizeof(clientAddr);  // new

  if (!(picture = fopen(filename, "r"))) 
  {
    //perror("fopen");
  printf("The file asked for is not found\n");
  printf("These are the list of files in your folder\n");
  system("ls");  
  size = -1;
  sendto(sockfd, (void *)&size, sizeof(int), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
  return -1;

  }    

   printf("Finding the size of the file using fseek\n"); 
   
   fseek(picture, 0, SEEK_END);
   size = ftell(picture);
   fseek(picture, 0, SEEK_SET);
   printf("Total file size is: %lu\n",size);

   char packet_0[16];
   char imageSize[8];

   memcpy(packet_0, "00000000", 8);
   sprintf(imageSize, "%08lu", size);

   printf("size very important %lu\n", size);

   strcat(packet_0, imageSize);

   printf("Sending Picture Size from Server to Client\n");
   sendto(sockfd, packet_0, 100, 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
   //Send Picture as Byte Array
   printf("Waiting for Acknowledgment of size received .....\n");

    FD_CLR(sockfd, &fds);
    FD_ZERO(&fds);
    FD_SET(sockfd,&fds);
    int ola = setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
            sizeof(timeout));

    if (ola < 0)
    {
      printf("setsockopt failed \n");
    }


    while(1)
    {
        // Receiving the ack for packet size
          recvfrom(sockfd, &read_buffer, 255, 0, (struct sockaddr *)&clientAddr, &servlen);
          if (strcmp(read_buffer, "Got_it") == 0)
          {
            printf("ACK for PKT  Received is : %s \n", read_buffer);
            //bzero(imagesize_filename, sizeof(imagesize_filename));
            bzero(packet_0, sizeof(packet_0));
            //bzero(filename, sizeof(filename));
            break;
          }

          else
          {
            printf("TIME OUT here!!!\n");
            printf("Re sending size:\n");
            stat = sendto(sockfd, packet_0, 100, 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));

          }
      }

  char sendBuf_withpcktSize[BUFLEN];

   while(!feof(picture)) {
      read_size = fread(send_buffer, 1, sizeof(send_buffer)-1, picture);
      char packet_size[8];
      sprintf(packet_size, "%08lu", packet_index);
      strcpy(sendBuf_withpcktSize, packet_size);
      memcpy(&sendBuf_withpcktSize[8], send_buffer, sizeof(send_buffer));


      printf("Packet NUMBER: %lu\n",packet_index);
      printf("Packet SIZE SENT: %lu\n",read_size);  
      

      //Send data through the socket 

      do{
        stat = sendto(sockfd, sendBuf_withpcktSize, read_size + 8, 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
        FD_CLR(sockfd, &fds);
        FD_ZERO(&fds);
        FD_SET(sockfd,&fds);
        int ola = setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                sizeof(timeout));

        if (ola < 0)
        {
          printf("setsockopt failed \n");
        }


        while(1)
        {
          recvfrom(sockfd, &read_buffer, 255, 0, (struct sockaddr *)&clientAddr, &servlen);
          if (strcmp(read_buffer, "Done") == 0)
          {
            printf("ACK for PKT %lu Received is : %s \n", packet_index, read_buffer);
            break;
          }

          else
          {
            printf("TIME OUT !!!n");
            printf("Re sending PKT: %lu\n", packet_index);
            stat = sendto(sockfd, sendBuf_withpcktSize, read_size+8, 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));

          }
      }
      }while (stat < 0);

      printf(" \n");
      printf(" \n");


      packet_index++;  

      //Zero out our send buffer
      bzero(send_buffer, sizeof(send_buffer));
      bzero(sendBuf_withpcktSize, sizeof(sendBuf_withpcktSize));
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

int buffersize = 0, recv_size = 0,size = 0, read_size, write_size,stat;
size_t packet_index =1;

char *imagearray,verify = '1';
imagearray = malloc(300241);
FILE *image;
char recBuf_withoutLen[BUFLEN];
char read_size_file_name[100];
//Find the size of the image
char packet_0_array[8];
char size_array[8];
char filename[100];

printf("entering loop\n");


fd_set fds;
int buffer_fd, buffer_out;
int j =0;
size_t packet_index_previous =0;
read_size = 0;
size = 1;
// continue till the size of the file completes

while(recv_size < size) 
{
      // reads the packet 
        read_size = recvfrom(sockfd, imagearray, 300241, 0, (struct sockaddr *)&clientAddr, &clientlen);

        // packet zero contains packet information
        memcpy(packet_0_array, imagearray,8);
        size_t packet_0 = strtoumax(packet_0_array, NULL, 10);
        printf("packet_0: %lu\n", packet_0);

        // packet 0 contains the information of file name, size of the image
        // packets > 0 contains the actual information of the file

        if (packet_0 == 0)
        {
        memcpy(size_array, &imagearray[8], 8);
        memcpy(filename, &imagearray[16], 100);
        printf("filename %s\n", filename);
        printf("after\n");
        printf("%s\n", packet_0_array );
        printf("%s\n", size_array);
          printf("Sending ACK for size ...\n");
          do{
     
            stat = sendto(sockfd, "Got_it", sizeof("Got_it"), 0, (struct sockaddr *)& clientAddr, sizeof(clientAddr));
          }while(stat<0);

          // Retreiving the information of size and filename
          size = strtoumax(size_array, NULL, 10);
          printf("FILE SIZE: %d\n",size);
          printf(" \n");
          char pathToImages[200] = "images/";
          char totalFileName[100] = "rec@server_";

          strcat(totalFileName, filename);
          printf("totalFileName %s\n", totalFileName);
          strcat(pathToImages, totalFileName);
          image = fopen(totalFileName, "ab+");

          if( image == NULL) {
          printf("Error has occurred. Image file could not be opened/ created\n");
          return -1; }
        
          bzero(imagearray, sizeof(imagearray));
          bzero(packet_0_array, sizeof(packet_0_array));
          bzero(size_array, sizeof(size_array));
          bzero(filename, sizeof(filename));
          bzero(totalFileName, sizeof(totalFileName));
          bzero(pathToImages, sizeof(pathToImages));      
      }
      
      else 
      {

          printf("Sending ACK ...\n");
          sendto(sockfd, "Done", sizeof("Done"), 0, (struct sockaddr *)& clientAddr, sizeof(clientAddr));
          printf("ACK for PKT %lu sent\n", packet_index);


          char recv_frame_size[8];
          strncpy(recv_frame_size, imagearray, 8);
          packet_index = strtoumax(recv_frame_size, NULL, 10);

          memcpy(recBuf_withoutLen, &imagearray[8], BUFLEN);


          printf("Packet number received: %lu\n",packet_index);
          printf("Packet size: %d\n",read_size);

          if ((packet_index - packet_index_previous) == 1)
          {
            //Write the currently read data into our image file
              read_size = read_size - 8;
              write_size = fwrite(recBuf_withoutLen,1,read_size, image);
              printf("Written File size: %i\n",write_size); 

              if(read_size !=write_size) 
              {
                printf("error in read write\n");      
              }
              //Increment the total number of bytes read
              recv_size += read_size;
              printf("Total received File size: %i\n",recv_size);
              printf(" \n");
              printf(" \n");
          }

          else
          {
            bzero(imagearray, sizeof(imagearray));
          }
          
          packet_index_previous = packet_index;
          printf("packet_index_previous %lu\n", packet_index_previous);
          bzero(imagearray, sizeof(imagearray));

          printf("recv_size %d\n", recv_size);
          printf("size %d\n", size);
        }

      }

        fclose(image);
        printf("File successfully Received!\n");
        recv_size = 0;
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
	struct sockaddr_in servAddr;	/* server address */
	struct sockaddr_in clientAddr;	/* remote address */
	socklen_t clientlen = sizeof(clientAddr);		/* length of addresses */
	int recvlen;			/* # bytes received */
	int sockfd;			
	if (argc < 2)
	{
		printf("Invalid arguments entered\n\n");
		printf("Enter the arguments in the following format: ./server [Port Number]\n");
		exit(1);
	}

	int portNum = strToInt(argv[1]);//argv[2];
  //printf("here\n");
	if (portNum < 1024 || portNum > 65535)
	{
		printf("You have entered the Port Number as %d and is invalid\n", portNum);
		printf("Choose a port number in between 1023 and 50000\n");
		exit(1);
	}

	/* create a UDP socket */

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Receiver socket creation failed\n");
		return 0;
	}

	// bind the socket to any IP address 

	memset((char *)&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);           
	servAddr.sin_port = htons(portNum);

	if (bind(sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
		perror("BIND operation failed");
		return 0;
	}


  char option;
  char putFileName[50];
  char getFileName[50];
  int stat;
  struct timeval timeout = {0,0};
  FILE *checkFile;
	for (;;) {
            int ola = setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                sizeof(timeout));
    if (recvfrom(sockfd, (void *)&option, sizeof(int), 0, (struct sockaddr *)&clientAddr, &clientlen)==-1)
    {
      perror("option receiving failed");
    }
    printf("Option %c received\n", option);

    switch(option)
    {
      case GET_FILE:
          // send the file
          printf("receiving the file name\n");
          
          // rceiving the file name to send
          if (recvfrom(sockfd, &getFileName, 50, 0, (struct sockaddr *)&clientAddr, &clientlen)==-1)
          {
            perror("option receiving failed");
          }
          printf("The file name asked for is %s\n", getFileName);

          // sending the image
          send_image(sockfd, getFileName, clientAddr);  
          
      break;

      case PUT_FILE:
        // gets the file
          printf("receiving the file name\n");
            // receiving the file
            receive_image( sockfd, clientAddr, clientlen); 
            printf("completed putfile case\n");
            break;

      case LIST_FILES:
      // list of files
          system("ls > list.txt"); // using the system command to store the lit of files
          printf("Sending the ist of files on server side\n");
          send_image(sockfd, "list.txt", clientAddr);

      break;

      case CLOSE_SOC:
        printf("Closing the socket and exiting\n");
        close(sockfd);   // closing the socket and exiting
        exit(1);
      break;

      default:
      break;

    }

	}

}
