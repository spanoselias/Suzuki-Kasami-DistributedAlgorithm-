//
// Created by elias on 9/29/15.
//
#ifndef DISTRIBUTEDALGORITHM_REPLICA_H
#define DISTRIBUTEDALGORITHM_REPLICA_H

struct tagID
{
    int num;
    int id;
};

struct message
{
    char *type;
    int  msg_id;
    int  value;
    struct tagID  tag;
};

struct sockaddr_in serv_addr;

/***********************************************************************************/
/*                            PROTOTYPES                                           */
/***********************************************************************************/



#endif //DISTRIBUTEDALGORITHM_REPLICA_H
