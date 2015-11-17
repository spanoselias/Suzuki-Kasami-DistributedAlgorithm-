/***********************************************************************************/
/*                                                                                 */
/*Name: Elias Spanos                                                 			   */
/*Date: 09/10/2015                                                                 */
/*Filename:FTP CLIENT                                                              */
/*                                                                                 */
/***********************************************************************************/
/***********************************************************************************/
/*                                     LIBRARIES                                   */
/***********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "sftp_cli.h"

#define MAXBUF 9999
#define  DEBUG 

int      serverlen;
struct   sockaddr_in server;
struct   sockaddr *serverPtr;
struct   hostent *rem;

int get_file(char *buffer , int sock, char *filename )
{
    FILE     *received_file;
    int      remain_data;
    int      bytes;
    int      file_size;

    sprintf(buffer , "get %s" , filename);
    file_size=strlen(filename) + 4;
    printf("Filename in get_file: %s , length: %d\n",buffer, file_size);
    if ((bytes=send(sock, buffer, file_size , 0)) < 0)
    {
        perror("send() failed\n");
        close(sock);
        exit(EXIT_FAILURE);
    }
    printf("Bytes sends: %d\n" ,bytes );

    if(recv(sock, &file_size, sizeof(file_size), 0) < 0)
    {
        perror("Received");
        exit(sock);
    }

    printf("Received: %d\n" , file_size);

    received_file = fopen(filename, "w");
    if (received_file == NULL)
    {
        fprintf(stderr, "Failed to open file foo --> %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    remain_data = file_size;

    bzero(buffer, MAXBUF);
    while (((bytes = recv(sock, buffer, MAXBUF, 0)) > 0) && (remain_data > 0))
    {
        fwrite(buffer, sizeof(char), bytes, received_file);
        remain_data -= bytes;
        fprintf(stdout, "Receive %d bytes and we hope :- %d bytes\n", bytes, remain_data);

      //  printf("Received: %d , remain : %d\n",bytes , remain_data);
        if(remain_data ==0 )
        {
            break;
        }
    }//While

    fclose(received_file);
    close(sock);
}

int send2ftp(char *filename, int newsock , char *buffer)
{
    int         fd;
    off_t       offset = 0;
    int         remain_data;
    int         sent_bytes;
    struct      stat file_stat; /*to retrieve information for the file*/
    int         len;
    int         file_size;
    int         send_size;

    printf("FileNAmeIn:%s" , filename);

    fd = open(filename,  O_RDONLY);
    if (fd < 0 )
    {
        fprintf(stderr, "Error opening file --> %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Get file stats */
    if (fstat(fd, &file_stat) < 0)
    {
        printf("Error fstat");
        close(newsock);
        close(fd);
        exit(1);
    }

    /* Sending file size */
    file_size=file_stat.st_size;
    printf("File Size: %d bytes\n",file_size);

    bzero(buffer,sizeof(buffer));
    sprintf(buffer,"put %s %d" , filename , file_size);

    /*Calculate the send bytes*/
    send_size= 5 + strlen(filename) + file_size;
    /* If connection is established then start communicating */
    len = send(newsock, buffer, send_size , 0);
    if (len < 0)
    {
        perror("send");
        exit(EXIT_FAILURE);
    }
    printf("\nServer sent %d bytes for the size\n" , len);

    /*delete*/
    sleep(1);

    /*Calculate the total size of the file*/
    remain_data = file_stat.st_size;
    /* Sending file data */
    while (((sent_bytes = sendfile(newsock, fd, &offset, MAXBUF)) > 0) && (remain_data > 0))
    {
        remain_data -= sent_bytes;
        fprintf(stdout, "Server sent %d bytes from file's data, offset is now : %d and remaining data = %d\n", sent_bytes, offset, remain_data);

    }

    printf("Finish sending\n");
    close(newsock);
    close(fd);

    return true;
}

int read_cmd(char *cmd_str , struct FTP_HEADER ftp_header)
{
    char *cmd;

    /* get the first token */
    cmd = strtok(cmd_str, " ");
    strcpy(ftp_header.cmd,cmd);

    if(strcmp(cmd , "get" ) == 0)
    {
        sprintf(ftp_header.filename,"%s",strtok(NULL," "));
    //    ftp_header.filename[strlen(ftp_header.filename)-1]='\0';
        //strcpy(ftp_header.filename ,strtok(NULL," "));
    }
    if(strcmp(cmd , "put" ) == 0)
    {
        sprintf(ftp_header.filename,"%s",strtok(NULL," "));
       // ftp_header.filename[strlen(ftp_header.filename)-1]='\0';
        //strcpy(ftp_header.filename ,strtok(NULL," "));
    }

    return 1;
}

int establish_conn(char *server_ip , int port)
{
    int sock;

    /*Create a socket and check if is created correct.*/
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket() failed");
        exit(1);
    }
    /*Internet domain*/
    server.sin_family=AF_INET;
    //Convert dotted-decimal address to 32-bit binary address
    server.sin_addr.s_addr=inet_addr(server_ip);
    server.sin_port=htons(port);

    //Point in the server struct.
    serverPtr=(struct sockaddr *) &server;
    serverlen= sizeof(server);

    /*Create a socket and check if is created correct.*/
    if(connect(sock,serverPtr,serverlen) < 0 )
    {
        perror("Connect() failed"); exit(1);
    }

    printf("***********************************\n");
    printf("Request connection to port %d\n", port);

    return sock;
}

int sftp_start(char *sftp_ip , int sftp_port , char *cmd)
{
    char     buffer[MAXBUF];/*Buffer to send*receive data*/
    int      sock; //Store Socket descriptor
    int       bytes;
    /*Store command from ftp client*/
    char      cmdbuf[MAXBUF];

    /*Store information for ftp header*/
    struct FTP_HEADER ftp_header;
    ftp_header.cmd=(char *)malloc(sizeof(char) * 20);
    ftp_header.filename=(char *)malloc(sizeof(char) * 255);

    /*Establish connection with ftp server*/
    sock=establish_conn(sftp_ip,sftp_port);

     /*Initialize buffer */
     bzero(buffer,MAXBUF);
    /* Receiving file size */

    /*Store the command request in buffer*/
     strcpy(buffer,cmd);

    /*Encode the command the you read in a struct*/
     read_cmd(buffer,ftp_header);

     /*For Debug purposes*/
     printf("\n%s" , ftp_header.filename);
     if(strcmp(ftp_header.cmd , "get")==0)
     {
         get_file(buffer,sock,ftp_header.filename);
     }
     else if(strcmp(ftp_header.cmd , "put")==0)
     {
        send2ftp(ftp_header.filename,sock,buffer);
     }
}

int main(int argc , char *argv[])
{
    printf("Running ftp client...");
    char *cmdReq="get sample2.txt";
    char *sftp_ip="127.0.0.1";
    int port= 10005;

    sftp_start(sftp_ip,port,cmdReq);

   return 0;
}