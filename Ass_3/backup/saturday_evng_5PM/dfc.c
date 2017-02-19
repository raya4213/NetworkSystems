
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
//#include <unistd.h>
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
#define TRUE 1
#define FALSE 0

#define SUCCESS 1
#define FAIL 0
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
The input argument is used to distinguish with and without server details
1. Extracts the DFS server information
2. Extracts Username and Password
************************************/ 

char Server_list[4][100], USERNAME[100], PASSWORD[100];
void readDfcconfig(int check)
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
            if (check){
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
        	}
        	else
        	{
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

int 	send_image(int sockfd, char *filename, struct sockaddr_in servAddr, char *subfolder){

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
   // send file name
   stat = send(sockfd, filename, 100,0);
   if (stat < 0)
   {
	perror("Error sending filename");
   	exit(1);
   }
   //send subfolder
   stat = send(sockfd, subfolder, 100,0);
   if (stat < 0)
   {
	perror("Error sending subfolder");
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

int receive_image(int sockfd, char *filename, struct sockaddr_in servAddr, socklen_t clientlen, char *subfolder)
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
  char filename_received[100];
  bzero(filename_received,sizeof(filename_received));
	
	// sending the subfolder to receive from
  printf("subfolder %s\n", subfolder);
	stat = send(sockfd, subfolder, 100, 0);
	if (stat < 0)//, (struct sockaddr *)&servAddr, sizeof(servAddr))==-1)
	{
		perror("subfolder sending failed");
	}
	//exit(1);
  stat = recv(sockfd, filename_received, sizeof(filename_received), 0);  // reading size of the image
  if (stat < 0)
  {
    perror("Error receiving filename");
  }
  //receiving status of file
  int filestatus;
  stat = recv(sockfd, &filestatus, sizeof(int), 0);  // reading size of the image
  if (stat < 0)
  {
    perror("Error receiving filestatus");
  }
  if (filestatus == 0)
  {
  	printf("*****************************\n");
  	printf("FILE DOESNOT EXIST ON THE SERVER SIDE\n");
  	printf("*****************************\n");
  }
  if (filestatus == 1 || filestatus == -1)
  {
	  stat = recv(sockfd, &size, sizeof(int), 0);  // reading size of the image
	  if (stat < 0)
	  {
	    perror("Error receiving Size");
	  }
	  printf("size of the file is %d\n", size);
	  //exit(1);

	  //strncat(filename_received, filename, 100);
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
	    if (read_size == 0)
	    {
	    	perror("Problem with the server");
	    	break;
	    }
	    printf("readsize is %d \n", read_size);
	    printf("size is %d \n", size);
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
	else
	{
		return 0;
	}
  

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

int sendUserDetails(int sockfd1)
{
	// do this to ignore SIGPIPE error
	
	int n, ack_putfile;
    readDfcconfig(0);  // only reads username and password
	printf("USERNAME %s\n", USERNAME);
	printf("PASSWORD %s\n", PASSWORD);

	n = send(sockfd1, USERNAME, 100, 0);
	if (n < 0)//, (struct sockaddr *)&servAddr, sizeof(servAddr))==-1)
	{
		perror("option sending failed");
	}

	n = send(sockfd1, PASSWORD, 100, 0);
	if (n < 0)//, (struct sockaddr *)&servAddr, sizeof(servAddr))==-1)
	{
		perror("option sending failed");
	}

	//receiving the status for username and passwords sent
	//printf("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&7\n");
	n = recv(sockfd1, &ack_putfile, sizeof(int), 0);    // Reading the option from user side
    if (n < 0)
    {
      perror("ack receiving failed");
      //exit(1);
    }  
    //printf("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&7\n");
    if (!ack_putfile)
    {
    	printf("INAVLID USERNAME/PASSWORD");
    	//exit(1);
    	return 0;
    }
    printf("***************************mama coming here\n");
    return 1;
}

void computeMd5sum(char *filename, char md5sum[100])  {
    //char md5sum[100];
    char systemmd5Cmd[100];
    strncpy(systemmd5Cmd, "md5sum ", sizeof("md5sum ")); 
    strncat(systemmd5Cmd, filename, strlen(filename));
    FILE *f = popen(systemmd5Cmd, "r");
    while (fgets(md5sum, 100, f) != NULL) {
	strtok(md5sum,"  \t\n");
        //printf( "%s %d\n", md5sum, strlen(md5sum) );
    }
    pclose(f);
}

void LSGetFiles(char filename[100])  {
	printf("file name %s\n", filename);
  //   char fileLs[100];
  //   char systemLSgetFiles[100];
  //   strncpy(systemLSgetFiles, "ls ", strlen("ls ")); // ls 
  //   strncat(systemLSgetFiles, filename, strlen(filename)); // ls [filename]
  //   strncat(systemLSgetFiles, "*_*", strlen("*_*"));
  //   char fileList[4][100];
  //   printf("systemLSgetFiles %s\n", systemLSgetFiles);
  //   FILE *f = popen(systemLSgetFiles, "r");
  //   int i = 0;
  //   while (fgets(fileLs, 100, f) != NULL) {
  //   	bzero(fileList[i], sizeof(fileList[i]));
		// strtok(fileLs,"  \t\n");
		// strncpy(fileList[i], fileLs, strlen(fileLs));
  //       //printf( "%s %d\n", md5sum, strlen(md5sum) );
  //       printf( "%s %d\n", fileList[i], strlen(fileList[i]) );
  //       i++;
  //   }
  //   pclose(f);
}

/*****************************************************************
 * main() starts the operations based on the commands
 
 ARGUMENTS
 int argc     - argument count  
 char **argv  - [ipaddress] [PORT NUMBER]
 ******************************************************************/
int main(int argc, char **argv)
{
	
	//exit(1);
	signal(SIGPIPE, SIG_IGN);
	char md5sum[100];
	int md5sumInt;
	int sendFailedArray[MAX_CONNECTIONS];
	int listCheck[MAX_CONNECTIONS] = {0, 0, 0, 0};
	int dummy = 0;
	// This function reads server info and user credentials
	readDfcconfig(1);
	// This function create four sockets reading the address from the dsc.conf
	createMultipleSockets();
	//char *putFileName1;
	char *subFolder;
	int i;
	int option = 0;      
	char getFileName[20];
	char putFileName[20];//, putFileName1[50];
	int n;
	FILE *picture;
	int j;
	int ack_putfile;
	int md5sumIndex;
	int finalIndex;
	char *val1;
	char subfolder[50];
	char fileNameList[10];
	while(1)
	{
		//
		option = user_commands(); // selecting the command received
		printf("option entered is %d\n", option);

		printf("Option Sent !!\n");

#if 1
		for (i=0;i<MAX_CONNECTIONS;i++)
		{
			printf("**Option Sent !!\n");
			int n = send(sockfd[i], (void *)&option, sizeof(int), 0);	
			printf(" ....%d\n", n);
			if (n < 0)//, 0, (struct sockaddr *)&servAddr, sizeof(servAddr))==-1)
			{
				sendFailedArray[i] = TRUE;
				perror("Writing to socket: option sending failed");
			}
			else
			{
				sendFailedArray[i] = FALSE;
			}
		}
#else
			int n = send(sockfd[0], (void *)&option, sizeof(int), 0);	
			printf(" ....%d\n", n);
			if (n < 0)//, 0, (struct sockaddr *)&servAddr, sizeof(servAddr))==-1)
			{
				perror("Writing to socket: option sending failed");
			}

#endif
		printf("Option Sent !!\n");
		// Sending the commad to server based on the option received
		int j;
		switch(option)
		{
			// This command gets the file from server
			case GET_FILE:
				// choosing the filename
				printf("Enter the file name and subfoolder you wish to receive from\n");
				scanf("%s", getFileName);
				scanf("%s", subfolder);
				printf("The file name entered is %s\n", getFileName);
				printf("subfolder %s\n", subfolder);

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

			    // // coptimizing get
			    // for (i<0;i<MAX_CONNECTIONS;i++)
			    // {

			    // }
			   #if 1
			    for (i = 0; i< MAX_CONNECTIONS; i++)
			    #else
			    	for (i = 0; i< 1; i++)
			   #endif
			    //tempVali = i;

			    {
			    	if (!sendFailedArray[i])
			    	{

			    		if (sendUserDetails(sockfd[i])){
			    			// receivinf image 1
					    	n = send(sockfd[i], getFileName, 50, 0);
							if (n < 0)//, (struct sockaddr *)&servAddr, sizeof(servAddr))==-1)
							{
								perror("option sending failed");
							}
							receive_image(sockfd[i], getFileName, servAddr[i], servlen[i], subfolder);
							printf("**********************\n");
							//sleep(1);
							// receiving image 2
							n = send(sockfd[i], getFileName, 50, 0);
							if (n < 0)//, (struct sockaddr *)&servAddr, sizeof(servAddr))==-1)
							{
								perror("option sending failed");
							}
							receive_image(sockfd[i], getFileName, servAddr[i], servlen[i], subfolder);
							printf("**********************\n");
						}

						//}

					}

		
				}


				// start of concatenation
			    char fileLs[100];
    			char systemLSgetFiles[100];
    			char decryptSystemCmd[300];
    			bzero(systemLSgetFiles, sizeof(systemLSgetFiles));
			    strncpy(systemLSgetFiles, "ls -a .", strlen("ls -a .")); // ls 
			    strncat(systemLSgetFiles, getFileName, strlen(getFileName)); // ls [filename]
			    strncat(systemLSgetFiles, "*_rec*", strlen("*_rec*"));
			    char fileList[4][100];
			    printf("systemLSgetFiles %s\n", systemLSgetFiles);
			    FILE *f = popen(systemLSgetFiles, "r");
			    int i = 0;
			    while (fgets(fileLs, 100, f) != NULL) {
			    	bzero(fileList[i], sizeof(fileList[i]));
			    	
					strtok(fileLs,"  \t\n");
					//printf("fileLs %s\n", fileLs);
					strncpy(fileList[i], fileLs, sizeof(fileLs));
			        printf( "%s %lu\n", fileList[i], strlen(fileList[i]) );
			        i++;
			    }
			    pclose(f);
			    //printf("value of i %d\n", i);
			    bzero(decryptSystemCmd, sizeof(decryptSystemCmd));
			    readDfcconfig(0);
			    if (i<MAX_CONNECTIONS)
			    {
			    	printf("FILE INCOMPLETE TO RESONSTRUCT\n");
			    }
			    else
			    {
			    	char catCommand[300];
			    	for(i=0;i<MAX_CONNECTIONS;i++)
			    	{
			    		sprintf(decryptSystemCmd,"openssl enc -d -aes-256-cbc -in %s -out de%s -k %s", fileList[i], fileList[i], PASSWORD);
			    		printf("mama here%s\n", decryptSystemCmd);
			    		system(decryptSystemCmd);
			    		//exit(1);

			    	}
			    		
			    	sprintf(catCommand,"cat de%s de%s de%s de%s > %s_received", fileList[0],fileList[1],fileList[2],fileList[3], getFileName);
			    	//printf("wow %s\n", catCommand);
			    	system(catCommand);
			    	printf("FILE CONCAT SUCCESSFUL\n");
			    	bzero(catCommand, sizeof(catCommand));
			    	system("rm .foo10_received .foo11_received .foo12_received .foo13_received");
			    }

			    bzero(decryptSystemCmd,sizeof(decryptSystemCmd));
				for (i=0;i<MAX_CONNECTIONS;i++)
				{
					bzero(fileList[i],sizeof(fileList[i]));
				}
				//
			    // system("rm .foo10_received .foo11_received .foo12_received .foo13_received");
				//exit(1);
				break;

			// This command puts the file onto server
			// USERNAME send
			// password send
			// ack for validity of user credentials receive
			// size send
			// filename send
			// fileContent send

			case PUT_FILE:

				printf("Enter the file name and subfolder you wish to send to\n");
				scanf("%s", putFileName);
				scanf("%s", subfolder);

				printf("putFileName %s\n", putFileName);
				printf("subfolder %s\n", subfolder);

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
			   	
			    // computes the md5sum of the file and is excuted on if the valid filename is entered
			    computeMd5sum(putFileName, md5sum);
				md5sumInt = md5sum[strlen(md5sum)-1] % 4;

				md5sumIndex = (4-md5sumInt)%4;
				printf("md5sumIndex %d\n", md5sumIndex);
			    // split -n 4 -a 1 -d [filename] [filename_withindex]
			    printf("Dividing the file into four equal parts...\n");
			    char systemCommand[150];
			    char filename[100];
			    bzero(filename, sizeof(filename));
			    strncpy(filename,putFileName,sizeof(putFileName));
			    //strncpy(systemCommand, "split -n 4 -a 1 -d ", st("split -n 4 -a 1 -d "));
			    //strncat(filename, " ", sizeof(" "));
			    strncpy(filename, putFileName,strlen(putFileName));
			    sprintf(systemCommand,"split -n 4 -a 1 -d %s en%s",putFileName, putFileName);
			    //strncat(systemCommand, filename, sizeof(filename));
			    printf("%s\n", systemCommand);
			    system(systemCommand);

			    printf("Dividing done\n\n");
			    printf("Starting Encryption...\n");
			    printf("\nUSING AES Encryption\n");
			    char encryptSystemCmd[200];
			    bzero(encryptSystemCmd,sizeof(encryptSystemCmd));
			    readDfcconfig(0);
			    for (i=0;i<MAX_CONNECTIONS;i++)
			    {
			    	sprintf(encryptSystemCmd,"openssl enc -aes-256-cbc -in en%s%d -out %s%d -k %s", putFileName,i, putFileName, i, PASSWORD);
			    	system(encryptSystemCmd);
			    	printf("%s\n", encryptSystemCmd);
			    }

			    char filenameWithIndex[4][100];
			    char fileIndex[1];
			    // Sending the file to server
			    /*******************************************
			    // Formulas for how finalIndex is calculated
			    // finalIndex = (i+md5sumIndex)%4
			    // md5sumIndex = (4-md5sumInt)%4;
			    // md5sumInt = md5sum[strlen(md5sum)-1] % 4;
			    //*******************************************/
			    #if 1
			    	for (i = 0; i< MAX_CONNECTIONS; i++)
			    #else
			    	for (i = 0; i< 1; i++)
			    #endif
			    {
			    	if (!sendFailedArray[i])
			    	{
				    	if (sendUserDetails(sockfd[i]))
				    	{
					    	//creating the file name with index
					    	// first file
					    	finalIndex = (i+md5sumIndex)%4;
					    	printf("***********************            %d  %d\n", finalIndex, (finalIndex+1)%4);
					    	strncpy(filenameWithIndex[finalIndex], putFileName, sizeof(putFileName));
					    	sprintf(fileIndex,"%d",finalIndex);
					    	printf("fileIndex  :   %s\n", fileIndex);
					    	strncat(filenameWithIndex[finalIndex], fileIndex, 1);
					    	printf("filename %s\n", filenameWithIndex[finalIndex]);
					    	 
							send_image(sockfd[i], filenameWithIndex[finalIndex], servAddr[i], subfolder);
							sleep(1);
							// seconf file
							strncpy(filenameWithIndex[(finalIndex+1)%4], putFileName, sizeof(putFileName));
					    	sprintf(fileIndex,"%d",(finalIndex+1)%4);
					    	printf("fileIndex  :   %s\n", fileIndex);
					    	strncat(filenameWithIndex[(finalIndex+1)%4], fileIndex, 1);
					    	printf("filename %s\n", filenameWithIndex[(finalIndex+1)%4]);
							send_image(sockfd[i], filenameWithIndex[(finalIndex+1)%4], servAddr[i], subfolder);
							
							bzero(filenameWithIndex[finalIndex], sizeof(filenameWithIndex[finalIndex]));
							bzero(filenameWithIndex[(finalIndex+1)%4], sizeof(filenameWithIndex[(finalIndex+1)%4]));
							bzero(fileIndex, sizeof(fileIndex));
						}
					}
				}

				break;

			// This command gets the list of files form the server in its current directory
			case LIST_FILES:
				system("rm .list0_received .list1_received .list2_received .list3_received");
				printf("Enter the subfolder\n");
				scanf("%s", subfolder);
				

			// //sendFailedArray
			// 	for (i=0;i<MAX_CONNECTIONS;i++)
			// 	{
			// 		printf("**Option Sent !!\n");
			// 		int n = send(sockfd[i], (void *)&dummy, sizeof(int), 0);	
			// 		printf(" ....%d\n", n);
			// 		if (n < 0)//, 0, (struct sockaddr *)&servAddr, sizeof(servAddr))==-1)
			// 		{
			// 			sendFailedArray[i] = TRUE;
			// 			perror("Writing to socket: option sending failed");
			// 		}
			// 		else
			// 		{
			// 			sendFailedArray[i] = FALSE;
			// 			break;
			// 		}
			// 	}
				int status =0;
				int checkNoCnnections = 0; 
				// this part should be uncommented
				for (i =0; i<MAX_CONNECTIONS;i++)
				{
					sprintf(fileNameList, "list%d", i);
					//printf("fileNameList %s\n", fileNameList);
					if (!sendFailedArray[i])
					{ 
						if (sendUserDetails(sockfd[i]))
			    		{
						
							n = send(sockfd[i], fileNameList, 50, 0);
							if (n < 0)//, (struct sockaddr *)&servAddr, sizeof(servAddr))==-1)
							{
								perror("option sending failed");
							}
							//exit(1);
							status = receive_image(sockfd[i], fileNameList, servAddr[i], servlen[i], subfolder);
							if (status == 0)
							{
								checkNoCnnections++;
							}
							printf("\n\nLOOPPP\n");
						}
						//	exit(1);
					}
					bzero(fileNameList, sizeof(fileNameList));
				}

				if (checkNoCnnections == 3)
				{
					printf("ALL SERVERS CLOSED\n");
				}
		        // creating files if not present
		        
		        system("touch .list0_received");
		        system("touch .list1_received");
		        system("touch .list2_received");
		        system("touch .list3_received");
		        // concat the files present
		        system("cat .list0_received .list1_received .list2_received .list3_received > temp_list");
		        system("sort temp_list | uniq > final_list");
		      
		        FILE *fp;
		        char wsBuf[200];
		        char *val1;
		        char array[256][256];
		        char array_withoutIndex[256][256];
		        char necessary_files[256][256];

		        int p = 0, q = 0;

		        FILE *fr;
				fr = fopen("final_list","r");
                if (fr != NULL)
                {
                    char line_string[500]; /* Buffer to store the contents of each line */
                    int i = 0;
                    char filename_string[500];
                    char filename_string1[500];
                    int subfile_count = 0;
                    while(fgets(line_string, sizeof(line_string), fr) != NULL)
                    {
                        if(strncmp(line_string,".",strlen(".")) == 0)
                        {
                            if(strlen(line_string)>3)
                            {
                                char *line_ptr; /* Pointer to the start of each line */
                                line_ptr = strstr((char *)line_string,".");
                                line_ptr = line_ptr + strlen(".");
                                if (subfile_count == 0)
                                {
                                    bzero(filename_string,sizeof(filename_string));
                                    memcpy(filename_string,line_ptr,strlen(line_ptr)-2);
                                }
                                bzero(filename_string1,sizeof(filename_string1));
                                memcpy(filename_string1,line_ptr,strlen(line_ptr)-2);
                                if(strcmp(filename_string,filename_string1) == 0)
                                {
                                    subfile_count = subfile_count +1;
                                    if(subfile_count == 4)
                                        printf("%s\n",filename_string);
                                }
                                else
                                {
                                    if(subfile_count != 4)
                                        printf("%s [incomplete]\n",filename_string);
                                    subfile_count = 1;
                                    strcpy(filename_string,filename_string1);

                                }
                            }
                        }
                    }
                    if(subfile_count != 4)
                        printf("%s [incomplete]\n",filename_string);
                }
                fclose(fr);

		     //    else
		     //    {

		     //        unsigned int wsConfigFileSize = get_file_size (fp);

		     //        printf("final_list size n = %d, filename = final_list \n", wsConfigFileSize);

		     //        while(fgets(wsBuf,wsConfigFileSize,fp)!=NULL) {
		     //        	  //printf("%c\n", wsBuf[strlen(wsBuf)-2]);
		     //        		//printf("%d\n", strlen(wsBuf));
		     //              if ((wsBuf[strlen(wsBuf)-2] >= '0') &&(wsBuf[strlen(wsBuf)-2] <= '3'))
		     //              {
		     //                sprintf(array[p],"%s",wsBuf);
		     //                strncpy(array_withoutIndex[p], array[p], strlen(wsBuf)-2);
		     //                printf("-- %s\n", array_withoutIndex[p]);
		     //                p++;
		     //              }

		     //        }

		     //        fclose(fp);
			    // }

			    //     // Pick all elements one by one
   				//  for (int p=0; p<n; i++)
	    		// 	{
	      //   		// Check if the picked element is already printed
				   //      int j;
				   //      for (j=0; j<i; j++)
				   //         if (arr[i] == arr[j])
				   //             break;
				 
				   //      // If not printed earlier, then print it
				   //      if (i == j)
				   //        cout << arr[i] << " ";
	    		// 	}
			    // int flag = 1;
			    // int countCheck = 0;
			    // int finalArrayIndex[MAX_CONNECTIONS] = {-1,-1,-1,-1};
			    // int l =0;
			    // char arrayToPrint[50];
			    // int y;
			    // for (i =0;i<p;)
			    // {
			    // 	printf(";;;;;;;\n");
			    // 	//if (flag)
			    // 	//{
				   //  	if (strcmp(array_withoutIndex[i], array_withoutIndex[i+1]) == 0)
				   //  	{
				   //  		//printf("%s - complete\n", array[i]);
				   //  		finalArrayIndex[array[i][strlen(array[i]) - 2] - 48] = 1;
				   //  		//printf("___________________________________________\n");
				   //  		countCheck++;
				   //  		finalArrayIndex[array[i+1][strlen(array[i+1]) - 2] - 48] = 1;
				   //  		//printf("___________________________________________\n");
				   //  		//printf("%s - complete\n", array[i+1]);
				   //  		countCheck++;
				   //  		if (strcmp(array_withoutIndex[i], array_withoutIndex[i+2]) == 0)
				   //  		{
				   //  			//printf("%s - complete\n", array[i+2]);
				   //  			//printf("___________________________________________\n");
				   //  			finalArrayIndex[array[i+2][strlen(array[i+2]) - 2] - 48] = 1;
				   //  			countCheck++;
				   //  		}
				   //  		if (strcmp(array_withoutIndex[i], array_withoutIndex[i+3]) == 0)
				   //  		{
				   //  			//printf("%s - complete\n", array[i+3]);
				   //  			//i++;
				   //  			//printf("coming here\n");
				   //  			finalArrayIndex[array[i+3][strlen(array[i+3]) - 2] - 48] = 1;
				   //  			countCheck++;
				   //  		}

				   //  		if (countCheck == 4)
				   //  		{	
							// 	 for (y=1;y<strlen((array[i])-3);y++)
							// 	 {
							// 	 	printf("%c\n", array[i][y]);
							// 	 }
							// 	//printf("%s COMPLETE\n", array[i]);

							// }
							// else
							// {
							// 	 for (y=1;y<strlen((array[i])-3);y++)
							// 	 {
							// 	 	printf("%c\n", array[i][y]);
							// 	 	printf("***************\n");
							// 	 }
							// 	//printf("%s INCOMPLETE\n",array[i]);
							// }
				   //  		// for (l =0;l<MAX_CONNECTIONS;l++)
				   //  		// {
				   //  		// 	printf("finalArrayIndex [%d] %d\n",l,finalArrayIndex[l] );
				   //  		// 	if (finalArrayIndex[l] == 1)
				   //  		// 	{
				   //  		// 		sprintf(arrayToPrint, "%s%d",array_withoutIndex[i], l);
				   //  		// 		printf("%s COMPLETE\n", arrayToPrint);
				   //  		// 		finalArrayIndex[l] = -1;
				   //  		// 	}
				   //  		// 	else
				   //  		// 	{
				   //  		// 		sprintf(arrayToPrint, "%s%d",array_withoutIndex[i], l);
				   //  		// 		printf("%s INCOMPLETE\n", arrayToPrint);
				   //  		// 		finalArrayIndex[l] = -1;
				   //  		// 	}
				   //  		// }
				   //  		i = i + countCheck;
				   //  	}
				   //  	else
				   //  	{
				   //  		i++;
				   //  	}

			    	
			    // }



			 
			 //    computeMd5sum("foo1", md5sum);
				// md5sumInt = md5sum[strlen(md5sum)-1] % 4;

				// md5sumIndex = (4-md5sumInt)%4;
				// printf("md5sumIndex %d\n", md5sumIndex);			
				// for (i=0; i<MAX_CONNECTIONS;i++)
				// {
				// 	if (!sendFailedArray[i])
				// 	{
				// 		listCheck[(i + md5sumIndex)%4]++;
				// 		listCheck[(i + md5sumIndex+1 )%4]++;
				// 	}
				// }

				// for (i = 0;i<MAX_CONNECTIONS;i++)
				// {
				// 	printf("listCheck[%d] %d\n",i, listCheck[i] );	
				// }
				// bzero(listCheck, sizeof(listCheck));
				// bzero(sendFailedArray, sizeof(sendFailedArray));
				// receive the file name babes
				
			break;

			default:
			break;

		}
	}


	close(sockfd);
	return 0;
}
