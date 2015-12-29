/***********************************************************************************/
/*                                                                                 */
/*Name: Elias Spanos                                                 			   */
/*Date: 30/09/2015                                                                 */
/*Filename: node.c                                                                 */
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
#include <netinet/in.h> //For variable sockaddr_in
#include "node.h"
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>


/***********************************************************************************/
/*                                     PROTOTYPES                                  */
/***********************************************************************************/
void *bind_thread();
void *accept_thread(void *accept_sock);

//Format for error for the threads
#define perror2(s,e) fprintf(stderr, "%s:%s\n" , s, strerror(e))

//The number for nodes that will participate on distributed algorithm
#define NODES_NO  3
#define MAX_QUEUE 100

/***********************************************************************************/
/*                             GLOBAL VARIABLES                                    */
/***********************************************************************************/
char Nodes_Ips[NODES_NO][100];        //Store all the names of nodes
u_int16_t *Nodes_ports;                 //Store all ports for each node
int  broad_sockets[(NODES_NO -1 )];     //The socket that you will use to do a broadcast
int  listen_socket;                     //To accept any client that interest to send you a msg

int  hasToken=0;                        //If i have the token the value is 1 otherwise is 0
int  req[NODES_NO];                     //The number of times that each nodes request to insert in critical section
int  last[NODES_NO];                    //The number when was the last time that you insert in critical section
int  queue[NODES_NO];                   //The nodes that interested to insert in critical section

struct sockaddr_in fd_nodes[NODES_NO];  //An array to store each socket descriptor for each node.
struct sockaddr *bindPtr , accept_node;                //A pointer struct for the bind
struct sockaddr *accept_nodePtr;
int NODE_ID=-1;                         //Store my id
int ID_PORT;                            //The ip that the process listen

struct sockaddr_in serv_addr,client_addr;
struct sockaddr *servPtr,*clientPtr ;
int sock;//Socket descriptor

//


/*Lock the strtok*/
pthread_mutex_t locker;

/*Lock the strtok*/
pthread_mutex_t file_locker;

int err;                                //Variable to store the error no if you received.

int isReq=0;
/***********************************************************************************/
/*                                DECODE MESSAGE                                   */
/***********************************************************************************/
void decode(struct message *msg , char *string)
{
    //This function decode the message that sent to each node. In this protocol
    //there are two different types of message. The first one is the IsToken type
    //where a node has finished from critical section and send the token with the
    //last queue. The second type of message is the IsREQ message where a node request
    //to insert in critical section.


    //Check the type of the message.
    msg->type=strdup(strtok(string, ","));

    if((strcmp(msg->type , "ISTOKEN"))==0)
    {
        int len=0 , i=0;
        msg->lenqueue = atoi(strtok(NULL, ","));
        len=msg->lenqueue;
        //delete.In case when you have to sent from queue
        if(len > 0){isReq=1;}

        //Read all the nodes id that exist in string.
        for(i=0; i<len; i++)
        {
            int tempQueue=atoi(strtok(NULL, ","));
            msg->queue[i]= tempQueue ;
            queue[i]=tempQueue;
        }

        msg->lenlast = atoi(strtok(NULL, ","));
        len=msg->lenlast;
        //Read all the nodes id that exist in string.
        for(i=0; i<len; i++)
        {
            int tempLast= atoi(strtok(NULL, ","));
            msg->last[i] =tempLast;
            last[i]=tempLast;
        }
        msg->NODEID=atoi(strtok(NULL, ","));
    }

    else if(strcmp(msg->type , "ISREQ")==0)
    {
        msg->NODEID=atoi(strtok(NULL, ","));
        msg->req = atoi(strtok(NULL, ","));
    }

}

void signalHandler()
{
    int i;
    close(listen_socket);

    for(i=0; i<NODES_NO; i++)
    {
        close(broad_sockets[i]);
    }
    exit(0);
}

/***********************************************************************************/
/*                           Read config information                               */
/***********************************************************************************/
void readConfigInfo(char *filename)
{
    FILE  *config;                   //File descriptor of the file containing nodes information
    int MAX_NODES;                  //Store the nodes the algorithm runs.
    char node_name[100];             //Store node name of config file
    int port;                       //Store port number retrieve from config file
    int i;                          //For loop statment

    //Opens config file that stores all the information
    // about the servers
    if(!(config = fopen("config.txt","r")))
    {
        perror("\nfopen config()");
        exit(-1);
    }

    //Read number of servers in the first line
    fscanf(config, "%d", &MAX_NODES);

    //Alocate two arrays one for the name of nodes and one for the ports
    Nodes_ports = (u_int16_t *) malloc(MAX_NODES * sizeof(int));

    //Go through each line of the config file and retrieve ip address & port of each node.
    for(fscanf(config,"%s%d", node_name,&port), i=0; !feof(config), i<MAX_NODES; fscanf(config,"%s%d", node_name,&port), i++)
    {
        strcpy(Nodes_Ips[i] , node_name);
        Nodes_ports[i]=port;
    }

}

/***********************************************************************************/
/*                        Initialization of variables & Sockets                    */
/***********************************************************************************/
void inisializations()
{
    int i=0; //Go through all the nodes.
    //Go through all the ports to find the node is and
    //store it in NODE_ID
    for(i=0; i< NODES_NO; i++)
    {
        if(ID_PORT == Nodes_ports[i])
        {
          NODE_ID=i;
          //Configure address_in struct for the specific NODE_ID
          fd_nodes[i].sin_family = AF_INET;
          fd_nodes[i].sin_addr.s_addr = inet_addr(Nodes_Ips[i]);
          fd_nodes[i].sin_port = htons(Nodes_ports[i]);
        }
        else
        {
            //Configure address_in struct for the specific NODE_ID
            fd_nodes[i].sin_family = AF_INET;
            fd_nodes[i].sin_addr.s_addr =  htonl(INADDR_ANY);
            fd_nodes[i].sin_port = htons(Nodes_ports[i]);
        }
    }

    //A pointer that point to the struct which is in bind_thread
    bindPtr=(struct sockaddr *) &fd_nodes[NODE_ID];

    //Set in position 0 of req array with value 1.
    for(i=0; i < NODES_NO; i++)
    {
        if(i==0)
        {
            req[i]=1;
        }
        else
        {
            req[i]=0;
        }
        queue[i]=-1;
        last[i]=0;

    }


    //Check the case where is the node 0 so you have to specify that have the token
    if(NODE_ID == 0)
    {
        hasToken=1;
    }

    /*Initialization of mutex for strtok in decode function*/
    pthread_mutex_init(&locker,NULL);

    /*Initialization of file locker*/
    pthread_mutex_init(&file_locker,NULL);

}

/***********************************************************************************/
/*                                  BIND THREAD                                    */
/***********************************************************************************/
void *bind_thread( )
{
    //Variable to store the thread id , type of pthread_t//
    pthread_t tid;

    //Store the size of node accept struct

    int newsock; //To hold the socket descriptor with each client

    int clientlen;/*To hold the fill struct after the accept.The size of the details for the accepted client*/

    accept_nodePtr = (struct sockaddr *) &accept_node;
    int nodelen=sizeof( accept_nodePtr);

    //printf("Node_ID: %d , Port: %d" , NODE_ID , Nodes_ports[NODE_ID]);

    /* Initialize socket structure */
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_addr.sin_port=htons(ID_PORT);

    servPtr=(struct sockaddr *) &serv_addr;


        /*Create a socket and check if is created correct.*/
        if((sock=socket(AF_INET,SOCK_STREAM,0)) < 0)
        {
            perror("Socket() failed"); exit(1);
        }

        /*Bind socket to address */
        if(bind(sock,servPtr, sizeof(serv_addr)) < 0)
        {
            perror("Bind() failed");
            exit(1);
        }

        /*Listen for connections */
        if(listen(sock , MAX_QUEUE) < 0)
        {
            perror("Listen() failed"); exit(1);
        }
        printf("\n**************Start to listen to port:%d*******************\n" , ID_PORT);


    // int accept_socket; //To establish a socket communication with a specific client
     while(1)
     {
        clientPtr=(struct sockaddr *) &client_addr;
        clientlen= sizeof(client_addr);

       // printf("***********Ready to accept connections to port:  %d *********\n" ,ID_PORT);
        if((newsock=accept(sock , clientPtr , &clientlen)) < 0)
        {
            perror("accept() failed"); exit(1);
        }

      //printf("Accepted connection from IP:%s , Port: %d \n" , inet_ntoa(client_addr.sin_addr) , ntohs(client_addr.sin_port) );
      //printf("*****************************\n");

        if(err=pthread_create(&tid , NULL , &accept_thread ,  (void *) newsock))
        {
            perror2("pthread_create for accept_thread" , err);
            exit(1);
        }
     }//While 1
}

/***********************************************************************************/
/*                                Accept_thread                                    */
/***********************************************************************************/
void *accept_thread(void *accept_sock)
{
  // printf("\n*******Running accept_thread*********************\n");
    int acpt_sock;
    char buffer[256];/*Buffer to send&receive data*/
    int msg_size; //The size of the msg that you received.


    acpt_sock= ((int)(accept_sock));

    bzero(buffer,sizeof(buffer));
    if((msg_size = recv(acpt_sock, buffer, sizeof(buffer),0)) < 0)
    {
        perror("Error received msg in bind_thread()");
        close(acpt_sock);
        exit(1);
    }

    printf("accept_thread received: %s\n" , buffer);


    /*Lock strtok due to is not deadlock free*/
    if(err=pthread_mutex_lock(&locker))
    {
        perror2("Failed to lock()",err);
    }

    //Struct to decode and store the received msg in the specific
    //struct.
    struct message msg;
    msg.type=(char *)malloc(sizeof(char) * 15);
    msg.queue = (int *)malloc(sizeof(int) * (NODES_NO));
    msg.last = (int *)malloc(sizeof(int) *  (NODES_NO));

    //Decode the message that you received from a node
    decode(&msg, buffer);

    /*Unloack Mutex*/
    if(err=pthread_mutex_unlock(&locker))
    {
        perror2("Failed to lock()",err);
    }

    //Check if the message type it request for the token.
    //If is true check also if this node has the token.
    if(strcmp(msg.type , "ISREQ")==0)
    {
            if(req[msg.NODEID] < msg.req )
            {
                req[msg.NODEID] = msg.req;

                /*Lock strtok due to is not deadlock free*/
                if(err=pthread_mutex_lock(&locker))
                {
                    perror2("Failed to lock()",err);
                }
                if(hasToken==1)
                {

                        int j;
                        for(j=0; j < NODES_NO; j++)
                        {
                           if(queue[j]== -1)
                            {
                                if(!IsInQueue(msg.NODEID) && req[j] ==last[j] + 1 )
                                {
                                    queue[j]= msg.NODEID;
                                    isReq=1;
                                }
                            }
                        }
                }
                /*Unloack Mutex*/
                if(err=pthread_mutex_unlock(&locker))
                {
                    perror2("Failed to lock()",err);
                }
            }
    }

    else if(strcmp(msg.type , "ISTOKEN")==0)
    {
        printf("\nNode: %d received the token from : %d \n", NODE_ID, msg.NODEID);
        hasToken=1;
    }

    //close accept socket
    close(acpt_sock);

    //Terminated thread
    pthread_exit(0);
}

/***********************************************************************************/
/*                           GENERATE RANDOM NUMBER                                */
/***********************************************************************************/
int genNo(int rand_NoIn)
{
    int randNo = 0;
    int MAX_N = rand_NoIn;
    int MIN_N = 4;

    srand(time(NULL));
    randNo = rand() % (MAX_N - MIN_N + 1) + MIN_N;

    return randNo;
}//Function calRandOddNo()

/***********************************************************************************/
/*                           BROADCAST VIA TCP                                     */
/***********************************************************************************/
void broadcast(char *msg)
{
    char buf[256];
    bzero(buf,sizeof(buf));
    int msglen=0; //The size of the message that the recv fuction received.

    int i=0;
    //Create socket & connection between  all the nodes.
    for(i=0; i< NODES_NO; i++)
    {
        if( i != NODE_ID)
        {
            /*Create a socket and check if is created correct.*/
            if ((broad_sockets[i] = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("Socket() failed");
                exit(1);
            }

            /*Establish a connection withe the IP address from the struct server*/
            if (connect(broad_sockets[i] , (struct sockaddr *) &fd_nodes[i], sizeof(fd_nodes[i])) < 0)
            {
                printf("Unable to Connect to : %s ,  PORT: %d\n", inet_ntoa(fd_nodes[i].sin_addr), ntohs(fd_nodes[i].sin_port));
            }
        }
    }

    //Send a request message to all nodes.
    for(i=0; i< NODES_NO; i++)
    {
       if( NODE_ID != i)
        {
            sprintf(buf,"ISREQ,%d,%d" , NODE_ID , req[NODE_ID]);
            printf("\nNode: %d is going to send a msg to Node: %d \n" , NODE_ID , i);
            send(broad_sockets[i] , buf,sizeof(buf) , 0);
        }
    }

    for(i=0; i< NODES_NO; i++)
    {
        if (NODE_ID != i)
        {
            close(broad_sockets[i]);

        }
    }

  }//Broadcast Function

void sentToken()
{

    /*Lock strtok due to is not deadlock free*/
    if(err=pthread_mutex_lock(&locker))
    {
        perror2("Failed to lock()",err);
    }
        hasToken=0;

    /*Unloack Mutex*/
    if(err=pthread_mutex_unlock(&locker))
    {
        perror2("Failed to lock()",err);
    }

    char buf[256]; //To store the message that
    //To store all the nodes that exist in queue.
    char sendQueue[256];

    char sendLast[256];

    //For loops statments
    int i;

    //The node number that i have to sent the token.
    int sendTokenTo=-1;
    int calQueueLen=0; //To calculate the length of the queue

    //Initialization of buffers
    bzero(sendQueue , sizeof(sendQueue));
    bzero(sendLast , sizeof(sendLast));

    char str[256];

    //Go through all the queue
    for(i=0; i<NODES_NO; i++)
    {
        if(queue[i] != -1 )
        {
            sendTokenTo=queue[i];
            queue[i]=-1;
            break;
        }
    }

    for(i=0; i<NODES_NO; i++)
    {
            if(queue[i] != -1)
            {
                bzero(str,sizeof(str));
                calQueueLen +=1;
                sprintf(str,",%d" , queue[i]);
                strcat(sendQueue,str);
            }
    }

    bzero(str, sizeof(str));
    //Go through all the queue
    for(i=0; i<NODES_NO; i++)
    {
        bzero(str,sizeof(str));
        sprintf(str,"%d" , last[i]);
        strcat(sendLast,str);

        if(i != (NODES_NO-1) )
        {
            strcat(sendLast, ",");
        }
    }

    //Clear queue and last queue.
    for(i=0; i<NODES_NO; i++)
    {
        last[i]=0;
        queue[i]=-1;
    }


    //Create a socket and check if is created correct.
    if ((broad_sockets[NODE_ID] = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket() failed");
        exit(1);
    }
    /*Establish a connection withe the IP address from the struct server*/
    if (connect(broad_sockets[NODE_ID] , (struct sockaddr *) &fd_nodes[sendTokenTo], sizeof(fd_nodes[1])) < 0)
    {
        printf("Unable to Connect to : %s ,  PORT: %d\n", inet_ntoa(fd_nodes[sendTokenTo].sin_addr), ntohs(fd_nodes[sendTokenTo].sin_port));
    }

    bzero(buf,sizeof(buf));
    if(calQueueLen !=0)
    {
        sprintf(buf,"ISTOKEN,%d%s,%d,%s,%d" ,calQueueLen, sendQueue , NODES_NO , sendLast , NODE_ID );
    }
    else
    {
        sprintf(buf,"ISTOKEN,%d,%d,%s,%d" ,calQueueLen, NODES_NO , sendLast,NODE_ID );
    }
    isReq=0;
    printf("\nNode: %d is going to send a msg to Node: %d , the msg: %s\n" , NODE_ID , sendTokenTo , buf );
    send(broad_sockets[NODE_ID] , buf,sizeof(buf) , 0);
    close(broad_sockets[NODE_ID]);

}

int IsInQueue(int nodeIn)
{
    int i=0;
    for(i=0; i < NODES_NO; i++)
    {
      if( queue[i] == nodeIn)
      {
          return 1;
      }
    }
    return 0;
}

int calTotalReq()
{
    int total=0;
    int i ;
    for(i=0; i < NODES_NO; i++)
    {
        total +=req[i];
    }

    total -= req[NODE_ID];

    return total;
}

void logExpirament(int totalReq , int loopsIn)
{



    if(err=pthread_mutex_lock(&file_locker))
    {
        perror2("Failed to lock()",err);
    }

    FILE *fp;
    char res[8];
    bzero(res,sizeof(res));
    sprintf(res,"%d\n" , (totalReq/loopsIn));

    fp=fopen("output.txt", "a+");
    fprintf(fp , res , sizeof(res));
    fclose(fp);

    /*Unloack Mutex*/
    if(err=pthread_mutex_unlock(&file_locker))
    {
        perror2("Failed to lock()",err);
    }


    printf("Average:%d\n" , (totalReq/loopsIn));


}

/***********************************************************************************/
/*                                  MAIN                                           */
/***********************************************************************************/
int main(int agrc , char *argc[])
{
    //Retrieve the port from the input parameter
    ID_PORT=atoi(argc[2]);

    //Handles signals
    signal(SIGINT, signalHandler); 	// ctrl-c

    //Variable to store the thread id , type of pthread_t//
    pthread_t tid[3];

    //Read configuration files(IPS,Ports)
    readConfigInfo("config.txt");

    //Read configuration files(IPS,Ports)
    inisializations();

    printf("----------------------------------------------------------");
    printf("\nNode:%d starting on port: %d.....\n" , NODE_ID , ID_PORT);
    printf("----------------------------------------------------------\n");

    sleep(3);
    //Create a thread for the bind.
    if(err=pthread_create(&tid[0] , NULL , &bind_thread , NULL))
    {
        perror2("pthread_create" , err);
        exit(1);
    }
      //For sychronization
      sleep(20);

  //the loop that the node need to insert in critical section
    //the loop that the node need to insert in critical section
    int counter=2;
    int loop=2;

    //For experamental evaluation
    int totalReqBefore =0;
    int totalReq;

  while(counter)
  {
      sleep(genNo(35));

    if(hasToken==0)
    {
        sleep(1);
        //Increase your request tries.
        req[NODE_ID] +=1;

        totalReqBefore = calTotalReq();
        broadcast("1");

        while(!hasToken){sleep(1);};
        totalReq += calTotalReq() - totalReqBefore ;
    }

    ////////////////////////////////////////////////////////////
    printf("----------------------------------------------------\n");
    printf("Node: %d entering critical section....\n" , NODE_ID);
      sleep(genNo(10));
    //Critical Sction
    /////////////////////
    last[NODE_ID]= req[NODE_ID];
    printf("Node: %d is exit the critical section....\n" , NODE_ID);
    printf("----------------------------------------------------\n");
    //Wait until to received a request for the token and send the token to the
    //node that request the token.
    while(isReq ==0);

    sentToken();

          sleep(genNo(30));
          counter -=1;
  }

    //Write a log with expiraments
    logExpirament(totalReq,loop);


    while(1);

    return 0;
}