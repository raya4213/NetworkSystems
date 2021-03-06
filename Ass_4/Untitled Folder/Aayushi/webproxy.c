#include <stdio.h>
#include <sys/types.h>
#include <openssl/md5.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#define MAXBUFSIZE 9999
#define BACKLOG 10

char parentdir[MAXBUFSIZE]="\0";
FILE *myFile;

char * getFileCreationTime(char *path)
{
    struct stat attr;
    stat(path, &attr);	
    //printf("Last modified time: %s", ctime(&attr.st_mtime));
    return ctime(&attr.st_mtime);
}

int cached(int fd, int timeout,char * pwd)
{
   char buf[MAXBUFSIZE]="\0";
   int file_time,cur_time,nbytes;
   FILE * myFile;
   if( access( pwd, F_OK ) != -1 ) 
   {    // file exists
     strcpy(buf,getFileCreationTime(pwd));
     printf("%s\n",buf);
     file_time = ((buf[11] - '0') * 10 + (buf[12] - '0')) * 3600 + ((buf[14] - '0') * 10 + (buf[15] - '0')) * 60 + ((buf[17] - '0') * 10 + (buf[18] - '0'));
     printf("timemod: %d\n",file_time);
     time_t t;
     time(&t);
     bzero(buf,sizeof(buf));
     strcpy(buf,ctime(&t));
     cur_time= ((buf[11] - '0') * 10 + (buf[12] - '0')) * 3600 + ((buf[14] - '0') * 10 + (buf[15] - '0')) * 60 + ((buf[17] - '0') * 10 + (buf[18] - '0'));
     printf("current time: %d\n",cur_time);
     if(cur_time - file_time < timeout)
     {
      bzero(buf,sizeof(buf));
      printf("Sending cached file\n");
      myFile=fopen(pwd,"rb");

      do
      {
      	nbytes = fread(buf,1,MAXBUFSIZE,myFile);
      	send(fd,buf,nbytes,0);

      }while(nbytes>0);

      fclose(myFile);
      close(fd);
      return 0;

     }
     else
     {
       return -1;
     
     }
    } 
   else 
   {
    return -1;
   
   } 
}



void client_handle(int fd, int timeout, char *pwd)
{

  char buffer[MAXBUFSIZE]= "\0";
  char method[MAXBUFSIZE]= "\0";
  char path[MAXBUFSIZE]= "\0";
  char http_ver[MAXBUFSIZE]= "\0";
  char filename[MAXBUFSIZE]="\0",root[MAXBUFSIZE]="\0",req[MAXBUFSIZE]="\0";
  unsigned char c[MAXBUFSIZE];
  char buff[MAXBUFSIZE]="\0";
  struct hostent *host;
  struct sockaddr_in Addr;
  char *data=NULL,*url=NULL;
  int on=1,i=0,len,s,skt,nbytes, sbytes;
  
  
  if (read(fd, buffer, MAXBUFSIZE)==-1)
  {
    printf("recieve error\n");
  }
  else
  {
    sscanf(buffer,"%s %s %s",method,path,http_ver);
    printf("buffer: %s %s %s\n",method,path,http_ver); 
    
    if(strncmp(method,"GET",strlen("GET")))
    {
      write(fd,"<html><body><H!>Error 400 Bad Request: Invalid Method </H1></body></html>",strlen("<html><body><H!>Error 400 Bad Request: Invalid Method </H1></body></html>"));
      exit(1);	 
    }
    
    else if(strncmp(http_ver,"HTTP/1.0",strlen("HTTP/1.0")))
    {
      write(fd,"<html><body><H1>Error 400 Bad Request: Invalid HTTP Version</H1></body></html>",strlen("<html><body><H1>Error 400 Bad Request: Invalid HTTP Version</H1></body></html>"));
      exit(1);	          
    }
    else
    {
      data= strstr(path,"//");
      data+=2;
      for(i=0;i<strlen(data);i++)
      {
        if(data[i]=='/')
            break;
        root[i]=data[i];
      }  
      url=strstr(data,"/");
      printf("root: %s url: %s\n",root,url);
      
      MD5_CTX mdContext;
      MD5_Init(&mdContext);
      MD5_Update (&mdContext, path, strlen(path));
      MD5_Final (c, &mdContext);
      for (i = 0; i< MD5_DIGEST_LENGTH; i++)
		{
	  		sprintf(&buff[2*i],"%02x", c[i]);
        }
      

      printf("Hash Value: %s\n",buff);
      sprintf(filename,"%s.html",buff);
      strncat(pwd,filename,strlen(filename));
      
      int cache = cached(fd,timeout,pwd);
      if (cache < 0)
       {
	   myFile=fopen(pwd,"ab");
	   host = gethostbyname(root);
	   if(!host)
	      {
		printf("Host not found\n");
		exit(1);      
	      }
	   bzero(&Addr,sizeof(Addr));                    //zero the struct
	   Addr.sin_family = AF_INET;                   //address family
	   Addr.sin_port = htons(80);        //htons() sets the port # to network byte order
	   memcpy(&Addr.sin_addr, host->h_addr, host->h_length);
	   len = sizeof(Addr);

	   s = socket(AF_INET, SOCK_STREAM, 0);
	   if (s < 0)
	     printf("Socket nt created\n");

	   setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, 4);
	   skt = connect(s, (struct sockaddr *) &Addr, len);
	   if (skt < 0) 
	     {
		printf("Connection problem\n");
		close(s);
	     }
			   	
	    //printf("Connected to %s on IP Address- %s",root,inet_ntoa(*((struct in_addr *)host->h_addr)));

	   if (strlen(url) !=0)
		sprintf(req,"GET %s %s\r\nHost: %s\r\nConnection: close\r\n\r\n",url,http_ver,root);
	   else
	   	sprintf(req,"GET / %s\r\nHost: %s\r\nConnection: close\r\n\r\n",http_ver,root);
	   
	   nbytes = send(s,req,strlen(req),0);
	    
	   if (nbytes<0)
	     {
	      printf("Error in Sending the request to remote host");
	     } 
	  else
	     {
	      do
	       {
		bzero((char*)buff,MAXBUFSIZE);
		nbytes=recv(s,buff,sizeof(buff),0);
							//printf("Content Length:%d\n", n);
		sbytes +=nbytes;
		fwrite(buff,1,nbytes,myFile);
							//printf("Content Return:%s\n",recv_buffer);
		if(!(nbytes<=0))
		send(fd,buff,nbytes,0);
	       }while(nbytes>0);
	
					//printf("Total Content:%d\n",k);
	     }
	     
	     fclose(myFile);
       }
    }    
  }
  close (s);
  close(fd);
} 

int main (int argc, char * argv[])
{
  char pwd[MAXBUFSIZE]="\0";
  struct sockaddr_in servAddr;
  struct sockaddr_in clientAddr;
  int pid;
  int s=1;
  int sock,sock_n;
  char cmd[MAXBUFSIZE]="\0";
  int port;
  int cache_timeout;
  
  if (argc < 3)
  {
   printf("USAGE: <Port Number> <Cache Timeout> \n");
   exit(1);
  } 
  port = atoi(argv[1]);
  cache_timeout= atoi(argv[2]);
  if (getcwd(parentdir, sizeof(parentdir)) != NULL)
   {
    printf("Current working dir: %s\n", parentdir);
    sprintf(pwd,"%s/cache/",parentdir);
    sprintf(cmd,"mkdir -p %s",pwd);
    system(cmd);
   }
  printf("%s\n",pwd);
  
  
  	
  bzero(&servAddr,sizeof(servAddr));                    //zero the struct
  servAddr.sin_family = AF_INET;                   //address family
  servAddr.sin_port = htons(port);        //htons() sets the port # to network byte order
  servAddr.sin_addr.s_addr = INADDR_ANY;
  
  
  if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
  {
   printf("ERROR: Failed to obtain Socket Descriptor");
   exit(1);
  }
    
  if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&s,sizeof(int)) == -1) 
    {
        printf("setsockopt error");
        exit(1);
    }
  
  if (bind(sock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
    {
       printf("unable to bind socket\n");
    }
  if (listen(sock, BACKLOG) == -1) 
  {
        printf("listen error");
        exit(1);
  } 
 
  while(1)
   { 
     int length = sizeof(clientAddr);
     if ((sock_n = accept(sock, (struct sockaddr *)&clientAddr,&length)) == -1)
      {
        printf("accept error");
      }
     else
     {
       printf("Connection accepted\n");
     }


     pid = fork();               
     if (pid <0)
       printf("Error on Fork !!");
		
     if (pid == 0)
      {
       close(sock);	
       client_handle(sock_n,cache_timeout,pwd);      
       exit(0);
      } 

     close(sock_n);
   }
  close(sock); 


}

