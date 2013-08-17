#ifndef _MPS_PROTOCOL_H
#define _MPS_PROTOCOL_H

//status structure
typedef struct _MPS_STATUS {
    unsigned short errclass;
    unsigned short errcode;
}MPS_STATUS;

#define MPS_OP_REG         1    //register a group
#define MPS_OP_UNREG       2    //unregister a group
#define MPS_OP_ADD         3    //add to a group
#define MPS_OP_QUIT        4    //quit from a group
#define MPS_OP_PUSH        5    //push message

//The header of MPS
#define MPS_IDENTIFIER   "MP\x34\x35"
typedef struct _MPS_HEADER {
    unsigned char ident[4];
    unsigned int  command;
    unsigned int  flags;
    MPS_STATUS    status;
    unsigned int  bytes;
}MPS_HEADER;
#define SZMPSHDR  (sizeof (MPS_HEADER))

#define ERR_CLASS_GROUP   2 //the error class for group
#define ERR_GROUP_EXISTS  1
#define ERR_GROUP_NO_FOUND 2
#define ERR_GROUP_FULL     3
#define ERR_GROUP_INVALID_GROUP 4

#define ERR_CLASS_MBR     3
#define ERR_MBR_NO_FOUND  1
#define ERR_MBR_EXISTS    2
#define ERR_MBR_FULL      3

/*
   +----------+-----------------------+--------------------+
   | command  |  request param        |  response param    |
   +----------+-----------------------+--------------------+
   | OP_REG   |      GID              |                    |
   |          |      max_num          |     GID            |
   |          |      group            |     UID            |
   +----------+-----------------------+--------------------+
   | OP_UNREG |      GID              |                    |
   +----------+-----------------------+--------------------+
   | OP_ADD   |      GID              |     UID            |
   |          |      UID              |                    |
   |          |      description      |                    |
   +----------+-----------------------+--------------------+
   | OP_QUIT  |      GID              |                    |
   |          |      UID              |                    |
   +----------+-----------------------+--------------------+
   | OP_PUSH  |      *MID             |                    |
   |          |      GID              |                    |
   |          |      UID              |                    |
   |          |      dstID            |                    |
   |          |      length           |                    |
   +----------+-----------------------+--------------------+
*/

#endif
