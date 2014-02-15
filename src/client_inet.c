
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>

#include "i2cbridge.h"

#define STR(X) #X
#define STREXP(X) STR(X)
#define printfv(f, ...) if(verbose) printf(f, #__VA_ARGS__)

void usage(char *name)
{
    printf("Usage: %s [-v] [-p <port>] <dest> <cmd> <addr> <reg> [<data>]\n", name);
    printf("dest: ip, hostname\n");
    printf("cmd: read8, read16, write8, write16\n");
    printf("addr/reg/data: hex\n");
    exit(-1);
}

int main(int argc, char *argv[])
{
    int sock;
    char *dest, *port = STREXP(I2CBRIDGE_PORT);
    int verbose = 0;
    
    int opt, ret;
    struct addrinfo hints, *hres, *iter;
    struct i2cbridge_request req;
    struct i2cbridge_response res;
    
    while((opt = getopt(argc, argv, "vp:")) != -1)
    {
        switch(opt)
        {
        case 'p':
            if(strspn(optarg, "1234567890") != strlen(optarg))
            {
                printf("port not numeric\n");
                return -1;
            }
            port = optarg;
            break;
        case 'v':
            verbose = 1;
            break;
        default:
            usage(argv[0]);
        }
    }
    
    if(argc-optind < 4 || argc-optind > 5)
        usage(argv[0]);
    
    dest = argv[optind];
    memset(&req, 0, sizeof(struct i2cbridge_request));
    
    if(!strcmp("read8", argv[optind+1]))
        req.cmd = I2CBRIDGE_CMD_READ8;
    else if(!strcmp("read16", argv[optind+1]))
        req.cmd = I2CBRIDGE_CMD_READ16;
    else if(!strcmp("write8", argv[optind+1]))
        req.cmd = I2CBRIDGE_CMD_WRITE8;
    else if(!strcmp("write16", argv[optind+1]))
        req.cmd = I2CBRIDGE_CMD_WRITE16;
    else
        usage(argv[0]);
    
    if(!sscanf(argv[optind+2], "%hhx", &req.addr))
    {
        printf("addr not hex\n");
        return -1;
    }
    if(!sscanf(argv[optind+3], "%hhx", &req.reg))
    {
        printf("reg not hex\n");
        return -1;
    }
    if(optind == argc-5 && !sscanf(argv[optind+4], "%hx", &req.data))
    {
        printf("data not hex\n");
        return -1;
    }
    
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Failed to open socket");
        return -2;
    }
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    if((ret = getaddrinfo(dest, port, &hints, &hres)) != 0)
    {
        printf("Failed to resolve server: %s\n", gai_strerror(ret));
        return -3;
    }
    
    for(iter=hres; iter; iter=iter->ai_next)
        if(!(ret = connect(sock, iter->ai_addr, iter->ai_addrlen)))
            break;
    
    freeaddrinfo(hres);
    
    if(ret == -1)
    {
        perror("Failed to connect");
        return -4;
    }
    
    if(send(sock, &req, sizeof(struct i2cbridge_request), 0) == -1)
    {
        perror("Failed to send");
        return -5;
    }
    
    if(recv(sock, &res, sizeof(struct i2cbridge_response), 0) == -1)
    {
        perror("Failed to recv");
        return -6;
    }
    
    switch(res.status)
    {
    case I2CBRIDGE_ERROR_OK:
        printfv("response: ok\ndata: 0x");
        printf("%04hx\n", res.data);
        break;
    case I2CBRIDGE_ERROR_INTERNAL:
        printfv("response: internal error\n");
        break;
    case I2CBRIDGE_ERROR_COMMAND:
        printfv("response: unknown command\n");
        break;
    case I2CBRIDGE_ERROR_ADDRESS:
        printfv("response: device with address not found\n");
        break;
    case I2CBRIDGE_ERROR_I2C:
        printfv("response: error while accessing i2c bus\n");
        break;
    }
    
    close(sock);
    
    return res.status;
}
