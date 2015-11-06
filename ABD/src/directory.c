/***********************************************************************************/
/*                                                                                 */
/*Name: Elias Spanos                                                 			   */
/*Date: 30/09/2015                                                                 */
/*Filename: directory.c                                                            */
/*                                                                                 */
/***********************************************************************************/
/***********************************************************************************/
/*                                     LIBRARIES                                   */
/***********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h> //For bzero
#include <arpa/inet.h> //To retrieve the ip_add to asci
#include <string.h>
#include "directory.h"
#include <time.h>

#define MAX_PENDING 15 // Pending clients in

/*Compile program under development mode*/
#define DEVMODE
/*Compile program under development details mode*/
#define DEVMODED

#define DEBUG1
#define BUFSIZE 256
/***********************************************************************************/
/*                             GLOBAL VARIABLES                                    */
/***********************************************************************************/
struct sockaddr_in serv_addr;
FILE *log_fd = NULL; //File to store the log information of the directory server.
char *logfilename;

/***********************************************************************************/
/*                             SHARED OBJECT                                       */
/***********************************************************************************/
struct message obj;

/***********************************************************************************/
/*              FUNCTION TO CONFIGURE A CONNECTION FOR THE SERVER                  */
/***********************************************************************************/
int isconnect(int port)
{
    int sock;/*File descriptor for socket*/
    struct sockaddr *servPtr;// Pointer to Struct for the structure of the TCP/IP socket.
    servPtr=(struct sockaddr *) &serv_addr; //Point to struct serv_addr.
    /*Create a socket and check if is created correct.*/
    if((sock=socket(AF_INET,SOCK_STREAM,0)) < 0)
    {
            perror("Socket() failed");
            exit(1);
    }

    /***************************************/
    /*Represent shared object*/
    obj.type=NULL;
    obj.tag.id=5;
    obj.tag.num=10;
    obj.value=20;
    /*****************************************/

    /* Initialize socket structure */
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_addr.sin_port=htons(port);

    /*Accosiate address with the sock socket*/
    if(bind(sock,servPtr, sizeof(serv_addr)) < 0)
    {
        perror("Bind() failed"); exit(1);
    }

    /*Listen for connections */
    /*MAX_PENDING:the number of client that can wait in the queue.*/
    if(listen(sock ,MAX_PENDING) < 0)
    { /* 15 max. requests in queue*/
        perror("Listen() failed"); exit(1);
    }

    #ifdef DEVMODE
        printf("\n*************Start to listen to port:%d*******************\n" , port);
        printf("--------------------------------------------------------------\n");
    #endif

    return sock;
}//Function connect

/***********************************************************************************/
/*                             DECODE MSG                                          */
/***********************************************************************************/
void decode(struct message *msg , char *buf)
{
    #ifdef DEVMODED
        printf("Decode server: %s\n" , buf);
    #endif

    /*Use comma delimiter to find the type of
     * send message and retrieve the apropriate
     * fields*/
    msg->type=strdup(strtok(buf, ","));

    /*Check if the type of the message is RWRITE*/
    if( (strcmp(msg->type , "RWRITE" )== 0) || (strcmp(msg->type , "WWRITE" )== 0) )
        {
            msg->msg_id=atoi(strtok(NULL,","));
            msg->tag.num=atoi(strtok(NULL,","));
            msg->tag.id=atoi(strtok(NULL,","));
            msg->value=atoi(strtok(NULL,","));
        }
    /*Check if the type of the message is RREAD*/
    else if(strcmp(msg->type , "RREAD" )== 0)
        {
            msg->msg_id=atoi(strtok(NULL,","));
            msg->tag.num=atoi(strtok(NULL,","));
            msg->msg_id=atoi(strtok(NULL,","));
        }

    else if(strcmp(msg->type , "WREAD" )== 0)
        {
            msg->msg_id=atoi(strtok(NULL,","));
        }


}//Function decode message

/***********************************************************************************/
/*                              WRITE LOG                                          */
/***********************************************************************************/
int writeLog(char *info)
{
    log_fd=fopen(logfilename , "a+");
    fprintf(log_fd , info , sizeof(info));
    fclose(log_fd);
}//Function writeLog

/***********************************************************************************/
/*                            GET CURRENT DATE                                     */
/***********************************************************************************/
char* set_date()
{
    char namelog[100];
    time_t t = time(NULL);
    struct tm tm=*localtime(&t);
    sprintf(namelog,"directory_%d_%d_%d.log",(tm.tm_mday),(tm.tm_mon+1),(tm.tm_year+1900));
    return namelog;
}//Fucntion setDate

/***********************************************************************************/
/*                                  MAIN                                           */
/***********************************************************************************/
int main(int argc , char *argv[])
{

/*********************************************/
/*           LOCAL DECLARATION               */
/********************************************/
    int newsock; //Store server socket.
    int port=-1; //Store the port that server listen.
    int clientlen;//Store the size of the client address struct.
    int bytes; //The bytes that you receive from recv.
    int i=0;
    char phase[20];

    /*Set Up the log file name for the
     * diretory server as follow:
     * filename:"diretory_day_month_year"*/
    logfilename=(char*)malloc(strlen("diretory_xx_xx_xxxx"));
    strcpy(logfilename,set_date());

    /*Declare the structure of the sending message in order
     * the client to communicate with server and vice versa*/
    struct message msg;

    /*Declaration of buffers in order to store the sending&receiving
     * data* */
    char buf[256]; //Buffer to store the send msg.
    char logText[256]; //Buffer to store message for the logfile

    /*Declare&Initialize sockaddr_in pointer for accept function */
    struct sockaddr *clientPtr;
    struct sockaddr_in client_addr;
    clientPtr=(struct sockaddr *) &client_addr;
    clientlen= sizeof(client_addr);

/*********************************************/
/*           MEMORY ALLOCATIONS              */
/********************************************/

    /*Check if the input parameters are correct. */
    if(argc!=3)
    {
        printf("\nUsage: ./executable -p [port]\n");
        exit(-1);
    }
    /*store the port from the inputs*/
    port = atoi(argv[2]);

    /*Accosiate address with the socket*/
    /*and start to listen.*/
    int servSock=isconnect(port);

    #ifdef DEVMODE
            printf("********************\n");
            char hostname[60];
            gethostname(hostname, sizeof(hostname));
            printf("Server informations\n");
            printf("--------------------\n");
            printf("Listen to Port: %d \n" , port);
            printf("Server Hostname: %s\n" ,hostname  );
            printf("ServerIPAddr: %s\n" , inet_ntoa(serv_addr.sin_addr));
            printf("--------------------\n");
            printf("*********************************");
    #endif


    while(1)
        {
            if((newsock=accept(servSock , clientPtr  , &clientlen )) < 0)
            {
                perror("accept() failed"); exit(1);
            }

            #ifdef DEVMODE
                sprintf(logText , "Accepted connection from IP:%s , port:%d\n" , inet_ntoa(client_addr.sin_addr),port);
                writeLog(logText);
                printf("%s" , logText);
            #endif

            switch (fork())
                {
                    case -1:
                        perror("fork failed!\n");
                        close(newsock);
                        close(servSock);
                        exit(1);
                    case 0:
                        while(1)
                        {

                        #ifdef DEVMODED
                             printf("After while.....\n");
                        #endif

                        /*If connection is established then start communicating */
                        /*Initialize buffer with zeros */
                        bzero(buf,sizeof(buf));
                        /*Waiting...to receive data from the client*/
                        if (recv(newsock, buf, sizeof(buf), 0) < 0)
                        {
                            perror("send() failed");
                            close(newsock);
                            exit(-1);
                        }

                        /*Show the message that received.*/
                        printf("Received:%s\n",buf);

                        /*Decode the message that receive from the client
                         * in order to identify the type of the message*/
                        decode(&msg ,buf);

                        /*ABD ALGORITHM*/

                        /*Check if the message if of type RREAD*/
                        if((strcmp("RREAD",msg.type)==0))
                        {
                            #ifdef DEVMODE
                                printf("FIRST PHASE-RREAD ABD INFORM FROM CLIENT :%s\n",buf);
                            #endif


                            strcpy(phase,"RREAD");
                            bzero(buf,sizeof(buf));
                            /*<RREAD-OK,tag.num , tag.id ,value,  MSG_ID >*/
                            sprintf(buf,"RREAD-OK,%d,%d,%d,%d",obj.tag.num,obj.tag.id,obj.value,msg.msg_id);

                            #ifdef DEVMODED
                                printf("Response to RREAD: %s\n",buf);
                            #endif

                            if (bytes=send(newsock, buf, strlen(buf), 0) < 0)
                            {
                                perror("Send() failed");
                                exit(1);
                            }
                        }
                        else if(strcmp("WREAD",msg.type)==0)
                        {
                            #ifdef DEVMODE
                                 printf("FIRST PHASE-WREAD ABD INFORM FROM CLIENT :%s\n",buf);
                            #endif

                            strcpy(phase,"WREAD");
                            bzero(buf,sizeof(buf));
                            /*<WREAD-OK , tag_num , tag_id  , mid >*/
                            sprintf(buf,"WREAD-OK,%d,%d,%d",obj.tag.num,obj.tag.id, msg.msg_id);

                            #ifdef DEVMODED
                                printf("Response to RWRITE: %s\n",buf);
                            #endif

                            if (bytes=send(newsock, buf, strlen(buf), 0) < 0)
                            {
                                perror("Send() failed");
                                exit(1);
                            }

                        }//Else  if

                        /*Check if the message if of type RWRITE OR WWRITE*/
                        else if((strcmp("RWRITE",msg.type)==0) || (strcmp("WWRITE",msg.type)==0) )
                        {

                            if ((obj.tag.num < msg.tag.num) \
                                || (obj.tag.num == msg.tag.num && obj.tag.id < msg.tag.id ))
                            {
                                obj.tag.num=msg.tag.num;
                                obj.tag.id=msg.tag.id;
                                obj.value=msg.value;
                            }

                            bzero(buf,sizeof(buf));
                            if(strcmp("RWRITE",msg.type)==0)
                            {
                                #ifdef DEVMODE
                                    printf("SECOND PHASE-RWRITE ABD INFORM FROM CLIENT :%s\n",buf);
                                #endif

                                strcpy(phase, "RWRITE");
                                /*<RWRITE-OK,MSG_ID , >*/
                                sprintf(buf, "RWRITE-OK,%d\n", msg.msg_id);
                                #ifdef DEVMODED
                                    printf("Response to RWRITE: %s\n", buf);
                                #endif
                            }
                            else
                            {
                                #ifdef DEVMODE
                                     printf("SECOND PHASE-WWRITE ABD INFORM FROM CLIENT :%s\n",buf);
                                #endif

                                strcpy(phase, "WWRITE");
                                /*<WWRITE-OK,MSG_ID , >*/
                                sprintf(buf, "WWRITE-OK,%d\n", msg.msg_id);
                                #ifdef DEVMODED
                                    printf("Response to WWRITE: %s\n", buf);
                                #endif
                            }

                            if (bytes=send(newsock, buf, strlen(buf), 0) < 0)
                            {

                                perror("Send() failed");
                                exit(1);
                            }

                        }//PHASE RWRITE OR WWRITE

                        /*Initialize buffer for the log file with zeros */
                        bzero(logText, sizeof(logText));/*Initialize buffer with zeros */
                        writeLog(buf);/*Write to log file the receive message*/


                        #ifdef DEVMODEDD
                            if(strcmp(msg.type , "RWRITE" )== 0)
                                {
                                    sprintf(logText, "Received from IP: %s , PORT: %d , message:%s,%d,%d,%d,%d\n", \
                                            inet_ntoa(client_addr.sin_addr), port, msg.type, msg.msg_id, msg.tag.num,  \
                                            msg.tag.id, msg.value);
                                }
                            if(strcmp(msg.type , "RREAD" )== 0)
                                {
                                    sprintf(logText, "Received from IP: %s , PORT: %d , message:%s,%d,%d,%d\n", \
                                            inet_ntoa(client_addr.sin_addr), port, msg.type, msg.msg_id, msg.tag.num,  \
                                            msg.tag.id);
                                }
                                writeLog(logText);
                                printf("%s\n", logText);
                        #endif

                        #ifdef DEVMODEDD
                            bzero(logText, sizeof(logText));
                            sprintf(logText , "ACK , msg_id: %d , FROM_PORT :%d" , msg.msg_id ,port);
                        #endif

                        if (bytes=send(newsock, logText, strlen(logText), 0) < 0)
                        {
                            perror("Send() failed");
                            exit(1);
                        }

                        #ifdef DEVMODEDD
                        printf("----------------------------------------------------------------");
                        #endif

                        if( (strcmp(phase,"RWRITE")==0) || (strcmp(phase,"WWRITE")==0)   )
                        {
                            #ifdef DEVMODEDD
                                printf("Ending...");
                            #endif

                            /*Store the latest values to the logfile*/
                            bzero(logText, sizeof(logText));
                            sprintf(logText,"LATEST VALUES,%d,%d,%d,%d",obj.tag.num,obj.tag.id,obj.value,msg.msg_id);
                            writeLog(logText);
                            close(newsock);
                            exit(1);
                        }

                        }//While statment
                            ;
                }//Switch statment
        }//While(1)

    return 0;
}//Main Function