//
// Created by elias on 9/29/15.
//

#ifndef DISTRIBUTEDALGORITHM_CLIENT_H
#define DISTRIBUTEDALGORITHM_CLIENT_H


/************************************/
/*           STRUCTS                */
/************************************/

struct TAG
{
    int num;
    int id;
};

struct message
{
    char *type;
    int  msg_id;
    int  value;
    struct TAG tag ;
};





#endif //DISTRIBUTEDALGORITHM_CLIENT_H
