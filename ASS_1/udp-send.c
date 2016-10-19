
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
// Header files to support Socket Programming
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 
#include <fcntl.h>
#include <signal.h>
#include <time.h>

#define BUFLEN 10248   // Choosing the packet size to 10248

// commands()
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
 * void select_menu() Displays the menu
 *****************************************************/
void select_menu()
{
	printf("\n\n");
	printf("				**************************************\n");
	printf("				* Enter the command u wish to perform\n");
	printf("				* gets: get file from server\n");
	printf("				* puts: put file from server\n");
	printf("				* ls: list files on the server side\n");
	printf("				* exit: close the socket\n");
	printf("				**************************************\n");
	printf("\n\n");

}


/*****************************************************
 * char user_commands(): Decides the command to send 
 						 based on use input
 *****************************************************/
char user_commands()
{
	char option[6];
	char optionValue;
	select_menu();
	
	while(1)
	{
		scanf("%s", option);           // scans the input string from the user

		if (strcmp(option, "gets") == 0)
		{
			optionValue = GET_FILE;   // Command: gets
			printf("COMMAND ENTERED =  is %s \n", option);
			break;
		}
		else if (strcmp(option, "puts") == 0)
		{
			optionValue = PUT_FILE;   // Command: puts
			printf("COMMAND ENTERED =  is %s \n", option);
			break;
		}
		else if (strcmp(option, "ls") == 0)
		{
			optionValue = LIST_FILES; // Command: ls
			printf("COMMAND ENTERED =  is %s \n", option);
			break;
		}

		else if(strcmp(option, "exit") == 0)
		{
			optionValue = CLOSE_SOC;  // Command: close the socket
			printf("COMMAND ENTERED =  is %s \n", option);
			break;
		}

		else
		{
			printf("The command you have entered is %s and is invalid\n", option);
			printf("Re-enter an command between\n");
			select_menu();
		}


	}

	return optionValue;
}


/*****************************************************
 * send_image: Transfers the file from Client to Server
 
 ARGUMENTS
 int sockfd - descriptor for socket created  
 char *filename - filename to send from client to server
 struct sockaddr_in servAddr  - contains the server informatiom
 *****************************************************/

int send_image(int sockfd, char *filename, struct sockaddr_in servAddr){

   FILE *picture;                 // Reads the picture
   size_t size, read_size, stat;  // size: stores the total size of the image, read_size: stores the return value of func recvfrm() 
   size_t packet_index;           // Sequence number of the packer being sent
   char send_buffer[10240], read_buffer[256];  // send_buffer: buffer to send the packet read_buffer: reads the message server
   struct timeval timeout = {2,0}; // determines the timeout


	fd_set fds;   
	int buffer_fd, buffer_out, flags;
    packet_index = 1;

   socklen_t servlen = sizeof(servAddr);	// determines the size of server address

	while(1)
	{
		if (!(picture = fopen(filename, "r"))) 
		{
				perror("fopen");
				printf("These are the list of files in your folder\n");
				system("ls");	    	
				printf("Re enter the file name\n");
				scanf("%s", filename);
		}

		else{
			break;
		}
	}

   // Determining the file size using fseek and ftell
   printf("Finding the size of the file using fseek\n"); 
   fseek(picture, 0, SEEK_END);
   size = ftell(picture);
   fseek(picture, 0, SEEK_SET);
   printf("Total file size is: %lu\n",size);

   char packet_0[16];  // appends sequence number into the buffer packet
   char imageSize[8];  // appends the image size
   char imagesize_filename[100]; // apends the filename 

   memcpy(packet_0, "00000000", 8);
   sprintf(imageSize, "%08lu", size);

   strcat(packet_0, imageSize);
   strcat(imagesize_filename, packet_0);

   memcpy(&imagesize_filename[16], filename, 20);
   printf("Sending Picture Size from Client to Server\n");

   // imagesize_file send the packet index, file size and the file name being sent
   sendto(sockfd, imagesize_filename, 100, 0, (struct sockaddr *)&servAddr, sizeof(servAddr));

   	   
	printf("Waiting for Acknowledgment of size received .....\n");

			FD_CLR(sockfd, &fds);
		    FD_ZERO(&fds);
			FD_SET(sockfd,&fds);
			int ola = setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
		            sizeof(timeout));       // Associating timeout with the socket sockfd
		if (ola < 0)
        {
        	printf("setsockopt failed \n");
        }


        while(1)
		{
			// Receiving the acknowledgement
	        recvfrom(sockfd, &read_buffer, 255, 0, (struct sockaddr *)&servAddr, &servlen);
	        if (strcmp(read_buffer, "Got_it") == 0)
	        {
	        	printf("ACK for PKT  Received is : %s \n", read_buffer);
	        	bzero(imagesize_filename, sizeof(imagesize_filename));
	        	bzero(packet_0, sizeof(packet_0));
	        	bzero(filename, sizeof(filename));
	        	break;
	        }

	        // Time out: Resending the packet
	        else
	        {
	        	printf("TIME OUT here!!!\n");
	        	printf("Re sending size:\n");
	        	stat = sendto(sockfd, imagesize_filename, 100, 0, (struct sockaddr *)&servAddr, sizeof(servAddr));

	        }
	    }

   char sendBuf_withpcktSize[BUFLEN];


   printf("Starting the FIle Transimission\n");

   while(!feof(picture)) {
 
      read_size = fread(send_buffer, 1, sizeof(send_buffer)-1, picture);  // reading a buffer of size 10248
      char packet_size[8];                                                // packet size
      sprintf(packet_size, "%08lu", packet_index);
      strcpy(sendBuf_withpcktSize, packet_size);
      memcpy(&sendBuf_withpcktSize[8], send_buffer, sizeof(send_buffer));

      // sendBuf_withpcktSize: contains the packet index and packet buffer

      printf("Packet NUMBER: %lu\n",packet_index);
      printf("Packet SIZE SENT: %lu\n",read_size);  
      
      do{  
      	// Sending the buffer
	    stat = sendto(sockfd, sendBuf_withpcktSize, read_size+8, 0, (struct sockaddr *)&servAddr, sizeof(servAddr));
        FD_CLR(sockfd, &fds);
        FD_ZERO(&fds);
    	FD_SET(sockfd,&fds);
    	int ola = setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                sizeof(timeout)); // associating timeout

        if (ola < 0)
        {
        	printf("setsockopt failed \n");
        }


        while(1)
		{
			// receiving the acknowledgeement for packets being sent

	        recvfrom(sockfd, &read_buffer, 255, 0, (struct sockaddr *)&servAddr, &servlen);
	        if (strcmp(read_buffer, "Done") == 0)
	        {
	        	printf("ACK for PKT %lu Received is : %s \n", packet_index, read_buffer);
	        	break;
	        }

	        else
	        {
	        	// Time out
	        	printf("TIME OUT !!!n");
	        	printf("Re sending PKT: %lu\n", packet_index);
	        	stat = sendto(sockfd, sendBuf_withpcktSize, read_size+8, 0, (struct sockaddr *)&servAddr, sizeof(servAddr));

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
 * receive_image: Receives the file from Client to Server
 
 ARGUMENTS
 int sockfd - descriptor for socket created  
 char *filename - filename to send from client to server
 struct sockaddr_in servAddr  - contains the server information
 socklen_t clientlen: length of the client

 Returns 1 on completion
 ******************************************************************/

int receive_image(int sockfd, char *filename, struct sockaddr_in servAddr, socklen_t clientlen)
{ 
	
	int buffersize = 0, recv_size = 0,size = 0, read_size, write_size, stat;
	size_t packet_index =1;

	char *imagearray;  // image array stores the incoming buffer
	imagearray = malloc(300241);
	FILE *image;

	char recBuf_withoutLen[BUFLEN]; // receiver buffer with the length and packet index removed
	char read_size_file_name[100];  // used to read file name
	char packet_0_array[8];         // used for reading packet zero
	char size_array[8];             // used for reading size

	fd_set fds;
	int buffer_fd, buffer_out;
	int j =0;
	size_t packet_index_previous =0;
	read_size = 0;
	size = 1;


	while(recv_size < size) {
		// receieves the buffer
		 read_size = recvfrom(sockfd, imagearray, 300241, 0, (struct sockaddr *)&servAddr, &clientlen);

        memcpy(packet_0_array, imagearray,8);

        size_t packet_0 = strtoumax(packet_0_array, NULL, 10);
        printf("packet_0: %lu\n", packet_0);

        // packet 0 contains the information of file name, size of the image
        // packets > 0 contains the actual information of the file
        if (packet_0 == 0)
        {
        memcpy(size_array, &imagearray[8], 8);

        printf("%s\n", packet_0_array );
        printf("%s\n", size_array);

        // sending the ack for size of the file
          printf("Sending ACK for size ...\n");
          do{
            stat = sendto(sockfd, "Got_it", sizeof("Got_it"), 0, (struct sockaddr *)& servAddr, sizeof(servAddr));
          }while(stat<0);

          // Retreiving the information of size and filename
          size = strtoumax(size_array, NULL, 10);
          printf("FILE SIZE: %d\n",size);
          printf(" \n");

          // storing the received images
          char pathToImages[200] = "images/";
          char totalFileName[100] = "rec@client_";
          // opens the file if already exists or crates a new one
          strcat(totalFileName, filename);
          strcat(pathToImages, totalFileName);
          image = fopen(totalFileName, "ab+");
         
          if( image == NULL) {
          printf("Error has occurred. Image file could not be opened/ created\n");
          return -1; }
        
          // zeroing out all the buffers
          bzero(imagearray, sizeof(imagearray));
          
          bzero(packet_0_array, sizeof(packet_0_array));
          
          bzero(size_array, sizeof(size_array));
          //bzero(filename, sizeof(filename));
          
          bzero(totalFileName, sizeof(totalFileName));
          
          bzero(pathToImages, sizeof(pathToImages));
      
      }
      
      else 
      {

      	  // sending an acknowledgement for packet received
          printf("Sending ACK ...\n");
          sendto(sockfd, "Done", sizeof("Done"), 0, (struct sockaddr *)& servAddr, sizeof(servAddr));
          printf("ACK for PKT %lu sent\n", packet_index);


          // Retrieving the packet index and actual buffer associated with the file being received
          char recv_frame_size[8];
          strncpy(recv_frame_size, imagearray, 8);
          packet_index = strtoumax(recv_frame_size, NULL, 10);

          memcpy(recBuf_withoutLen, &imagearray[8], BUFLEN);


          printf("Packet number received: %lu\n",packet_index);
          printf("Packet size: %d\n",read_size);

          // This loops discards the packet that are received in redundancy
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
 char **argv  - [ipaddress] [PORT NUMBER]
 ******************************************************************/

int main(int argc, char **argv)
{
	// invalid if arguments less than three
	if (argc < 3)
	{
		printf("Invalid arguments entered\n\n");
		printf("Enter the arguments in the following format: ./client [Server address] [Port Number]\n");
		exit(1);
	}

	int portNum = strToInt(argv[2]);
	if (portNum < 1024 || portNum > 65535)
	{
		printf("You have entered the Port Number as %d and is invalid\n", portNum);
		printf("Choose a port number in between 1023 and 50000\n");
		exit(1);
	}

	struct sockaddr_in clientAddr, servAddr;
	socklen_t servlen = sizeof(servAddr);
	int sockfd, i, slen=sizeof(servAddr);
	struct hostent *server;
	char buf[BUFLEN];
	server = gethostbyname(argv[1]);
	
	// Creation of a socket
	if ((sockfd=socket(AF_INET, SOCK_DGRAM, 0))==-1)
	{
		printf("Client socket creation failed\n");
	}

	// binding to the ip addess 
	memset((char *)&clientAddr, 0, sizeof(clientAddr));
	clientAddr.sin_family = AF_INET;
	clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	clientAddr.sin_port = htons(0);

	if (bind(sockfd, (struct sockaddr *)&clientAddr, sizeof(clientAddr)) < 0) {
		perror("bind failed");
		return 0;
	}       

	// Defining the server address 

	memset((char *) &servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(portNum);
	        bcopy((char *)server->h_addr, 
             (char *)&servAddr.sin_addr.s_addr,
             server->h_length);


	// choosing the option

	char option;      
	char getFileName[20];
	char putFileName[20];

	FILE *picture;

	while(1)
	{
		option = user_commands(); // selecting the command received

		printf("Option Sent !!\n");

			if (sendto(sockfd, (void *)&option, sizeof(int), 0, (struct sockaddr *)&servAddr, sizeof(servAddr))==-1)
			{
				perror("option sending failed");
			}



		// Sending the commad to server based on the option received
		switch(option)
		{
			// gets the file from server
			case GET_FILE:
				// choosing the filename
				printf("Enter the file name you wish to receive\n");
				scanf("%s", getFileName);
				printf("The file name entered is %s\n", getFileName);

				while(1)
			    {
			    	if (!(picture = fopen(getFileName, "r"))) 
			    	{
			   			perror("fopen");
						printf("These are the list of files in your folder\n");
			    		system("ls");	    	
			    		printf("Re enter the file name\n");
			    		scanf("%s", getFileName);
			    	}

			    	else{
			    		break;
			    	}
			    }

			    // sending the filename
				if (sendto(sockfd, getFileName, sizeof(getFileName), 0, (struct sockaddr *)&servAddr, sizeof(servAddr))==-1)
				{
					perror("option sending failed");
				}

				// receiving the image from server
				receive_image(sockfd, getFileName, servAddr, servlen);

				break;

			// puts the file onto server
			case PUT_FILE:
				printf("Enter the file name you wish to send\n");
				scanf("%s", putFileName);
				printf("The file name entered is %s\n", putFileName);

				while(1)
			    {
			    	if (!(picture = fopen(putFileName, "r"))) 
			    	{
			   			perror("fopen");
						printf("These are the list of files in your folder\n");
			    		system("ls");	    	
			    		printf("Re enter the file name\n");
			    		scanf("%s", putFileName);
			    	}

			    	else{
			    		break;
			    	}
			    }

			    // Sending the file to server
				send_image(sockfd, putFileName, servAddr);
				break;

			case LIST_FILES:
				// receving the list of files on server side

				receive_image( sockfd, "list_client.txt", servAddr, servlen);
				FILE *fListfile;
			 	char readCharfrmFile;
			    // Open file
			    fListfile = fopen("rec@client_list_client.txt", "r");
			    if (fListfile == NULL)
			    {
			        printf("Unable to open the file list_client.txt \n");
			        exit(0);
			    }
			 
			    // Read contents from file
			    readCharfrmFile = fgetc(fListfile);
			    while (readCharfrmFile != EOF)
			    {
			        printf ("%c", readCharfrmFile);
			        readCharfrmFile = fgetc(fListfile);
			    }
			 
			    fclose(fListfile);
			break;

			case CLOSE_SOC:
			break;

			default:
			break;

		}
	}


	close(sockfd);
	return 0;
}
