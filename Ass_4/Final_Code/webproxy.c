/***********************************************************************************************************************
* This proxy implemets WEBPROXY  

SUPPORTS: GET requets
          HTTP 1.0 and HTTP 1.1
          Link prefetching implemented

WEBPROXY implementation:

        1. The webproxy listens to connections from clients such as firefox by acting as a webserver to the clients.
        2. Supports GET Method
        3. The GET request is parsed on passed on to get from server
        4. Every File is stored with its hash balue for maintaing uniformity
        4. If the url requested is available in the CACHE, and if the cache timeout has not expired, then the data is served from the cache
        6. The data from the web also is stored with timestamp to serve the same data in case it is requested again.

CACHE implementation:
    
        1. All the downloaded files are stored into folder <./cache>
        2. The md5sum value of the url is checked in the cache memory
        3. If the filename with same name is not present, then filename with the md5sum hash is created with the contents of the url obtained from the web.
        4. If the file is present and if the cache is not expired, then open the file, read the contents and send to client.
        6. If either file is not present or if cache timeout is expired, then fetch the data from the web.

NOTE: 
The socket creation and fork operaion of main() and a part of respond() is inspired from the follwing link
Ref : http://blog.abhijeetr.com/2010/04/very-simple-http-server-writen-in-c.html
***********************************************************************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>
#include <openssl/md5.h>
#include <time.h>

// defines for connection
#define MAX_CONNECTIONS 1000
#define BYTES 1024
#define MSG_SIZE 99999
#define wsMaxSize 1000

// Errors
#define GETADDRINFOERROR 1
#define BINDERROR        1
#define LISTENERROR      1
#define SLOTERROR        0
#define PORTERROR        1
#define WSCONFIGERROR    1
#define SOCKETCLOSE		 0

#define TRUE             1
#define FALSE            0
#define SUCCESS          1
#define FAIL             0


#define MAX_BUFFER_SIZE 9999

int TIMEOUT;
char *ROOT_DIR;
int listenfd, clients[MAX_CONNECTIONS];
void error(char *);
void startWebServer();
void respond(int sockfd, char *timeout, char *pwd);
char * checkContentType(char *);
char PORT[10];
struct itimerval timeout;

/**************************
Handles Time Out
**************************/
int globalClientNumber;

/***********************
Gets file size
Input Arguments fileDescriptor
return size of the file
************************/
static unsigned int get_file_size (FILE * fileDescriptor)
{
    unsigned int size;

    fseek(fileDescriptor, 0L, SEEK_END);
    size = ftell(fileDescriptor);
    fseek(fileDescriptor, 0L, SEEK_SET);

    return size;
}



/************************************************
starting TCP socket and binding it
Input Arguments NONE
return NONE
************************************************/
void startWebServer()
{
    struct addrinfo webServerHints, *res, *p;
    memset (&webServerHints, 0, sizeof(webServerHints)); // Making sure the struct is empty
    webServerHints.ai_family = AF_INET;                  // IPv4
    webServerHints.ai_socktype = SOCK_STREAM;            // TCP stream sockets
    webServerHints.ai_flags = AI_PASSIVE;
    int s = 1;
    //sprintf(port_num_array, "%d", port);

    /*int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints,
                struct addrinfo **res);
    Given node and service, which identify an Internet host and a service, 
    getaddrinfo() returns one or more addrinfo structures, each of which contains an Internet address that can be specified in a call to bind(2) or connect(2). 
    */

    // getting address
    if (getaddrinfo( NULL, PORT, &webServerHints, &res) != 0)
    {
        perror ("ERROR in getaddrinfo()");
        exit(GETADDRINFOERROR);
    }
    // Bind the cock address
    for (p = res; p!=NULL; p=p->ai_next)
    {
        if ((listenfd = socket (p->ai_family, p->ai_socktype, 0)) == -1)
        {
            continue;
        }
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break;
    }

    if (p==NULL)
    {
        perror ("socket() or bind() creation failed");
        exit(BINDERROR);
    }

    freeaddrinfo(res);
    
    // setting the socket property to reuse
    if (setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&s,sizeof(int)) == -1) 
    {
        printf("setsockopt error");
        exit(1);
    }
  
    // listen for incoming connections
    if ( listen (listenfd, MAX_CONNECTIONS) != 0 )
    {
        perror("listen() error");
        exit(LISTENERROR);
    }
}

/************************************************
Finds the creation time of the file passed 
Input arguments
    pwd: filename
    timecreated: returns the time value created for the file
************************************************/

void getFileCreationTime(char *pwd, char timeCreated[1000]) {
    //printf("Coming in ti getFileCreationTime\n");
    struct stat attr;
    stat(pwd, &attr);
    char date[10];
    sprintf(timeCreated,"%s",ctime(&attr.st_mtime));

}


/************************************************
Computed the md5sum of the given file
Input arguments
    str: filename
    length of the md5sum request 
************************************************/

char *computeMD5(const char *str, int length) 
{
    int n;
    MD5_CTX c;
    unsigned char digest[16];
    char *out = (char*)malloc(33);

    MD5_Init(&c);

    while (length > 0) {
        if (length > 512) {
            MD5_Update(&c, str, 512);
        } else {
            MD5_Update(&c, str, length);
        }
        length -= 512;
        str += 512;
    }

    MD5_Final(digest, &c);

    for (n = 0; n < 16; ++n) {
        snprintf(&(out[n*2]), 16*2, "%02x", (unsigned int)digest[n]);
    }

    return out;
}

/*************************************************************************

This function performs the link prefetching operation
check for the content href:"http://
I exists copies the url and makes it ready to extract from the server

Input arguments 
    pwd: current working directory
    sockfd: socket descriptor of the clinet 

return - 

****************************************************************************/

#define FILE_SIZE 1000000

void linkPrefetching(char * pwd, int sockfd)
{
    printf("#####     Entering prefetching    #####\n");
    
    char bufTosend[FILE_SIZE];
    char filepath[MAX_BUFFER_SIZE];
    char prefeReq[MAX_BUFFER_SIZE];
    char buffer[MAX_BUFFER_SIZE];
    int readFile,n;
    char check[MAX_BUFFER_SIZE];
    int i,openFile;
    char *md5;  
    int flag;
    FILE *fp;   
    char newVar[MAX_BUFFER_SIZE];
    char * ret;
    char *retVal;
    char path[MAX_BUFFER_SIZE];
    struct sockaddr_in host_addr;
    struct hostent* host;
    int p=9;

    bzero(bufTosend,sizeof(bufTosend));
    bzero(filepath,sizeof(filepath));
    bzero(prefeReq,sizeof(prefeReq));
    bzero(check,sizeof(check));
    bzero(newVar,sizeof(newVar));
    bzero(path,sizeof(path));
    bzero(buffer,sizeof(buffer));

    // opening the file from current pwd
    openFile=open(pwd,O_RDONLY);
    if(openFile==-1){
        printf("\n error in reading the file");
    }

    // reading file
    readFile = read(openFile,bufTosend, sizeof bufTosend);
    if (readFile < 0)
    {
        printf("FILE reading error\n");
    }

    char *newPtr;

    // check if href is present
    if((ret=strstr(bufTosend,"href=\"http://"))!= NULL){
        while((ret=strstr(ret,"href=\"http://")) ){
            ret = ret+13;
            i=0;

            // extracting the http url after href command
            while(*ret!='"')
                {
                    newVar[i] = *ret;   
                    printf("%c",*ret);
                    ret++;
                    i++;
                }
            newPtr=ret;
            newVar[i]='\0';

            // computing the md5sum of the file 
            strcpy(check, newVar);
            md5 = computeMD5(check,strlen(check));
            

            printf("\n md5 sum of the link prefetching file:%s",md5);
            strcpy(filepath,"./cachedir/");
            strcat(filepath,md5);
            strcat(filepath,".html");
            printf("filepath: %s",filepath);
            retVal = strstr(newVar,"/");
            if ( retVal == NULL){
              continue;
            }
            if(retVal!=NULL){
                strcpy(path,retVal);
            }
            
            *retVal='\0';
            ret =newPtr +1;

            printf("\nGET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n",path,newVar);
            sprintf(prefeReq,"GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n",path,newVar);
            
            // sending request to server
            n = send(sockfd,prefeReq,strlen(prefeReq),0);
            printf("\n sending request");
            
            fp=fopen(filepath,"w");
            do{
                printf("\n getting written into file"); 
                // receiving the file from server
                n=recv(sockfd,buffer,500,0);

                // writing the received contents into a file
                printf("\nbuffer is: %s\n",buffer);
                fwrite(buffer,1,n,fp);
            }while(n>0);

            memset(pwd,0,0);
        }
    }


}


/*************************************************************************

This function receives request from client
Parses the request in appropriate format

Checks if present in cache 
else:
    sends the request to server (Ex www.google.com)

Responds back the client by offering the service requested


Input arguments: 
    int sockfd - socket descriptor as server
    char *timeout - desired value of timeout in string format
    char *pwd - The current working directory

return - 

****************************************************************************/
int COUNT = 0;

void respond(int sockfd, char *timeout, char *pwd)
{
    int cache;

    char readBufferFrmClient[MAX_BUFFER_SIZE];
    char readBufferFrmServer[MAX_BUFFER_SIZE];
    char method[MAX_BUFFER_SIZE];
    char path[MAX_BUFFER_SIZE];
    char http_ver[MAX_BUFFER_SIZE];
    char requesToHost[MAX_BUFFER_SIZE];
    char md5sum[100];
    char website[MAX_BUFFER_SIZE];
    char *url = NULL;
    char *websiteWithSlash = NULL;
    char *websiteWithSlash1;
    char buff[MAX_BUFFER_SIZE];
    struct hostent *hostToconnect;
    struct sockaddr_in hostAddr;
    int on = 1;
    int findcache;

    int hostfd;    //connects to host
    int nbytes,sbytes;      //
    int len;

    char filename[MAX_BUFFER_SIZE];
    FILE *fileProxy;


    bzero(readBufferFrmClient, sizeof(readBufferFrmClient));
    bzero(method, sizeof(method));
    bzero(path, sizeof(path));
    bzero(http_ver, sizeof(http_ver));
    bzero(website, sizeof(website));
    bzero(md5sum, sizeof(md5sum));


    char Invalid_Method[MAX_BUFFER_SIZE] = "<html><body><H1>Error 400 Bad Request: Invalid Method </H1></body></html>";
    char Invalid_version[MAX_BUFFER_SIZE] =  "<html><body><H1>Error 400 Bad Request: Invalid HTTP Version</H1></body></html>";
    
    if (read(sockfd, readBufferFrmClient, MAX_BUFFER_SIZE)<0)
    {
        printf("recieve error\n");
    }


    // On receiving valid data
    else
    {
        
        // Extracting 
        // 1. method - request method
        // 2. url
        // 3. HTTP Version

        sscanf(readBufferFrmClient,"%s %s %s",method,path,http_ver);
        printf("readBufferFrmClient: %s %s %s\n",method,path,http_ver); 


        // checking validity of request method
        if (strncmp(method,"GET",strlen("GET")) != 0)
        {
            write(sockfd,Invalid_Method,strlen(Invalid_Method));
            printf("Invalid Request method\n");
            exit(1);
        }

        // checking validity of HTTP method
        else if ((strncmp(http_ver,"HTTP/1.0",strlen("HTTP/1.0")) != 0)  || (strncmp(http_ver,"HTTP/1.0",strlen("HTTP/1.1")) != 0))
        {
            write(sockfd,Invalid_version,strlen(Invalid_version));
            printf("Invalid HTTP Version");
            exit(1);
        }

        else
        {
            int i=0;
            websiteWithSlash= strstr(path,"//");
            websiteWithSlash+=2;

            for(i=0;i<strlen(websiteWithSlash);i++)
            {
                if(websiteWithSlash[i]=='/')
                    break;
                website[i]=websiteWithSlash[i];
            }  
            url=strstr(websiteWithSlash,"/");
            //printf("root: %s url: %s\n",website,url);

            printf("\n website: %s\n", website);
            printf("\n path to url: %s\n", url);

            MD5_CTX mdContext;
            MD5_Init(&mdContext);
            MD5_Update (&mdContext, path, strlen(path));
            MD5_Final (md5sum, &mdContext);


            // computes the md5sum of the file

            for (i = 0; i< MD5_DIGEST_LENGTH; i++)
            {
                sprintf(&buff[2*i],"%02x", md5sum[i]);
            }
  

            sprintf(filename,"%s.html",buff);
            printf("md5sum %s\n", buff);
            sprintf(pwd,"%s%s",pwd, filename);
            printf("pwd with filename%s\n", pwd);

            // find if the file is present in cache or not
            findcache = cachePresent(timeout, pwd, sockfd);
            printf("Printing the status of cache %d\n", findcache);

            // if file doesn't exit on cache get it from server
            if (findcache == FAIL)
            {
                printf("NO CACHE FOUND...\n\n");

                printf("Extracting file from host server\n");

                fileProxy = fopen(pwd,"ab");
                if (fileProxy < 0)
                {
                    perror("FILE ERROR in RESPOND");
                }
                hostToconnect = gethostbyname(website);
                if (!hostToconnect)
                {
                    perror("Inavlid host address");
                    exit(1);
                }

                // parameters to connect to host server
                bzero(&hostAddr,sizeof(hostAddr));                    //zero the struct
                hostAddr.sin_family = AF_INET;                   //address family
                hostAddr.sin_port = htons(80);        //htons() sets the port # to network byte order
                memcpy(&hostAddr.sin_addr, hostToconnect->h_addr, hostToconnect->h_length);
                len = sizeof(hostAddr);
                hostfd = socket(AF_INET, SOCK_STREAM, 0);
                if (hostfd<0)
                {
                    perror("HOST socket creation failed");
                }
                setsockopt(hostfd, SOL_SOCKET, SO_REUSEADDR, &on, 4);   //????????????

                int skt = connect(hostfd, (struct sockaddr *) &hostAddr, len);
                if (skt < 0) 
                {
                    printf("Connection problem\n");
                    close(hostfd);
                }

                // creating url to send to host

                if (url != 0)
                    sprintf(requesToHost,"GET %s %s\r\nHost: %s\r\nConnection: close\r\n\r\n",url,http_ver,website);
                else
                    sprintf(requesToHost,"GET / %s\r\nHost: %s\r\nConnection: close\r\n\r\n",http_ver,website);
           
                printf("requesToHost %s\n", requesToHost);
      
                // sending request to host server- Ex www.google.com
                nbytes = send(hostfd,requesToHost,sizeof(requesToHost),0);

                if (nbytes < 0)
                {
                    perror("Host send failed");
                }

                else
                {
                    printf("\n\nSarted sending file from server...\n\n");
                    do{
                        
                        bzero((char*)readBufferFrmServer,MAX_BUFFER_SIZE);
                       
                        nbytes=recv(hostfd,readBufferFrmServer,sizeof(readBufferFrmServer),0);        // receiving from host server
     
                        
                        if(!(nbytes<=0))
                        {
                            send(sockfd,readBufferFrmServer,nbytes,0);                                 // sending to client 
                            fwrite(readBufferFrmServer,1,nbytes,fileProxy);
                        }
                       
                        
                    }while(nbytes>0);

                    
                    printf("\n\nStarting the Prefeting operation... \n\n");
                    linkPrefetching(pwd,hostfd);
                    printf("linkPrefetching Done\n" );
                }

                fclose(fileProxy);
            }

            else
            {
                //printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
            }
        }

    }
    bzero(Invalid_version,sizeof(Invalid_version));
    bzero(Invalid_Method,sizeof(Invalid_Method));
    close(sockfd);
    close(hostfd);
    printf("Closing the sockets........................\n");
}

int cachePresent(char *timeoutStr, char *pwd, int sockfd)
{
    int timeout = atoi(timeoutStr);
    char timeCreated[1000];
    int nbytes;
    char bufTosend[MAX_BUFFER_SIZE];
    getFileCreationTime(pwd, timeCreated);
    printf("timeCreated %s\n", timeCreated);
    FILE *sendFilefd;

    if( access( pwd, F_OK ) != -1 ) 
    {
        // file available
        char *hours, *minutes,*seconds;

        int hoursInt, minutesInt, secondsInt;
        hours = strtok(timeCreated,":") ;
        
        minutes = strtok(NULL,":") ;
        
        seconds = strtok(NULL,":") ;
        seconds = strtok(seconds, " ");

        hours = strtok(hours," ") ;
        hours = strtok(NULL," ");
        hours = strtok(NULL," ");
        hours = strtok(NULL," ");

        int fileTime = atoi(hours)*3600 + atoi(minutes)*60 + atoi(seconds); 

        time_t current_time;
        time(&current_time);
        bzero(timeCreated,sizeof(timeCreated));
        //strcpy(timeCreated,ctime(&current_time));
        sprintf(timeCreated,"%s", ctime(&current_time));

        printf("presentTime %s\n", timeCreated);

        hours = strtok(timeCreated,":") ;
        
        minutes = strtok(NULL,":") ;
        
        seconds = strtok(NULL,":") ;
        seconds = strtok(seconds, " ");

        hours = strtok(hours," ") ;
        hours = strtok(NULL," ");
        hours = strtok(NULL," ");
        hours = strtok(NULL," ");

        int presentTime = atoi(hours)*3600 + atoi(minutes)*60 + atoi(seconds); 

        printf("Present time %d\n", presentTime);
        printf("File time %d \n", fileTime);
        printf("mama very important %d\n", presentTime - fileTime);
        if (presentTime - fileTime > timeout)
        {
            printf("TIMEOUT EXPIRED\n");
            return FAIL;
        }

        else
        {
            printf("SENDING FILE FROM CACHE MEMORY\n");
            bzero(bufTosend,sizeof(bufTosend));
            printf("Sending cached file\n");
            sendFilefd=fopen(pwd,"r");

            do{
                nbytes = fread(bufTosend,1,MAX_BUFFER_SIZE,sendFilefd);
                send(sockfd,bufTosend,nbytes,0);
            }
            while(nbytes>0);

            fclose(sendFilefd);
            close(sockfd);
            return SUCCESS;

        }
    }

    else
    {
        return FAIL;
    }


}




/**********************************************
The socket creation and fork operaion of main() and a part of respond() is 
referenced from the follwing link
Ref : http://blog.abhijeetr.com/2010/04/very-simple-http-server-writen-in-c.html
**********************************************/

int main(int argc, char* argv[])
{


    char pwd[MAX_BUFFER_SIZE];  // Current working direcroty
    char timeout[10];
    int sock_n;
    if (argc<3)
    {
        printf("Invalid argumants: ./webproxy <port_num> <Cache Timeout>\n");
        exit(1);
    }

    printf("\n\nPORT number is: %s\n\n", argv[1]);
    printf("\n\nCache Timeout is: %s\n\n", argv[2]);

    strcpy(PORT,argv[1]);
    strcpy(timeout,argv[2]);

    int pid;
    int connectionNum=0;
    int i;

    struct sockaddr_in clientAddr;
    socklen_t addrlen;
    char c;    

    char parentdir[MAX_BUFFER_SIZE];
    char cmdCreateCache[MAX_BUFFER_SIZE];


    // Finds the current working directory
    if (getcwd(parentdir, sizeof(parentdir)) != NULL)
    {
        printf("Current working dir: %s\n", parentdir);
        sprintf(pwd,"%s/cache/",parentdir);
        sprintf(cmdCreateCache,"mkdir -p %s",pwd);
        system(cmdCreateCache);
    }
    printf("Current working Directory: %s\n",pwd);


    for (i=0; i<MAX_CONNECTIONS; i++)
    {
        clients[i]=-1;
    }
 
    int port_num = atoi(PORT);

    // Exit if port number less than 1024
    if (port_num < 1024)
    {
        fprintf(stderr, "The port number chosen is %d and is INVALID\n", port_num);
        exit(PORTERROR);
    }
    
    // Used to start the server
    startWebServer();

    int connectionCount = 0;
    // ACCEPT connections
    while (1)
    {
        addrlen = sizeof(clientAddr);
        // accepting the connections and forking it into a child 
        
        clients[connectionNum] = accept (listenfd, (struct sockaddr *) &clientAddr, &addrlen);
        //printf("Coming here again\n");
        if (clients[connectionNum]<0)
            error ("accept() error");
        else
        {
            printf("\n\n####    Connection Accepted: %d    ####\n\n", connectionCount++); 
        }
            pid = fork();               
            if (pid <0)
                printf("Error on Fork !!");
        
            if (pid == 0)
            {
                // The follwing function handles the incoming connections
                respond(clients[connectionNum], timeout, pwd);    
                close(listenfd);
                exit(0);
            } 
            
    }

    return 0;
}