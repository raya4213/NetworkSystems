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

#define MAX_CONNECTIONS 1000
#define BYTES 1024
//#define PORT_NUM 10000
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
#define TIMEOUT          10

#define LINEBUFLENGTH    2000

char *ROOT_DIR;
int listenfd, clients[MAX_CONNECTIONS];
void error(char *);
void startWebServer();
void respond(int);
char * checkContentType(char *);
//char *directoryroot;
char PORT[10];
struct itimerval timeout;

/* Handles Time Out */
int globalClientNumber;
void alarm_handler(void)
{  
	printf("Entering alarm alarm_handler\n");
	printf("closing the socket %d\n", globalClientNumber);
	shutdown (clients[globalClientNumber], SHUT_RDWR);         //All further send and recieve operations are DISABLED...
	close(clients[globalClientNumber]);
	clients[globalClientNumber]=-1;
   	exit(SOCKETCLOSE);
}
static unsigned int get_file_size (FILE * fileDescriptor)
{
    unsigned int size;

    fseek(fileDescriptor, 0L, SEEK_END);
    size = ftell(fileDescriptor);
    fseek(fileDescriptor, 0L, SEEK_SET);

    return size;
}

char *getExtension (char *filename) {
    char *extension = strrchr (filename, '.');
    //if (extension == NULL)
        //extension = ""; // fast method, could also use &(fspec[strlen(fspec)]).
    return extension;
}

int extractFileFormats(char *filetype)
{

    FILE *fp;
    char wsBuf[wsMaxSize];
    //char *val1;
    unsigned int formatIndex = 0;
    int file_supported = FAIL;
    char formats[20][100];


    //char *wsConfigfile = "/home/rahul/Desktop/NETSYS/Ass2_Code/ws.conf";//getenv("PWD");
    char *wsConfigfile1 = getenv("PWD");
	//strncat(wsConfigfile1,"/ws.conf", 8);
	if (wsConfigfile1 != NULL)
    	printf("Path to wsConfig file is in extractFileFormats is: %s \n", wsConfigfile1);

    fp=fopen(wsConfigfile1,"r");
    unsigned int wsConfigFileSize = get_file_size (fp);

    //printf("ws.conf size n = %d, filename = %s\n", wsConfigFileSize, wsConfigfile);

    while(fgets(wsBuf,wsConfigFileSize,fp)!=NULL) {//read from the .conf file
        strcpy(formats[formatIndex],wsBuf);
        //printf("format %s", formats[formatIndex]);
        formatIndex++;
    }

    int k=0;

    
    for(k=0;k<formatIndex+1;k++) {
        if(strncmp(formats[k],filetype,3)==0) {//check if the file is supported
            //printf("\nsupported file format");
            file_supported = SUCCESS;//if supported then set file_supported.
            break;
        }
    }

    fclose(fp);

    return file_supported;


}


void readWSconfig()
{
    FILE *fp;
    char wsBuf[wsMaxSize];
    char *val1;
    //char *wsConfigfile = "/home/rahul/Desktop/NETSYS/Ass2_Code/ws.conf";//getenv("PWD");
    char *wsConfigfile = getenv("PWD");
	strncat(wsConfigfile,"/ws.conf", 8);
    printf("Path to wsConfig file is %s: \n", wsConfigfile);

    fp=fopen(wsConfigfile,"r");

    if (fp == NULL)
    {

        perror("ws.conf");
        printf("Exiting the program\n");
        exit(WSCONFIGERROR);
    }

    else
    {
        unsigned int wsConfigFileSize = get_file_size (fp);

        printf("ws.conf size n = %d, filename = %s\n", wsConfigFileSize, wsConfigfile);


        while(fgets(wsBuf,wsConfigFileSize,fp)!=NULL) {


            /**********************
            * Finds Root directory
            /**********************/
            if(strncmp(wsBuf,"DocumentRoot",12)==0) {
                printf("wsBuf: %s",wsBuf);
                val1=strtok(wsBuf," \t\n");
                val1 = strtok(NULL, " \t\n");
                ROOT_DIR=(char*)malloc(100);
                strcpy(ROOT_DIR,val1);
                printf("ROOT_DIR:%s",ROOT_DIR);
                bzero(wsBuf, sizeof(wsBuf));

            }

            /****************
            * Finds PORT NUM 
            /****************/
            if(strncmp(wsBuf,"Listen",6)==0) {
                printf("wsBuf: %s",wsBuf);
                val1=strtok(wsBuf," \t\n");
                val1 = strtok(NULL, " \t\n");
                strcpy(PORT, val1);
                printf("PORT number%s\n", PORT);
            	bzero(wsBuf, sizeof(wsBuf));
            }

        }

        fclose(fp);
    }

}

int main(int argc, char* argv[])
{

    int connectionNum=0;
    int i;

    struct sockaddr_in clientAddr;
    socklen_t addrlen;
    char c;    

    readWSconfig();     // reads the wsConfigFile
    for (i=0; i<MAX_CONNECTIONS; i++)
    {
        clients[i]=-1;
    }

    int port_num = atoi(PORT);
    if (port_num < 1024)
    {
        fprintf(stderr, "The port number chosen is %d and is INVALID\n", port_num);
        exit(PORTERROR);
    }
    //if ()
    startWebServer();

    // ACCEPT connections
    while (1)
    {
        addrlen = sizeof(clientAddr);
        clients[connectionNum] = accept (listenfd, (struct sockaddr *) &clientAddr, &addrlen);
        if (clients[connectionNum]<0)
            error ("accept() error");
        else
        {
            if ( fork()==0 )
            {
                respond(connectionNum);
                //exit(SLOTERROR);
            }
        }

        while (clients[connectionNum]!=-1) 
            {
                connectionNum = (connectionNum+1)%MAX_CONNECTIONS;
            }
    }

    return 0;
}

//start server
void startWebServer(int port)
{
    struct addrinfo webServerHints, *res, *p;
    //char port_num_array[6];
    // getaddrinfo for host
    memset (&webServerHints, 0, sizeof(webServerHints)); // Making sure the struct is empty
    webServerHints.ai_family = AF_INET;                  // IPv4
    webServerHints.ai_socktype = SOCK_STREAM;            // TCP stream sockets
    webServerHints.ai_flags = AI_PASSIVE;
    //sprintf(port_num_array, "%d", port);

    /*int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints,
                struct addrinfo **res);
    Given node and service, which identify an Internet host and a service, 
    getaddrinfo() returns one or more addrinfo structures, each of which contains an Internet address that can be specified in a call to bind(2) or connect(2). 
    */

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

    // listen for incoming connections
    if ( listen (listenfd, MAX_CONNECTIONS) != 0 )
    {
        perror("listen() error");
        exit(LISTENERROR);
    }
}

char * checkContentType(char *filetype)
{
    char *type;

    if((strcmp(filetype,".htm"))==0 || (strcmp(filetype,".html"))==0)
        type = "text/html";
    else if((strcmp(filetype,".jpg"))==0)
        type = "image/jpeg";
    else if(strcmp(filetype,".gif")==0)
        type = "image/gif";
    else if(strcmp(filetype,".txt")==0)
        type = "text/plain";
    else if(strcmp(filetype,".png")==0)
        type = "image/png";
    else if(strcmp(filetype,".css")==0)
        type = "text/css";
    else if(strcmp(filetype,".js")==0)
        type = "text/javascript";
    else 
        type="application/octet-stream";

    return type;

}


//int globalClientNumber;
//client connection
int COUNT = 0;
void respond(int n)
{
    char mesgFrmClient[MSG_SIZE], *reqline[3], data_to_send[BYTES], path[MSG_SIZE];
    int rcvd, fd, bytes_read;
    FILE *fp;
    char status_line[LINEBUFLENGTH];
    char connection_status[50];
    globalClientNumber = n;
    int post_req_check=0;
    //struct timespec timeout;
	signal(SIGALRM, (void(*) (int)) alarm_handler);  /* Creating SIGALRM interrupt to implement timeout */
	timeout.it_value.tv_sec = TIMEOUT;
	timeout.it_value.tv_usec = 0;
	timeout.it_interval = timeout.it_value;

	printf("********************************************** globalClientNumber : %d\n",globalClientNumber);
	while(1){

	post_req_check = 0;  // make it zero before every post request
    bzero(mesgFrmClient, sizeof(mesgFrmClient));
    bzero(reqline, sizeof(reqline));
    bzero(data_to_send, sizeof(data_to_send));
    bzero(path, sizeof(path));
    bzero(status_line, sizeof(status_line));
    bzero(connection_status, sizeof(connection_status));

    memset( (void*)mesgFrmClient, (int)'\0', MSG_SIZE );
    rcvd=recv(clients[n], mesgFrmClient, MSG_SIZE, 0);


    // doubt here
	char filename[50] = "storeMsg";
    char count_str[50];
    char line_copy[99999];
    sprintf(count_str,"%d", COUNT);
    strcat(filename, count_str);
    FILE *fp_storeMsg = fopen(filename, "w");
    if (fp_storeMsg != NULL)
    {
        fputs(mesgFrmClient, fp_storeMsg);
        fclose(fp_storeMsg);
    }
    // have to check this
	if (!strstr(mesgFrmClient,"Connection: Keep-alive"))    // capturing the last string from the received message
	{
	    //setitimer(ITIMER_REAL,&timeout,NULL);
	    strncpy(connection_status, "Connection: Keep-alive", strlen("Connection: Keep-alive"));
	}
	else    /* -- If Keep-alive is not found, close the connection --- */
	{
		strncpy(connection_status, "Connection: Close",strlen("Connection: Close"));
		//exit(0);
		//break;
	}

    bzero(status_line, sizeof(status_line));
    if (rcvd<0)    // receive error
        fprintf(stderr,("recv() error\n"));
    else if (rcvd==0)    // receive socket closed
    	rcvd = 0;
        //fprintf(stderr,"Client disconnected upexpectedly.\n");

    else    // message received
    {
    	// if another request occpurs pertaining to the same socket then TIMEOUT value has to be reset
		timeout.it_value.tv_sec = TIMEOUT;
		timeout.it_value.tv_usec = 0;
		timeout.it_interval = timeout.it_value;
    	if (!strstr(mesgFrmClient,"Connection: Keep-alive"))    // capturing the last string from the received message
		{
	        //setitimer(ITIMER_REAL,&timeout,NULL);
	        strncpy(connection_status, "Connection: Keep-alive", strlen("Connection: Keep-alive"));
		}
		else    /* -- If Keep-alive is not found, close the connection --- */
		{
			strncpy(connection_status, "Connection: Close",strlen("Connection: Close"));
		}

        printf("\n## printing mesgFrmClient %s\n", mesgFrmClient);

        // Now breaking the incoming strng into three different paths
        reqline[0] = strtok (mesgFrmClient, " \t\n");
        if ((strncmp(reqline[0], "GET\0", 4)==0) || (strncmp(reqline[0], "POST\0", 5)==0))
        {
        	if (strncmp(reqline[0], "POST\0", 5)==0)
        	{
        		printf("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&Com\n");
        		post_req_check = TRUE;
        	}

            reqline[1] = strtok (NULL, " \t");
            reqline[2] = strtok (NULL, " \t\n");
            

            char http_version[8];
            if (strncmp(reqline[2], "HTTP/1.1", 8) == 0)
                strcpy(http_version, "HTTP/1.1");
            else
                strcpy(http_version, "HTTP/1.0");


            if ( strncmp( reqline[2], "HTTP/1.0", 8)!=0 && strncmp( reqline[2], "HTTP/1.1", 8)!=0 )
            {


                strncat(status_line,http_version,strlen(http_version));
                strncat(status_line," 400 Bad Request",strlen(" 400 Bad Request"));
                strncat(status_line,"\n",strlen("\n"));//strncat(status_line,"\r\n",strlen("\r\n"));
                strncat(status_line,"Content-Type:",strlen("Content-type:"));
                strncat(status_line,"NONE",strlen("NONE"));
                strncat(status_line,"\n",strlen("\n"));
                strncat(status_line,"Content-Length:",strlen("Content-Length:"));
                strncat(status_line,"NONE",strlen("NONE"));
                strncat(status_line,"\n",strlen("\n"));
	            strncat(status_line,connection_status,strlen(connection_status));
                strncat(status_line,"\r\n",strlen("\r\n"));
                strncat(status_line,"\r\n",strlen("\r\n"));
                strncat(status_line,"<HEAD><TITLE>400 Bad Request Reason</TITLE></HEAD>",strlen("<HEAD><TITLE>400 Bad Request Reason</TITLE></HEAD>"));
                //strncat(status_line,"\r\n",strlen("\r\n"));
                strncat(status_line,"<html><BODY>>400 Bad Request Reason: Invalid HTTP-Version :",strlen("<BODY>>400 Bad Request Reason: Invalid HTTP-Version :"));
                strncat(status_line,"HTTP",strlen("HTTP"));
                strncat(status_line,"</BODY></html>",strlen("</BODY></html>"));
                // strncat(status_line,"\r\n",strlen("\r\n"));
                strncat(status_line,"\r\n",strlen("\r\n"));
                printf("%s\n",status_line);
                write(clients[n], status_line, strlen(status_line));
            }
            else
            {
                if ( strncmp(reqline[1], "/\0", 2)==0 )
                    reqline[1] = "/index.html";        //Because if no file is specified, index.html will be opened by default (like it happens in APACHE...

                strcpy(path, ROOT_DIR);
                strcpy(&path[strlen(ROOT_DIR)], reqline[1]);
                printf("file: %s\n", path);

                int formatCheck;
                char *ext = getExtension(path);
                if (ext == NULL)
                {
                    formatCheck = FALSE;
                }
                else
                {
                    formatCheck = extractFileFormats(ext);
                }
                
                
                char size_array[20];

                
                if (formatCheck == TRUE)
                {
                    //printf("************************************************** File supported: %s\n", ext);
                    if ( (fd=open(path, O_RDONLY))!=-1 )    //FILE FOUND
                    {
                        fp = fopen(path,"r");;
                        char *checkfileType = checkContentType(ext);
                        int size= get_file_size(fp);
                        sprintf(size_array,"%d",size);
                        
                        // strncat(status_line," 200 OK",strlen(" 200 OK"));
                        // strncat(status_line,"\n",strlen("\n"));
                        // strncat(status_line,"Content-Type:",strlen("Content-type:"));
                        // strncat(status_line,checkfileType,strlen(checkfileType));
                        // strncat(status_line,"\n",strlen("\n"));
                        // strncat(status_line,"Content-Length:",strlen("Content-Length:"));
                        // strncat(status_line,size_array,strlen(size_array));
                        // strncat(status_line,"\n",strlen("\n"));
                        // strncat(status_line,connection_status,strlen(connection_status));
                        // strncat(status_line,"\r\n",strlen("\r\n"));
                        // strncat(status_line,"\r\n",strlen("\r\n"));
                        // printf("printing status_line %s\n",status_line);
                        // send(clients[n], status_line, strlen(status_line), 0);


                        
                        char msgRecvPost[99999];
                        if (post_req_check)
                        {
                        	COUNT++;
                        	// add some thing to data_to_send
                        	printf("coming into post loop\n");
                        	FILE * fp_chck_storeMsg;
						    char * line = NULL;
						    size_t len = 0;
						    ssize_t read;
						    int emptyLineFound = FALSE;
						    fp_chck_storeMsg = fopen(filename, "r");
						    if (fp_chck_storeMsg == NULL)
						        exit(EXIT_FAILURE);

						    int dummy = 1;

						    while ((read = getline(&line, &len, fp_chck_storeMsg)) != -1) {
						    
						    	if (read == 2 && emptyLineFound == FALSE)
						    	{
						    		//printf("			Retrieved line of length %zu :\n", read);
						        	//printf("			%s", line);
						        	//printf("			heelo sirji\n");
						    		emptyLineFound = TRUE;
						    		//strncat(msgRecvPost, line, strlen(line));

						    	}

						   		if (emptyLineFound == TRUE)
						    	{
						    		printf("			Retrieved line of length %zu :\n", read);
						        	printf("			%s", line);
						        	if (dummy == 1)
						        	{
						        		
						        		dummy = 0;
						        	}
						        	else
						        	{
						        		
						    			strncat(msgRecvPost, line, strlen(line));
						    		}
						    		
						    	}

						    }
						    printf("						%s\n", msgRecvPost);	
						    dummy = 1;
						    emptyLineFound = FALSE;
						    fclose(fp_chck_storeMsg);
						    remove(filename);
						    if (line)
						        free(line);
                        }

                        char msg[99999];
                        if (post_req_check)
                        {
                        	strncat(status_line,"POST ",strlen("POST "));	
                        }
                        strncat(status_line,http_version,strlen(http_version));
                        strncat(status_line," 200 OK",strlen(" 200 OK"));
                        strncat(status_line,"\n",strlen("\n"));
                        strncat(status_line,"Content-Type:",strlen("Content-type:"));
                        strncat(status_line,checkfileType,strlen(checkfileType));
                        strncat(status_line,"\n",strlen("\n"));
                        strncat(status_line,"Content-Length:",strlen("Content-Length:"));
                        strncat(status_line,size_array,strlen(size_array));
                        strncat(status_line,"\n",strlen("\n"));
                        strncat(status_line,connection_status,strlen(connection_status));
                        if (post_req_check){
	                        strncat(status_line,"\r\n\r\n",strlen("\r\n\r\n"));
	                        sprintf(msg,"<h1>Post Data</h1><html><body>%s</body></html>\n",msgRecvPost);
	                        strncat(status_line, msg, strlen(msg));
	                        strncat(status_line,"\r\n",strlen("\r\n"));
	                    }
	                    else{
	                        strncat(status_line,"\r\n",strlen("\r\n"));
	                        strncat(status_line,"\r\n",strlen("\r\n"));
	                    }
                        printf("printing status_line %s\n",status_line);
                        send(clients[n], status_line, strlen(status_line), 0);

                        while ( (bytes_read=read(fd, data_to_send, BYTES))>0 )
                            write (clients[n], data_to_send, bytes_read);

                        fclose(fp);
                        bzero(msgRecvPost,sizeof(msgRecvPost));
                        bzero(msg,sizeof(msg));
                    }
                
                    else{  // file not found loop

                    	strncat(status_line,http_version,strlen(http_version));
                        strncat(status_line," 404 Not Found",strlen(" 404 Not Found"));
                        strncat(status_line,"\n",strlen("\n"));//strncat(status_line,"\r\n",strlen("\r\n"));
                        strncat(status_line,"Content-Type:",strlen("Content-type:"));
                        strncat(status_line,"Invalid",strlen("Invalid"));
                        strncat(status_line,"\n",strlen("\n"));
                        strncat(status_line,"Content-Length:",strlen("Content-Length:"));
                        strncat(status_line,"Invalid",strlen("Invalid"));
	                	strncat(status_line,"\n",strlen("\n"));
		            	strncat(status_line,connection_status,strlen(connection_status));                     
                        strncat(status_line,"\r\n",strlen("\r\n"));
                        strncat(status_line,"\r\n",strlen("\r\n"));
                        //strncat(status_line,"<HEAD><TITLE>404 Not Found</TITLE></HEAD>",strlen("<HEAD><TITLE>404 Not Found</TITLE></HEAD>"));
                        strncat(status_line,"<html><BODY>404 Not Found: URL does not exist:",strlen("<BODY>404 Not Found: URL does not exist:"));
                        strncat(status_line,path,strlen(path));
                        strncat(status_line,"</BODY></html>",strlen("</BODY></html>"));
                        // strncat(status_line,"\r\n",strlen("\r\n"));
                        strncat(status_line,"\r\n",strlen("\r\n"));
                        printf("%s\n",status_line);
                        //write(clients[n], "HTTP/1.1 404 Not Found\nayyo etla mama\n\n", 24); //FILE NOT FOUND
                        write(clients[n], status_line, strlen(status_line)); //FILE NOT FOUND
                            
                    }    
                        
                }

                else // file not supported
                {
                	printf("*********************************************************************8	\n");
                	strncat(status_line,http_version,strlen(http_version));
                    strncat(status_line," 501 Not Implemented",strlen(" 501 Not Implemented"));
                    strncat(status_line,"\n",strlen("\n"));//strncat(status_line,"\r\n",strlen("\r\n"));
                    strncat(status_line,"Content-Type:",strlen("Content-type:"));
                    strncat(status_line,"NONE",strlen("NONE"));
                    strncat(status_line,"\n",strlen("\n"));
                    strncat(status_line,"Content-Length:",strlen("Content-Length:"));
                    strncat(status_line,"NONE",strlen("NONE"));
                	strncat(status_line,"\n",strlen("\n"));
	            	strncat(status_line,connection_status,strlen(connection_status));                    
                    strncat(status_line,"\r\n",strlen("\r\n"));
                    strncat(status_line,"\r\n",strlen("\r\n"));
                    strncat(status_line,"<HEAD><TITLE>501 Not Implemented</TITLE></HEAD>",strlen("<HEAD><TITLE>501 Not Implemented</TITLE></HEAD>"));
                    //strncat(status_line,"\r\n",strlen("\r\n"));
                    strncat(status_line,"<BODY>501 Not Implemented: File format not supported:",strlen("<BODY>501 Not Implemented: File format not supported:"));
                    strncat(status_line,http_version,strlen(http_version));
                    strncat(status_line,"</BODY></html>",strlen("</BODY></html>"));
                    // strncat(status_line,"\r\n",strlen("\r\n"));
                    strncat(status_line,"\r\n",strlen("\r\n"));
                    write(clients[n], status_line, strlen(status_line)); //FILE NOT FOUND   
                }

            }

        }

        else
            {

                strncat(status_line,"HTTP/1.1",strlen("HTTP/1.1"));
                strncat(status_line,"\n",strlen("\n"));//strncat(status_line,"\r\n",strlen("\r\n"));
                strncat(status_line,"Content-Type:",strlen("Content-type:"));
                strncat(status_line,"NONE",strlen("NONE"));
                strncat(status_line,"\n",strlen("\n"));
                strncat(status_line,"Content-Length:",strlen("Content-Length:"));
                strncat(status_line,"NONE",strlen("NONE"));
                strncat(status_line,"\r\n",strlen("\r\n"));
                strncat(status_line,"\r\n",strlen("\r\n"));
                strncat(status_line,"<HEAD><TITLE>501 Not Implemented</TITLE></HEAD>",strlen("<HEAD><TITLE>501 Not Implemented</TITLE></HEAD>"));
                //strncat(status_line,"\r\n",strlen("\r\n"));
                strncat(status_line,"<BODY>501 Not Implemented: File format not supported:",strlen("<BODY>501 Not Implemented: File format not supported:"));
                strncat(status_line,"HTTP/1.1",strlen("HTTP/1.1"));
                strncat(status_line,"</BODY></html>",strlen("</BODY></html>"));
                // strncat(status_line,"\r\n",strlen("\r\n"));
                strncat(status_line,"\r\n",strlen("\r\n"));
                write(clients[n], status_line, strlen(status_line)); //FILE NOT FOUND   
                // strncat(status_line," 400 Bad Request",strlen(" 400 Bad Request"));
                // strncat(status_line,"\n",strlen("\n"));//strncat(status_line,"\r\n",strlen("\r\n"));
                // strncat(status_line,"Content-Type:",strlen("Content-type:"));
                // strncat(status_line,"NONE",strlen("NONE"));
                // strncat(status_line,"\n",strlen("\n"));
                // strncat(status_line,"Content-Length:",strlen("Content-Length:"));
                // strncat(status_line,"NONE",strlen("NONE"));
                // strncat(status_line,"\r\n",strlen("\r\n"));
                // strncat(status_line,"\r\n",strlen("\r\n"));
                // strncat(status_line,"<HEAD><TITLE>400 Bad Request Reason</TITLE></HEAD>",strlen("<HEAD><TITLE>400 Bad Request Reason</TITLE></HEAD>"));
                // //strncat(status_line,"\r\n",strlen("\r\n"));
                // strncat(status_line,"<BODY>>400 Bad Request Reason: Invalid Method :",strlen("<BODY>>400 Bad Request Reason: Invalid Method :"));
                // strncat(status_line,"GET",strlen("GET"));
                // strncat(status_line,"</BODY></html>",strlen("</BODY></html>"));
                // // strncat(status_line,"\r\n",strlen("\r\n"));
                // strncat(status_line,"\r\n",strlen("\r\n"));
                // printf("%s\n",status_line);
    //             write(clients[n], status_line, strlen(status_line));
             }
    }

	if (!strstr(mesgFrmClient,"Connection: Keep-alive"))    // capturing the last string from the received message
	{
	    setitimer(ITIMER_REAL,&timeout,NULL);
	    //strncpy(connection_status, "Connection: Keep-alive", strlen("Connection: Keep-alive"));
	}
	else    /* -- If Keep-alive is not found, close the connection --- */
	{
		//strncpy(connection_status, "Connection: Close",strlen("Connection: Close"));
		//
		//break;
	    shutdown (clients[n], SHUT_RDWR);         //All further send and recieve operations are DISABLED...
    	close(clients[n]);
    	clients[n]=-1;
    	exit(0);
	}
}

    //Closing SOCKET
    // shutdown (clients[n], SHUT_RDWR);         //All further send and recieve operations are DISABLED...
    // close(clients[n]);
    // clients[n]=-1;

    printf("coming here rey\n");
}