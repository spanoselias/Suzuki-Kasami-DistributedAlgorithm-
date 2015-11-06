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

int serverlen;

struct sockaddr_in server;
struct sockaddr *serverPtr;
struct hostent *rem;

int get_file(char *buffer , int sock, char *filename )
{
    FILE     *received_file;
    int remain_data;
    int bytes;
    int file_size;

    strcpy(buffer,filename);
    printf("Filename: %s\n",buffer);

    if ((bytes=send(sock, buffer, (strlen(filename)+1), 0)) < 0)
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

    sprintf(filename , "%s_%d" , filename , (rand() % 1000));
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
    }

    fclose(received_file);
    close(sock);

}

int read_cmd(char *cmd_str  )
{
    char *cmd;

    /* get the first token */
    cmd = strtok(cmd_str, " ");

    if(strcmp(cmd ,  ))


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
    /*File name*/
    char     *filename;
    int       bytes;
    /*Store command from ftp client*/
    char      cmdbuf[1024];

    //Check of input arguments
    if(argc!=4)
    {
        printf("\nUsage: argv[0] [SERVER_IP] [PORT] [filename]\n");
        exit(-1);
    }

    //Retrieve input parameters
    server_ip=(char*)malloc(sizeof(char) * strlen(argv[1]));
    filename=(char*)malloc(sizeof(char) * strlen(argv[3]));
    strcpy(server_ip,argv[1]);
    strcpy(filename , argv[3] );
    port=atoi(argv[2]);

    /*Establish connection with ftp server*/
    sock=establish_conn(server_ip,port);

    bzero(buffer, sizeof(BUFSIZE));
    /* Receiving file size */

    get_file(buffer,sock,filename);

    printf("ftp to %s ..." , server_ip);
    do
    {
        fgets(cmdbuf,sizeof(cmdbuf), stdin);

        if(strcmp(cmdbuf,"get_file")==0)
        {
            get_file(buffer,sock,filename);
        }

    }while(strcmp(cmdbuf,"exit") !=0);

    return 0;
}
