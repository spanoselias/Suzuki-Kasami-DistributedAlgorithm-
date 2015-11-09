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

#define BUFSIZE 4096

#define  DEBUG 

struct FTP_HEADER
{
    char *cmd;
    char *filename;
};

int serverlen;

struct sockaddr_in server;
struct sockaddr *serverPtr;
struct hostent *rem;

int get_file(char *buffer , int sock, char *filename )
{
    FILE     *received_file;
    int      remain_data;
    int      bytes;
    int      file_size;


    //char *simple="31.mp3";
    //sprintf(buffer,"%s",simple);
    printf("Filename in get_file: %s\n",filename);
    sprintf(buffer , "%s" , filename);
    if ((bytes=send(sock, buffer, sizeof(buffer), 0)) < 0)
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

   //sprintf(filename , "%s_%d" , filename , (rand() % 1000));
    received_file = fopen(filename, "w");
    if (received_file == NULL)
    {
        fprintf(stderr, "Failed to open file foo --> %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    remain_data = file_size;

    bzero(buffer, sizeof(BUFSIZE));
    while (((bytes = recv(sock, buffer, BUFSIZE, 0)) > 0) && (remain_data > 0))
    {
        fwrite(buffer, sizeof(char), bytes, received_file);
        remain_data -= bytes;
        fprintf(stdout, "Receive %d bytes and we hope :- %d bytes\n", bytes, remain_data);

        printf("Received: %d , remain : %d\n",bytes , remain_data);
        if(remain_data ==0 )
        {
            break;
        }
    }//While

    fclose(received_file);
    close(sock);

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
        ftp_header.filename[strlen(ftp_header.filename)-1]='\0';
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

int main(int argc , char  *argv[])
{

/***********************************************************************************/
/*                                   LOCAL VARIABLES                               */
/***********************************************************************************/
    char     buffer[BUFSIZE];/*Buffer to send*receive data*/
    int      file_size;
    int      remain_data = 0;
    int      len;
    int      sock; //Store Socket descriptor

    //Input parameters variables
    /*The port that ftp server listen*/
    int      port;
    /*The ftp server ip*/
    char     *server_ip;

    int       bytes;
    /*Store command from ftp client*/
    char      cmdbuf[1024];

    /*Store information for ftp header*/
    struct FTP_HEADER ftp_header;
    ftp_header.cmd=(char *)malloc(sizeof(char) * 20);
    ftp_header.filename=(char *)malloc(sizeof(char) * 255);

    //Check of input arguments
    if(argc!=3)
    {
        printf("\nUsage: argv[0] [SERVER_IP] [PORT]\n");
        exit(-1);
    }

    //Retrieve input parameters
    server_ip=(char*)malloc(sizeof(char) * strlen(argv[1]));
    strcpy(server_ip,argv[1]);
    port=atoi(argv[2]);

    /*Establish connection with ftp server*/
    sock=establish_conn(server_ip,port);

    bzero(buffer, sizeof(BUFSIZE));
    /* Receiving file size */

    printf("Ftp to %s ..." , server_ip);
    do
    {
        printf("\nFtp>");

        fgets(cmdbuf,sizeof(cmdbuf),stdin);

        read_cmd(cmdbuf,ftp_header);

      //  sprintf(ftp_header.filename,"%s",strtok(ftp_header.filename,"\n"));
        printf("\n%s" , ftp_header.filename);
        if(strcmp(ftp_header.cmd , "get")==0)
        {
            get_file(buffer,sock,ftp_header.filename);
        }

    }while(strcmp(cmdbuf,"exit") !=0);

    return 0;
}
