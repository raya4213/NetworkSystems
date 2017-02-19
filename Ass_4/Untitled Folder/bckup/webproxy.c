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
void respond(int);
char * checkContentType(char *);
char PORT[10];
struct itimerval timeout;

/**************************
Handles Time Out
**************************/
int globalClientNumber;

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



/************************************************
starting TCP socket and binding it
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
    // socket and bind
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

/**********************************************
The socket creation and fork operaion of main() and a part of respond() is 
referenced from the follwing link
Ref : http://blog.abhijeetr.com/2010/04/very-simple-http-server-writen-in-c.html
**********************************************/

char pwd[MAX_BUFFER_SIZE];
int main(int argc, char* argv[])
{

    int pid;
    int connectionNum=0;
    int i;

    struct sockaddr_in clientAddr;
    socklen_t addrlen;
    char c;    

    char parentdir[MAX_BUFFER_SIZE];
    char cmdCreateCache[MAX_BUFFER_SIZE];

    if (getcwd(parentdir, sizeof(parentdir)) != NULL)
    {
        printf("Current working dir: %s\n", parentdir);
        sprintf(pwd,"%s/cache/",parentdir);
        sprintf(cmdCreateCache,"mkdir -p %s",pwd);
        system(cmdCreateCache);
    }
    printf("%s\n",pwd);

    //readWSconfig();     // reads the wsConfigFile
    for (i=0; i<MAX_CONNECTIONS; i++)
    {
        clients[i]=-1;
    }

    strcpy(PORT,"10001");   
    int port_num = atoi(PORT);

    // Exit if port number less than 1024
    if (port_num < 1024)
    {
        fprintf(stderr, "The port number chosen is %d and is INVALID\n", port_num);
        exit(PORTERROR);
    }
    
    // starts the server
    startWebServer();

    // ACCEPT connections
    while (1)
    {
        addrlen = sizeof(clientAddr);
        // accepting the connections and forking it into a child 
        //printf("Coming here\n");
        clients[connectionNum] = accept (listenfd, (struct sockaddr *) &clientAddr, &addrlen);
        //printf("Coming here again\n");
        if (clients[connectionNum]<0)
            error ("accept() error");
        else
        {
           
            pid = fork();               
            if (pid <0)
            printf("Error on Fork !!");
        
            if (pid == 0)
            {
                //close(sock); 
                respond(clients[connectionNum]);    
                exit(0);
            } 
            
        }

        while (clients[connectionNum]!=-1) // supporting 1000 connections
            {
                connectionNum = (connectionNum+1)%MAX_CONNECTIONS;
            }
    }

    return 0;
}


/************************************************
This function performs the HTTP response based 
on the request received
************************************************/
int COUNT = 0;


//void respond(int fd)
void respond(int sockfd)
{
    int cache;

#if (1)
    char readBufferFrmClient[MAX_BUFFER_SIZE];
    char readBufferFrmServer[MAX_BUFFER_SIZE];
    char method[MAX_BUFFER_SIZE];
    char path[MAX_BUFFER_SIZE];
    char http_ver[MAX_BUFFER_SIZE];
    char requesToHost[MAX_BUFFER_SIZE];
    //char url[MAX_BUFFER_SIZE];
    char website[MAX_BUFFER_SIZE];
    char *url = NULL;
    char *websiteWithSlash = NULL;
    char *websiteWithSlash1;
    struct hostent *hostToconnect;
    struct sockaddr_in hostAddr;
    int on = 1;

    int hostfd;    //connects to host
    int nbytes,sbytes;      //
    int len;

    char filename[MAX_BUFFER_SIZE];
    FILE *fileProxy;


    bzero(readBufferFrmClient, sizeof(readBufferFrmClient));
    bzero(method, sizeof(method));
    bzero(path, sizeof(path));
    bzero(http_ver, sizeof(http_ver));
    //bzero(website, sizeof(website));
    //bzero(url, sizeof(url));


    char Invalid_Method[MAX_BUFFER_SIZE] = "<html><body><H1>Error 400 Bad Request: Invalid Method </H1></body></html>";
    char Invalid_version[MAX_BUFFER_SIZE] =  "<html><body><H1>Error 400 Bad Request: Invalid HTTP Version</H1></body></html>";
    if (read(sockfd, readBufferFrmClient, MAX_BUFFER_SIZE)==-1)
    {
        printf("recieve error\n");
    }

    else
    {
        
        sscanf(readBufferFrmClient,"%s %s %s",method,path,http_ver);
        printf("readBufferFrmClient: %s %s %s\n",method,path,http_ver); 

        if (strncmp(method,"GET",strlen("GET")) != 0)
        {
            write(sockfd,Invalid_Method,strlen(Invalid_Method));
            perror("Invalid Request method");
            exit(1);
        }

        else if (strncmp(http_ver,"HTTP/1.0",strlen("HTTP/1.0")) != 0)
        {
            write(sockfd,Invalid_version,strlen(Invalid_version));
            perror("Invalid HTTP Version");
            exit(1);
        }

        else
        {
            printf("Entering valid loop\n");
            int i=0;
            websiteWithSlash= strstr(path,"//");
            websiteWithSlash+=2;

            printf("strlen(websiteWithSlash) %d\n",strlen(websiteWithSlash));
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


            hostToconnect = gethostbyname(website);
            if (!hostToconnect)
            {
                perror("Inavlid host address");
                exit(1);
            }

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
            if (url != 0)
                sprintf(requesToHost,"GET %s %s\r\nHost: %s\r\nConnection: close\r\n\r\n",url,http_ver,website);
            else
                sprintf(requesToHost,"GET / %s\r\nHost: %s\r\nConnection: close\r\n\r\n",http_ver,website);
       
            printf("requesToHost %s\n", requesToHost);
            sprintf(filename,"%s.html",website);
            strncat(pwd,filename,strlen(filename));
            fileProxy = fopen(pwd,"ab");
            // sending request to host server- Ex www.google.com
            printf("coming into send\n");
            nbytes = send(hostfd,requesToHost,sizeof(requesToHost),0);

                printf("coming into send\n");
            if (nbytes < 0)
            {
                perror("Host send failed");
            }

            else
            {
                printf("coming into send\n");
                do{
                    printf("hhhhhhh\n");
                    bzero((char*)readBufferFrmServer,sizeof(readBufferFrmServer));

                    nbytes=recv(hostfd,readBufferFrmServer,sizeof(readBufferFrmServer),0);        // receiving from host server
                    sbytes +=nbytes;
                    
                    fwrite(readBufferFrmServer,1,nbytes,fileProxy);
                    if(!(nbytes<=0))
                        send(sockfd,readBufferFrmServer,sizeof(readBufferFrmServer),0);                                 // sending to client 
                    
                }while(nbytes>0);
            }

            fclose(fileProxy);

        }
    }

    close(sockfd);
    close(hostfd);

}
