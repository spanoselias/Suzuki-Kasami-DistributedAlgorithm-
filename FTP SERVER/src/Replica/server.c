/***********************************************************************************/
/*                                                                                 */
/*Name: Elias Spanos                                                 			   */
/*Date: 09/10/2015                                                                 */
/*Filename:FTP SERVER                                                              */
/*                                                                                 */
/***********************************************************************************/
/***********************************************************************************/
/*                                     LIBRARIES                                   */
/***********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> //To retrieve the ip_add to asci
#include <sys/stat.h>
#include <errno.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <stdbool.h>

#define BUFSIZE 4096
#define MAXCLIENT 100

#define DEBUG

struct FTP_HEADER
{
    char *cmd;
    char *filename;
    int filesize;
};

int send2ftp(char *filename, int newsock)
{
    int         fd;
    off_t       offset = 0;
    int         remain_data;
    int         sent_bytes;
    struct      stat file_stat; /*to retrieve information for the file*/
    int         len;
    int         file_size;

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
    printf("File Size: \n %d bytes\n",file_size);

    /* If connection is established then start communicating */
    len = send(newsock, &file_size, sizeof(file_size), 0);
    if (len < 0)
    {
        perror("send");
        exit(EXIT_FAILURE);
    }

    printf("Server sent %d bytes for the size\n" , len);

    remain_data = file_stat.st_size;
    /* Sending file data */
    while (((sent_bytes = sendfile(newsock, fd, &offset, BUFSIZE)) > 0) && (remain_data > 0))
    {
        remain_data -= sent_bytes;
        fprintf(stdout, "Server sent %d bytes from file's data, offset is now : %d and remaining data = %d\n", sent_bytes, offset, remain_data);

    }
    printf("Finish sending\n");
    close(newsock);
    close(fd);

    return true;

}

int ftp_recv(char *buffer , int sock, char *filename , int fileSize )
{
    FILE     *received_file;
    int      remain_data;
    int      bytes;
    int      file_size;

    printf("Received: %d\n" , fileSize);

    //sprintf(filename , "%s_%d" , filename , (rand() % 1000));
    received_file = fopen(filename, "w");
    if (received_file == NULL)
    {
        fprintf(stderr, "Failed to open file foo --> %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    remain_data = fileSize;

    bzero(buffer, sizeof(buffer));
    while (((bytes = recv(sock, buffer, sizeof(buffer), 0)) > 0) && (remain_data > 0))
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
        //ftp_header.filename[strlen(ftp_header.filename)-1]='\0';
        //strcpy(ftp_header.filename ,strtok(NULL," "));
    }

    else if(strcmp(cmd , "put" )==0)
    {
        sprintf(ftp_header.filename,"%s",strtok(NULL," "));
        sprintf(ftp_header.filesize,"%d",atoi(strtok(NULL," ")));
    }

    return 1;
}

int main(int argc , char  *argv[])
{

/***********************************************************************************/
/*                                   LOCAL VARIABLES                               */
/***********************************************************************************/
    struct      sockaddr_in serv_addr;
    struct      sockaddr_in client_addr;
    struct      sockaddr *servPtr;
    struct      sockaddr *clientPtr ;
    int         servSock;
    int         newsock;
    int         port;
    int         clientlen;
    int         bytes;
    char        buf[BUFSIZE];
    int         len;
    int         file_size;
    char        *filename=NULL;

    /*Store information for ftp header*/
    struct FTP_HEADER ftp_header;
    ftp_header.cmd=(char *)malloc(sizeof(char) * 20);
    ftp_header.filename=(char *)malloc(sizeof(char) * 255);

    //Check of input arguments
    if(argc !=2)
    {
        printf("\nUsage: argv[0] [port]\n");
        exit(-1);
    }

    //Retrieve input parameters
    port=atoi(argv[1]);

    /* Initialize socket structure */
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_addr.sin_port=htons(port);

    servPtr=(struct sockaddr *) &serv_addr;

    /*Create a socket and check if is created correct.*/
    if((servSock=socket(AF_INET,SOCK_STREAM,0)) < 0)
    {
        perror("Socket() failed"); exit(1);
    }

    /*Bind socket to address */
    if(bind(servSock,servPtr, sizeof(serv_addr)) < 0)
    {
        perror("Bind() failed");
        exit(1);
    }

    /*Listen for connections */
    /* MAXCLIENT. requests in queue*/
    if(listen(servSock , MAXCLIENT) < 0)
    {
        perror("Listen() failed"); exit(1);
    }

    printf("\nStarting server....\n");
    printf("Listening for connections to port %d\n" , port);
    while(true)
    {
        clientPtr=(struct sockaddr *) &client_addr;
        clientlen= sizeof(client_addr);

        if((newsock=accept(servSock , clientPtr , &clientlen)) < 0){
            perror("accept() failed"); exit(1);}

        printf("\nAccepted connection from IP:%s \n" , inet_ntoa(client_addr.sin_addr));
        printf("*****************************\n");
        switch (fork())
        {
            case -1:
                perror("fork failed!"); exit(1);

            case 0:

                //Initialize buffer
                bzero(buf,sizeof(buf));

                //Request filename
                if(recv(newsock, buf, sizeof(buf), 0) < 0)
                {
                    perror("Received");
                    exit(newsock);
                }

                filename=(char*)malloc((sizeof(char) * sizeof(buf)));
                sprintf(filename ,"%s" , buf);
                read_cmd(buf,ftp_header);
                //strcpy(filename,buf);
                printf("\nReceived filename:%s\n" , buf);

            if(strcmp(ftp_header.cmd , "get")==0)
            {
                if (ftp_recv(buf,newsock,ftp_header.filename,ftp_header.filesize) == false)
                {
                    printf("Error ftp_send()");
                }
            }


        }//Switch
    }//While(1)

    return 0;
}//Main Function