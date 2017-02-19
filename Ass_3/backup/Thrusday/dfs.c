
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <dirent.h>
#include <signal.h>
//#define portNum 21234

#define BUFLEN 10240



#define GET_FILE 1
#define PUT_FILE  2
#define LIST_FILES 3
#define CLOSE_SOC 4

#define VALID 1
#define INVALID 0

typedef struct user_credential{
  char USERNAME[100];
  char PASSWORD[100];
}user_credentials;

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

  user_credentials putfileUser;  // important
/*****************************************************
 * send_image: Transfers the file from Server to Client
 
 ARGUMENTS
 int sockfd - descriptor for socket created  
 char *filename - filename to send from Server to Client
 struct sockaddr_in servAddr  - contains the client informatiom
 *****************************************************/

int send_image(int sockfd, char *filename, struct sockaddr_in clientAddr, int indexFrmPort, int fileNumber){

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

  char dfsFolder[100];
  bzero(dfsFolder, sizeof(dfsFolder));

  // variables for opeing a directory
  DIR           *d;
  struct dirent *dir;
  int count = 0;
  int index = 0;
  char name[256][256];


  char dirToOpenWithUser[100];
  switch(indexFrmPort)
  {
    case 1:
      strncpy(dfsFolder, "DFS1", sizeof("DFS1"));
      strncpy(dirToOpenWithUser, "./DFS1/", sizeof("./DFS1/"));
      strncat(dirToOpenWithUser, putfileUser.USERNAME, strlen(putfileUser.USERNAME));
      d = opendir(dirToOpenWithUser);
      //system("mkdir -p DFS1");
    break;
    case 2:
      strncpy(dfsFolder, "DFS2", sizeof("DFS2"));
      strncpy(dirToOpenWithUser, "./DFS2/", sizeof("./DFS2/"));
      strncat(dirToOpenWithUser, putfileUser.USERNAME, strlen(putfileUser.USERNAME));
      d = opendir(dirToOpenWithUser);;
      //system("mkdir -p DFS2");
    break;
    case 3:
      strncpy(dfsFolder, "DFS3", sizeof("DFS3"));
      strncpy(dirToOpenWithUser, "./DFS3/", sizeof("./DFS3/"));
      strncat(dirToOpenWithUser, putfileUser.USERNAME, strlen(putfileUser.USERNAME));
      d = opendir(dirToOpenWithUser);
      //system("mkdir -p DFS3");
    break;
    case 4:
      printf("I am coming here\n");
      strncpy(dfsFolder, "DFS4", sizeof("DFS4"));
      strncpy(dirToOpenWithUser, "./DFS4/", sizeof("./DFS4/"));
      strncat(dirToOpenWithUser, putfileUser.USERNAME, strlen(putfileUser.USERNAME));
      d = opendir(dirToOpenWithUser);
      //system("mkdir -p DFS4");
    break;    
  }

  bzero(dirToOpenWithUser, sizeof(dirToOpenWithUser));

  char filename_dummy[100];
  bzero(filename_dummy, sizeof(filename_dummy));
  strcpy(filename_dummy, filename);

  // copies the current list of files in to the array name[count]
  //printf("************************************\n");
  if (d)
  {
    while ((dir = readdir(d)) != NULL)
    {
      
      //strcpy(name[count],dir->d_name);
        //printf("dir->d_name: %s %d\n", dir->d_name, sizeof(dir->d_name));
        //printf("filename_dummy: %s %d\n", filename_dummy, strlen(filename));
      if (strncmp(dir->d_name, filename_dummy, strlen(filename)) == 0)
      {
        printf(" dir->d_name[strlen(filename) %d\n",  dir->d_name[strlen(filename) + 1]);
        if ((dir->d_name[strlen(filename) + 1]>=0) &&  (dir->d_name[strlen(filename) + 1]<=3))
        {
          strcpy(name[count],dir->d_name);
          printf("name[count] %s\n", name[count]);
          count++;
        }
      }

    }

    closedir(d);
  }
  printf("************************************\n");

  
  strncat(dfsFolder,"/",sizeof("/")); //dfs[]/
  strncat(dfsFolder,putfileUser.USERNAME,sizeof(putfileUser.USERNAME)); //dfs[]/USERNAME
  strncat(dfsFolder,"/",sizeof("/")); //dfs[]/USERNAME/
  printf("dfsFolder %s\n", dfsFolder);
  char filenameWithDot[100];
  bzero(filenameWithDot,sizeof(filenameWithDot));
  #if 1
  
  strcpy(filenameWithDot, name[fileNumber]);  // take care over here

  #else
  strncpy(filenameWithDot,".",sizeof("."));
  strcat(filenameWithDot, filename);
  #endif

  // sending the filename from server to client
  send(sockfd, filenameWithDot, sizeof(filenameWithDot), 0);

  //printf("********************filenameWithDot: %s\n", filenameWithDot);
  strncat(dfsFolder,filenameWithDot, sizeof(filenameWithDot));
  printf("dfs folder latest%s\n", dfsFolder);
  //exit(1);

  if (!(picture = fopen(dfsFolder, "r"))) 
  {
    perror("fopen");
    return -1;
  }    

   printf("Finding the size of the file using fseek\n"); 
   
   fseek(picture, 0, SEEK_END);
   size = ftell(picture);
   fseek(picture, 0, SEEK_SET);
   printf("Total file size is: %d\n",size);


   printf("Sending Picture Size from Server to Client\n");
   send(sockfd, (void *)&size, sizeof(int), 0);
    if (stat < 0)
   {
    perror("Error sending size");
    exit(1);
   }


  char sendBuf_withpcktSize[BUFLEN];

   while(!feof(picture)) {
      read_size = fread(send_buffer, 1, sizeof(send_buffer)-1, picture);
 

      do{
   
            stat = send(sockfd, send_buffer, read_size, 0);//, (struct sockaddr *)&clientAddr, sizeof(clientAddr));

  
      }while (stat < 0);

      printf(" \n");
      printf(" \n");


      //Zero out our send buffer
      bzero(send_buffer, sizeof(send_buffer));
      //bzero(sendBuf_withpcktSize, sizeof(sendBuf_withpcktSize));
     }
     bzero(dfsFolder, sizeof(dfsFolder));
}



/*****************************************************************
 * receive_image: Receives the file from Server to Client
 
 ARGUMENTS
 int sockfd - descriptor for socket created  
 struct sockaddr_in servAddr  - contains the server information
 socklen_t clientlen: length of the Server

 Returns 1 on completion
 ******************************************************************/

int receive_image(int sockfd, struct sockaddr_in clientAddr, socklen_t clientlen, int indexFrmPort)
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
  printf("Filename received is %s\n", filename_received);

  char dfsFolder[100];
  bzero(dfsFolder, sizeof(dfsFolder));
  char FolderWithUsername[150];

  // creates two folders 
  // 1. DFS[]
  // 2. folder with USERNAME
  switch(indexFrmPort)
  {
    case 1:
      strncpy(dfsFolder, "DFS1", sizeof("DFS1"));
      system("mkdir -p DFS1");
      strncpy(FolderWithUsername, "mkdir -p DFS1/", strlen("mkdir -p DFS1/"));
    break;
    case 2:
      strncpy(dfsFolder, "DFS2", sizeof("DFS2"));
      system("mkdir -p DFS2");
      strncpy(FolderWithUsername, "mkdir -p DFS2/", strlen("mkdir -p DFS2/"));
    break;
    case 3:
      strncpy(dfsFolder, "DFS3", sizeof("DFS3"));
      system("mkdir -p DFS3");
      strncpy(FolderWithUsername, "mkdir -p DFS3/", strlen("mkdir -p DFS3/"));
    break;
    case 4:
      //printf("I am coming here\n");
      strncpy(dfsFolder, "DFS4", sizeof("DFS4"));
      system("mkdir -p DFS4");
      strncpy(FolderWithUsername, "mkdir -p DFS4/", strlen("mkdir -p DFS4/"));
    break;    
  }

  // creates DFS[]/Username
  strncat(FolderWithUsername, putfileUser.USERNAME, strlen(putfileUser.USERNAME));
  printf("FolderWithUsername %s\n", FolderWithUsername);
  system(FolderWithUsername);

  //printf("dfsFolder %s\n", dfsFolder);
  strncat(dfsFolder,"/",sizeof("/")); //dfs folder with slash command DFS[]/
  strncat(dfsFolder,putfileUser.USERNAME, strlen(putfileUser.USERNAME)); //dfs folder with slash command DFS[]/USERNAME
  //printf("dfsFolder %s\n", dfsFolder);
  char filenameWithDot[100];
  bzero(filenameWithDot,sizeof(filenameWithDot));
  #if 1
    strcpy(filenameWithDot, filename);
  #else
  strncpy(filenameWithDot,".",sizeof("."));
  strcat(filenameWithDot, filename);
  #endif
  strncat(dfsFolder,"/",sizeof("/")); //dfs folder with slash command DFS[]/USERNAME/
  strncat(dfsFolder,filenameWithDot, sizeof(filenameWithDot));
  printf("********************dfs folder: %s\n", dfsFolder);
  // opeing an image in write mode
  image = fopen(dfsFolder, "w");
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
  bzero(FolderWithUsername, sizeof(FolderWithUsername));
  return 1;

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
int checkValidFrmDFSconf(char *USERNAME1, char *PASSWORD1)
{
    FILE *fp;
    char wsBuf[200];
    char *val1;

    int i = 0;
    fp=fopen("dfs.conf","r");
    if (fp == NULL)
    {

        perror("dfs.conf");
        exit(1);
    }

    else
    {

        unsigned int wsConfigFileSize = get_file_size (fp);

        printf("dfc.conf size n = %d, filename = dsc.conf \n", wsConfigFileSize);

        while(fgets(wsBuf,wsConfigFileSize,fp)!=NULL) {

              printf("wsBuf: %s",wsBuf);
              val1=strtok(wsBuf," \t\n");
              if ((strncmp(val1, USERNAME1, strlen(USERNAME1)) == 0)  &&  (strlen(val1)==strlen(USERNAME1)) )
              {

                val1 = strtok(NULL, " \t\n");   
                if ((strncmp(val1, PASSWORD1, strlen(PASSWORD1)) == 0) &&  (strlen(val1)==strlen(PASSWORD1)))
                {
                  return VALID; // valid 
                }
              }

        }

        fclose(fp);
    }

    return INVALID; // invalid
}

int sendAckforUserdetails(int newsockfd)
{

  int n, ack_putfile;   
  n = recv(newsockfd, putfileUser.USERNAME, 100, 0);    // Reading the option from user side
  if (n < 0)
  {
    perror("option receiving failed");
    exit(1);
  }

  n = recv(newsockfd, putfileUser.PASSWORD, 100, 0);    // Reading the option from user side
  if (n < 0)
  {
    perror("option receiving failed");
    exit(1);
  }  

  ack_putfile = checkValidFrmDFSconf(putfileUser.USERNAME, putfileUser.PASSWORD);
  //printf("***************ack_putfile %d\n", ack_putfile);

  n = send(newsockfd, &ack_putfile, sizeof(int), 0);  // reading size of the image
  if (n < 0)
  {
    perror("Error receiving ack for username and password");
  }
  return ack_putfile;
}
/*****************************************************************
 * main() starts the operations based on the commands
 
 ARGUMENTS
 int argc     - argument count  
 char **argv  - [PORT NUMBER]
 ******************************************************************/

#define FIRSTFILE 0
#define SECONDFILE 1
int main(int argc, char **argv)
{

  signal(SIGPIPE, SIG_IGN);
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
  int ack_putfile;
  int indexFrmPort = portno%5;
  //printf("%s\n", );
  printf("indexFrmPort %d\n", indexFrmPort);

	for (;;) {

      n = recv(newsockfd, (void *)&option, sizeof(int), 0);    // Reading the option from user side
      if (n < 0)
      {
        perror("option receiving failed");
        exit(1);
      }          

    //printf("Option %d received\n", option);

    switch(option)
    {
      // This command sends the file to Client 
      case GET_FILE:
          printf("receiving the file name\n");

              if (sendAckforUserdetails(newsockfd))
              {
                stat = recv(newsockfd, getFileName, 50,0);
                 if (stat < 0)
                 {
                  perror("Error sending filename");
                  exit(1);
                 }
                printf("The file name asked for first time is %s\n", getFileName);
                // sending the image

                send_image(newsockfd, getFileName, clientAddr, indexFrmPort, FIRSTFILE);  
                printf("Exiting the GET_FILE\n");

                stat = recv(newsockfd, getFileName, 50,0);
                 if (stat < 0)
                 {
                  perror("Error sending filename");
                  exit(1);
                 }
                printf("The file name asked for second time is %s\n", getFileName);
                // sending the image

                send_image(newsockfd, getFileName, clientAddr, indexFrmPort, SECONDFILE);  
                printf("Exiting the GET_FILE\n");
            }

          option = 10;
      break;

      // This command gets file from Client 
      // USERNAME receive
      // password receive
      // ack for validity of user credentials send
      // size receive
      // filename receive
      // fileContent receive
      case PUT_FILE:
            // indexFrmPort tells which folder to store in
            if (sendAckforUserdetails(newsockfd))
            { 
              // receiving fst image     
              receive_image( newsockfd, clientAddr, clientlen,indexFrmPort); 
              printf("completed putfile case\n");

              // receiving second image
              receive_image( newsockfd, clientAddr, clientlen,indexFrmPort); 
              printf("completed putfile case\n");

            }
            option = 10;
            break;

      // This command send list of files in the current directory to client
      case LIST_FILES:
      // list of files
      // while(1)
      //     {
      //           if (recvfrom(sockfd, &getFileName, 50, 0, (struct sockaddr *)&clientAddr, &clientlen)==-1)
      //           {
      //             perror("option receiving failed");
      //           }

      //           if (getFileName[0]>0 && getFileName[0] <5)
      //           {
      //             //bzero(getFileName, sizeof(getFileName));
                  
      //           }

      //           else
      //           {
      //               printf("The file name asked for is %s\n", getFileName);

      //             // sending the image
      //             system("ls > list.txt"); // using the system command to store the lit of files
      //             printf("Sending the ist of files on server side\n");
      //             send_image(sockfd, "list.txt", clientAddr);
                  
      //             printf("Exiting the GET_FILE\n");
      //             break;
      //           }
      //   }

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
