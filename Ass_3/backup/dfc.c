
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
#define GET_FILE 1  
#define PUT_FILE 2
#define LIST_FILES 3
#define CLOSE_SOC 4

#define DFS_SERVER1  0
#define DFS_SERVER2  1
#define DFS_SERVER3  2
#define DFS_SERVER4  3
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

/***********************
Gets file size
************************/
static unsigned int get_file_size (FILE * fileDescriptor)
{
    unsigned int size;

    fseek(fileDescriptor, 0L, SEEK_END);
    size = ftell(fileDescriptor);
    fseek(fileDescriptor, 0L, SEEK_SET);

    return size;
}

/***********************************
Reading the dsc.conf
1. Extracts the DFS server information
2. Extracts Username and Password
************************************/ 

char Server_list[4][100], USERNAME[100], PASSWORD[100];
void readDfcconfig()
{
    FILE *fp;
    char wsBuf[200];
    char *val1;

    int i = 0;
    fp=fopen("dfc.conf","r");
    if (fp == NULL)
    {

        perror("dfc.conf");
        exit(1);
    }

    else
    {
        unsigned int wsConfigFileSize = get_file_size (fp);

        printf("dfc.conf size n = %d, filename = dsc.conf \n", wsConfigFileSize);

        while(fgets(wsBuf,wsConfigFileSize,fp)!=NULL) {

            /**********************
            * Finds Server
            /**********************/
            if((strncmp(wsBuf,"Server",6)==0)  || (strncmp(wsBuf,"SERVER",6)==0) ) {
                printf("dfcBuf: %s",wsBuf);
                val1=strtok(wsBuf," \t\n");
                val1 = strtok(NULL, " \t\n");

                // validating the DFS_ string to find the information on server number 
                // and storing it accordingly into the array for easier use
                if (val1[3] == '1'){
                	val1 = strtok(NULL, " \t\n");
                	strcpy(Server_list[DFS_SERVER1],val1);
                	i = 0;
                }
                if (val1[3] == '2'){
                	val1 = strtok(NULL, " \t\n");
                	strcpy(Server_list[DFS_SERVER2],val1);
                	i = 1;
                }

                if (val1[3] == '3'){
                	val1 = strtok(NULL, " \t\n");
                	strcpy(Server_list[DFS_SERVER3],val1);
                	i =2;
                }
                if (val1[3] == '4'){
                	val1 = strtok(NULL, " \t\n");
                	strcpy(Server_list[DFS_SERVER4],val1);
                	i =3;
                }
                printf("%s\n",Server_list[i]);
                bzero(wsBuf, sizeof(wsBuf));
                i = i%4;
            }

            // /****************
            // * Finds Username 
            // /****************/
            if(strncmp(wsBuf,"Username",8)==0) {
                printf("wsBuf: %s",wsBuf);
                val1=strtok(wsBuf," \t\n");
                val1 = strtok(NULL, " \t\n");
                strcpy(USERNAME, val1);
                printf("%s \n", USERNAME);
            	bzero(wsBuf, sizeof(wsBuf));
            }

            // /****************
            // * Finds Password
            // /****************/
            if(strncmp(wsBuf,"Password",8)==0) {
                printf("wsBuf: %s",wsBuf);
                val1=strtok(wsBuf," \t\n");
                val1 = strtok(NULL, " \t\n");
                strcpy(PASSWORD, val1);
                printf("%s\n", PASSWORD);
            	bzero(wsBuf, sizeof(wsBuf));
            }



        }

        fclose(fp);
    }

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
   size_t read_size, stat;  // size: stores the total size of the image, read_size: stores the return value of func recvfrm() 
   int size;
   size_t packet_index;           // Sequence number of the packer being sent
   char send_buffer[1024], read_buffer[256];  // send_buffer: buffer to send the packet read_buffer: reads the message server
   struct timeval timeout = {2,0}; // determines the timeout


	fd_set fds;   
	int buffer_fd, buffer_out, flags;
    packet_index = 1;

   socklen_t servlen = sizeof(servAddr);	// determines the size of server address

   // Takes input as filename 
   // breaks only when valid file name is received
   // else keeps on looping asking filename from user
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
   printf("Total file size is: %d\n",size);

   // very important to send the file name from user
   char imagesize_filename[100]; // apends the filename 
   printf("Sending Picture Size from Client to Server\n");
   stat = send(sockfd, &size, sizeof(int), 0);// sending size of the file
   if (stat < 0)
   {
	perror("Error sending size");
   	exit(1);
   }

   stat = send(sockfd, filename, 100,0);
   if (stat < 0)
   {
	perror("Error sending filename");
   	exit(1);
   }


   printf("Starting the FIle Transimission...\n");

   while(!feof(picture)) {
 
      read_size = fread(send_buffer, 1, sizeof(send_buffer)-1, picture);  // reading a buffer of size 1024

	  // sending file chunk by chunk      
      do{  

	    stat = send(sockfd, send_buffer, read_size, 0);//, 0, (struct sockaddr *)&servAddr, sizeof(servAddr));

	    if (stat < 0)
	    {
	    	perror("Error in sending image");
	    }

      }while (stat < 0);   
      printf(" \n");
      printf(" \n");

      // zeroing out the send_buffer
      bzero(send_buffer, sizeof(send_buffer));
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

  int buffersize = 0, recv_size = 0, read_size, write_size,stat;
  int size = 0;
  char *imagearray;
  imagearray = malloc(300241);
  FILE *image;
  //char filename[100];

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

#define MAX_CONNECTIONS 4
struct sockaddr_in  servAddr[MAX_CONNECTIONS];
socklen_t servlen[MAX_CONNECTIONS];
int sockfd[MAX_CONNECTIONS], slen=sizeof(servAddr);
struct hostent *server;


void createMultipleSockets()
{

    int i = 0;
    int portNum;
    char *serverName;
    char *portStr;


    for (i=0;i<MAX_CONNECTIONS;i++)
    {
    	servlen[i] = sizeof(servAddr[i]);
    	serverName = strtok(Server_list[i], ":");
		portStr = strtok(NULL,"");
		printf("server %s\n", serverName);
		printf("portStr %s\n", portStr);
		portNum = atoi(portStr);

			// Creation of a socket
		if ((sockfd[i]=socket(AF_INET, SOCK_STREAM, 0))==-1)
		{
			perror("Error opening socket");
		}

		server = gethostbyname(serverName);   // getting the address of the server

			// Creating a socket with port num and host address from the config file
		memset((char *)&servAddr[i], 0, sizeof(servAddr[i]));
		servAddr[i].sin_family = AF_INET;
		servAddr[i].sin_port = htons(portNum);
		bcopy((char *)server->h_addr, 
	             (char *)&servAddr[i].sin_addr.s_addr,
	             server->h_length);
	     
		// Connecting to the server socket
		
	    if (connect(sockfd[i],(struct sockaddr *) &servAddr[i],sizeof(servAddr[i])) < 0) 
	    {
	    	perror("ERROR connecting socket");
	    	//exit(1);
	    }
    }
}


/*****************************************************************
 * main() starts the operations based on the commands
 
 ARGUMENTS
 int argc     - argument count  
 char **argv  - [ipaddress] [PORT NUMBER]
 ******************************************************************/

int main(int argc, char **argv)
{

	// This function reads server info and user credentials
	readDfcconfig();
	// This function create four sockets reading the address from the dsc.conf
	createMultipleSockets();


	int option = 0;      
	char getFileName[20];
	char putFileName[20];
	int n;
	FILE *picture;
	int j;
	while(1)
	{
		//
		option = user_commands(); // selecting the command received
		printf("option entered is %d\n", option);

		printf("Option Sent !!\n");

		int n = send(sockfd[0], (void *)&option, sizeof(int), 0);	
		if (n < 0)//, 0, (struct sockaddr *)&servAddr, sizeof(servAddr))==-1)
		{
			perror("Writing to socket: option sending failed");
		}

		// Sending the commad to server based on the option received
		int j;
		switch(option)
		{
			// This command gets the file from server
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

			    	n = send(sockfd[0], getFileName, 50, 0);
					if (n < 0);//, (struct sockaddr *)&servAddr, sizeof(servAddr))==-1)
					{
						perror("option sending failed");
					}

					// receiving the image from server
				//}
				receive_image(sockfd[0], getFileName, servAddr[0], servlen[0]);
				break;

			// This command puts the file onto server
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
				send_image(sockfd[0], putFileName, servAddr[0]);
				break;

			// This command gets the list of files form the server in its current directory
			case LIST_FILES:
			// 	//receving the list of files on server side
			// 	if (sendto(sockfd[0], "list_client.txt", sizeof("list_client.txt"), 0, (struct sockaddr *)&servAddr, sizeof(servAddr))==-1)
			// 	{
			// 		perror("option sending failed");
			// 	}


			// 	receive_image( sockfd[0], "list_client.txt", servAddr[0], sizeof(servAddr[0]));
			// 	FILE *fListfile;
			//  	char readCharfrmFile;
			//     // Open file
			//     fListfile = fopen("rec@client_list_client.txt", "r");
			//     if (fListfile == NULL)
			//     {
			//         printf("Unable to open the file list_client.txt \n");
			//         exit(0);
			//     }
			 
			//     // Read contents from file
			//     readCharfrmFile = fgetc(fListfile);
			//     while (readCharfrmFile != EOF)
			//     {
			//         printf ("%c", readCharfrmFile);
			//         readCharfrmFile = fgetc(fListfile);
			//     }
			 
			//     fclose(fListfile);
			// break;

			// This comand closes the socket
			case CLOSE_SOC:
			break;

			default:
			break;

		}
	}


	close(sockfd);
	return 0;
}
