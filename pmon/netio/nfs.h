#ifndef __NFS_H__
#define __NFS_H__

#define SUNRPC_PORT     111

#define NFS_READ_SIZE   8192


#define NFS_MAXLINKDEPTH 16

#define PROG_PORTMAP    100000
#define PROG_NFS        100003
#define PROG_MOUNT      100005

#define MSG_CALL        0
#define MSG_REPLY       1

#define PORTMAP_GETPORT 3

#define MOUNT_ADDENTRY  1
#define MOUNT_UMOUNTALL 4

#define NFS_LOOKUP      4
#define NFS_READLINK    5
#define NFS_READ        6

#define NFS_FHSIZE      32

#define NFSERR_PERM     1
#define NFSERR_NOENT    2
#define NFSERR_ACCES    13
#define NFSERR_ISDIR    21
#define NFSERR_INVAL    22


#define NFS_MAXLINKDEPTH 16
struct rpc_t {
    union {
        uint8_t data[128+NFS_READ_SIZE];
        struct {
            uint32_t id;
            uint32_t type;
            uint32_t rpcvers;
            uint32_t prog;
            uint32_t vers;
            uint32_t proc;
            uint32_t data[1];
        } call;
        struct {
            uint32_t id;
            uint32_t type;
            uint32_t rstatus;
            uint32_t verifier;
            uint32_t v2;
            uint32_t astatus;
            uint32_t data[19];
        } reply;
    } u;
};

#endif
