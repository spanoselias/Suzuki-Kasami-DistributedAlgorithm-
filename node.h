/***********************************************************************************/
/*                                                                                 */
/*Name: Elias Spanos                                                 			   */
/*Date: 30/09/2015                                                                 */
/*Filename: node.h                                                                 */
/*                                                                                 */
/***********************************************************************************/

#ifndef EPL601_EX4_NODE_H
#define EPL601_EX4_NODE_H

struct message
{
    char *type;
    int  req;
    int  NODEID;
    int  *queue;
    int  lenqueue;
    int  *last;
    int  lenlast;
};

#endif //EPL601_EX4_NODE_H
