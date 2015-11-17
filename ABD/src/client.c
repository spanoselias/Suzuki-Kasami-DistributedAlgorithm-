/***********************************************************************************/
/*                                                                                 */
/*Name: Elias Spanos                                                 			   */
/*Date: 30/09/2015                                                                 */
/*Filename: client.c                                                               */
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
#include <time.h>
#include <sys/time.h>
#include <strings.h> //For bzero
#include <netdb.h>
#include <assert.h>
#include <fcntl.h>

#include "client.h"


/***********************************************************************************/
/*                                   MACROS                                        */
/***********************************************************************************/
#define BUF_SIZE 256 //Size of the buffer
#define DEVLP //Development mode
#define DEVMODE
#define DEVMODEDD

/***********************************************************************************/
/*                             GLOBAL VARIABLES                                    */
/***********************************************************************************/
FILE *LOG_FILE = NULL; //File to store the log information.
char msg_info[256];//Buffer to store message for the log file.
int *direct_ports;//Port that each avail directory listen.
int *sock_fd;//Socket descriptor
struct sockaddr_in *server;//An array that holds all the servers.
fd_set readers_fd; //Set of socket descriptor for select.
int max_fd=-1;
int AVAIL_SERVERS = 3; //Available directory servers.
int message_id;//Store the message counter
/***********************************************************************************/
/*                             decode message                                      */
/***********************************************************************************/
void decode(struct message *msg , char *buf)
{
    #ifdef DEVMODED
        printf("Client Decoding msg:%s " , buf);
    #endif

    /*Use comma delimiter to find the type of
     * send message and retrieve the apropriate
     * fields*/
    msg->type=strdup(strtok(buf, ","));

    /*Check if the type of the message is RREAD-OK*/
    if( (strcmp(msg->type , "RREAD-OK" )== 0) )
    {
        msg->tag.num = atoi(strtok(NULL, ","));
        msg->tag.id = atoi(strtok(NULL, ","));
        msg->value = atoi(strtok(NULL, ","));
        msg->msg_id = atoi(strtok(NULL, ","));
    }

    else if((strcmp(msg->type , "WREAD-OK" )== 0))
    {
        msg->tag.num = atoi(strtok(NULL, ","));
        msg->tag.id = atoi(strtok(NULL, ","));
        msg->msg_id = atoi(strtok(NULL, ","));

    }

    /*Check if the type of the message is RWRITE*/
    else if((strcmp(msg->type , "RWRITE-OK" )== 0) || (strcmp(msg->type , "WWRITE-OK" )== 0) )
    {
        msg->msg_id = atoi(strtok(NULL, ","));
    }

}//Function decode message


/***********************************************************************************/
/*                            encode message                                       */
/***********************************************************************************/
void encode(struct message *msg , char *buf , char *type)
{
    #ifdef DEVMODED
         printf("Client Encoding type:%s  " , type);
    #endif

    //Check if the type of the message is RREAD
    if((strcmp(type , "RREAD" )== 0) || (strcmp(type , "WREAD" )== 0) )
        {
            sprintf(buf,"%s,%d" ,type , msg->msg_id );
        }
    //Check if the type of the message is RWRITE
    else if( (strcmp(type , "RWRITE" )== 0 ) || (strcmp(type , "WWRITE" )== 0 ) )
        {
            sprintf(buf, "%s,%d,%d,%d,%d", type , msg->msg_id,  msg->tag.num, msg->tag.id, msg->value);
        }


}//Function decode message


/***********************************************************************************/
/*                            Establish connections                                */
/***********************************************************************************/
void establishConnections(int AVAIL_SERVERS)
{
    int port=10001;
    int i;

    /*Configure ports for all the avail directories*/
    for(i=0; i <AVAIL_SERVERS; i++ )
    {
        direct_ports[i]=port + i;
    }//For statmnet

    //Initialization for all the servers.
    for(i=0; i < AVAIL_SERVERS; i++ )
    {
        server[i].sin_family = AF_INET; /*Internet domain*/
        server[i].sin_addr.s_addr = inet_addr("127.0.0.1"); //Convert dotted-decimal address to 32-bit binary address
        server[i].sin_port = htons(direct_ports[i]);
    }

    /*Go through all the available directories to establish a connection.*/
    for(i=0; i < AVAIL_SERVERS; i++)
    {
        /*Create a socket and check if is created correct.*/
        if ((sock_fd[i] = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            perror("Socket() failed");
            exit(1);
        }
        /*Establish a connection withe the IP address from the struct server*/
        if (connect(sock_fd[i] , (struct sockaddr *) &server[i], sizeof(server[i])) < 0)
        {
            bzero(msg_info , sizeof(msg_info));
            /*Use htohs to convert network port to host port.*/
            sprintf(msg_info, "Unable to Connect to : %s ,  PORT: %d\n", inet_ntoa(server[i].sin_addr), ntohs(server[i].sin_port) );
            /*perror(msg_info ); //Review this problem. This close the next connections.*/
            printf("%s" , msg_info);
        }
        else
        {
            #ifdef DEVLP
                sprintf(msg_info, "Connection established to IP: %s ,  PORT: %d\n", inet_ntoa(server[i].sin_addr), direct_ports[i] );
                writeLog(LOG_FILE, msg_info);
                printf("Connection established to host: %s , PORT %d\n", inet_ntoa(server[i].sin_addr), direct_ports[i] );
            #endif
        }
        //Add socket to set
        FD_SET(sock_fd[i] , &readers_fd);
        if (sock_fd[i] > max_fd)
        {
            max_fd = sock_fd[i];
        }
    }//For is


}//Function establish connections

/***********************************************************************************/
/*                             RECEIVE MAJIORITY                                   */
/***********************************************************************************/
int recv_majority(struct TAG *maxTag,int *value,int ischeck)
{
    int bytes=0; //To validate the size of the received msg.
    struct timeval tv; //To set a time out for the select
    int i; //For loop statment
    char buffer[256];
    int readsocks=0;
    int majority=0; //Count the acks from the directories.
    int is_major=0;


    /*To store the message that receive from each server*/
    struct message msg;
    /*Initialize tag*/
   // msg.tag.num=0;
    //msg.tag.id=0;

    usleep(10000); //Sleep x miliseconds.
    do
    {
        //Wait 10 seconds to timeout.
        tv.tv_sec = 30;
        tv.tv_usec = 0;

        //printf("beforeDebugResult:%d\n" , triger);
        readsocks= select((max_fd + 1) , &readers_fd , (fd_set *) 0 ,(fd_set *) 0 , &tv);
        if (readsocks < 0)
        {
            printf("select error");
        }
        if(readsocks == 0)
        {
            printf("Timeout...Not receiving majority..");
            break;
        }

        #ifdef DEVLP
            printf("DebugResult:%d\n" , readsocks);
        #endif

        // if(result < 0){exit(-1);}
        for(i=0; i <AVAIL_SERVERS; i++)
        {
            if (FD_ISSET(sock_fd[i], &readers_fd))
            {
                bzero(buffer, sizeof(buffer));
                if ((bytes = recv(sock_fd[i], buffer, BUF_SIZE, 0)) < 0)
                {
                    perror("Failed client receive the message");
                }
                if (bytes < 0)
                {
                    printf("Unable to received the msg!Invalid bytes");
                }

                printf("MessageMajor: %s\n", buffer);


                /*Decode the message that you received from server*/
                decode(&msg,buffer);

                /*Check if you received the message that you wait..otherwise drop the msg*/
                if(msg.msg_id == message_id)
                {
                    ++is_major;
                    /*In case you wait the tag&value with the response message
                     * otherwise it's not nessesary to wait something*/
                    if (ischeck)
                    {
                        if ((maxTag->num < msg.tag.num)  || (maxTag->num == msg.tag.num && maxTag->id < msg.tag.id ))
                        {
                            (maxTag->num)=msg.tag.num;
                            (maxTag->id)=msg.tag.id;
                            *value=msg.value;
                        }
                    }
                    /*condition ? valueIfTrue : valueIfFalse*/
                }
                #ifdef DEVMODE
                else
                {
                    printf("Old message received, msg_id:%d",msg.msg_id);
                }
                #endif


                printf("Before going to is_major:%d\n" , is_major );
                if(is_major >=3)
                {
                    break;
                }
            }

        }//For statment

        printf("Outside\n");

    }while(is_major <  ((AVAIL_SERVERS/2) +1));

    if(is_major >= (AVAIL_SERVERS/2)+1)
    {
        return 1;
    }
    return -1;

}//Function recv_majority


/***********************************************************************************/
/*                             READER FUNCTION                                     */
/***********************************************************************************/
int reader(char *buffer , int avail_directories )
{
    //Temp files
    struct message msg;
    msg.type=(char *)malloc(sizeof(char) * (strlen("RWRITE")+1));
    strcpy(msg.type,"RREAD");
    msg.msg_id=++(message_id);

    int i;
    int bytes;
    /*Initialization*/
    struct TAG *tag;
    tag=(struct TAG*)malloc(sizeof(struct TAG));
    tag->num=-1;
    tag->id=-1;

    int value;

    bzero(buffer, sizeof(buffer));
    encode(&msg , buffer , msg.type );

    for(i=0; i < avail_directories; i++)
    {
        if (send(sock_fd[i], buffer, strlen(buffer), 0) < 0)
        {
            perror("send() failed\n");
            exit(EXIT_FAILURE);
        }
    }//For statment

    if(recv_majority(tag,&value,1) < 0)
    {
        printf("\nUnable to read  majiority in the first phase of reader\n");
    }
    #ifdef DEVMODE
        printf("TagNum:%d\n" , tag->num);
        printf("TagID:%d\n" , tag->id);
        printf("ABD READER SECOND PHASE\n");
    #endif

    /*Second phase of the ABD
     * Announce the maxtag&value that you found to
     * quarantine strong consistent */
    strcpy(msg.type,"RWRITE");
    msg.tag.num=tag->num;
    msg.tag.id=tag->id;
    msg.value= value;
    msg.msg_id=++(message_id);

    bzero(buffer, sizeof(buffer));
    encode(&msg , buffer , msg.type  );
    for(i=0; i < avail_directories; i++)
    {
        if (send(sock_fd[i], buffer, strlen(buffer), 0) < 0)
        {
            perror("send() failed\n");
            exit(EXIT_FAILURE);
        }
    }//For statment

    if(recv_majority(tag,&value,1) < 0)
    {
        printf("Unable to read  majiority in the second phase of reader\n");
    }

    printf("*************************************************************************\n");
    printf("Read operation completed: Value: %d , tagNum:%d , tagID:%d\n",value,tag->num,tag->id );
    printf("*************************************************************************\n");
}//Function reader

/***********************************************************************************/
/*                           WRITER FUNCTIO                                        */
/***********************************************************************************/
int writer(char *buffer , int avail_directories)
{
    //Temp files
    struct message msg;
    msg.type=(char *)malloc(sizeof(char) * (strlen("WREAD")+1));
    strcpy(msg.type,"WREAD");
    msg.msg_id=++(message_id);

    int i;
    int bytes;
    /*Initialization*/
    struct TAG *tag;
    tag=(struct TAG*)malloc(sizeof(struct TAG));
    tag->num=-1;
    tag->id=-1;

    int value;
    bzero(buffer, sizeof(buffer));
    encode(&msg , buffer , msg.type );

    for(i=0; i < avail_directories; i++)
    {
        if (send(sock_fd[i], buffer, strlen(buffer), 0) < 0)
        {
            perror("send() failed\n");
            exit(EXIT_FAILURE);
        }
    }//For statment

    if(recv_majority(tag,&value,1) < 0)
    {
        printf("\nUnable to read  majiority in the first phase of reader\n");
    }
    #ifdef DEVMODE
        printf("WRITER OPERATION FIRST PHASE");
        printf("TagNum:%d\n" , tag->num);
        printf("TagID:%d\n" , tag->id);
        printf("ABD READER SECOND PHASE\n");
    #endif

    /*Second phase of the ABD
     * Announce the maxtag&value that you found to
     * quarantine strong consistent */
    strcpy(msg.type,"WWRITE");
    msg.tag.num= ++(tag->num);
    msg.tag.id=  ++(tag->id);
    msg.value= 89;
    msg.msg_id=++(message_id);

    bzero(buffer, sizeof(buffer));
    encode(&msg , buffer , msg.type  );
    for(i=0; i < avail_directories; i++)
    {
        if (send(sock_fd[i], buffer, strlen(buffer), 0) < 0)
        {
            perror("send() failed\n");
            exit(EXIT_FAILURE);
        }
    }//For statment

    if(recv_majority(tag,&value,1) < 0)
    {
        printf("Unable to read  majiority in the second phase of reader\n");
    }


    printf("**********************************************************************************\n");
    printf("WRITE OPERATION COMPLETED , LAST_TAG_NUM:%d , LAST_TAG_IS:%d",msg.tag.num,msg.tag.id );
    printf("**********************************************************************************\n");


}//Function writer

/***********************************************************************************/
/*                              WRITE LOG                                          */
/***********************************************************************************/
int writeLog(FILE *log_file , char *info)
{
    log_file=fopen("client.log" , "a+");
    fprintf(log_file , info , sizeof(info));
    fclose(log_file);
}//Function writeLog

char* set_date()
{
    char namelog[100];
    time_t t = time(NULL);
    struct tm tm=*localtime(&t);
    sprintf(namelog,"client_%d_%d_%d.log",(tm.tm_mday),(tm.tm_mon+1),(tm.tm_year+1900));
    return namelog;
}//Fucntion setDate

/***********************************************************************************/
/*                                  MAIN                                           */
/***********************************************************************************/
int main(int agrc , char *argc[])
{
    /*********************************************/
    /*           LOCAL DECLARATION               */
    /********************************************/
    int bytes;
    int i; //Declare i to loop in all available servers.

    /*Declaration of buffers in order to store the sending&receiving
     *data* */
    char buf[BUF_SIZE]; //Buffer to store the send msg.

    /*********************************************/
    /*                 ALLOCATIONS               */
    /*********************************************/
    /*Allocation of a matrix in order to hold all file descriptor for each servers.*/
    sock_fd=(int*)malloc(AVAIL_SERVERS * sizeof(int));
    /*Parallel matrix to hold the listen port for each server.*/
    direct_ports=(int*)malloc(AVAIL_SERVERS * sizeof(int));
    /*Allocation of a parallel matrix to hold socket information for each server. */
    server=(struct sockaddr_in*)malloc(AVAIL_SERVERS * sizeof(struct sockaddr_in));

    /*Initialize the message id*/
    message_id=0;

    /*Inisialization  of the set of file descriptors.*/
    FD_ZERO(&readers_fd);

    //sprintf(buf,"%s,%d,%d,%d" ,msg.type , msg.msg_id , msg.tag , msg.value);

    //Establish a connection with all available diretories servers.
    establishConnections(AVAIL_SERVERS );

    //Read operation
    //reader(buf , AVAIL_SERVERS);
    writer(buf , AVAIL_SERVERS);




    return 0;
}
