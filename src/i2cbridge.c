/*
 * Copyright (c) 2014 Martin Rödel aka Yomin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#define _BSD_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <linux/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <wiringPiI2C.h>

#include "i2cbridge.h"

#define BACKLOG 10

#define request  i2cbridge_request
#define response i2cbridge_response

#define ERROR(X) I2CBRIDGE_ERROR_ ## X

struct con
{
    int count;
    struct request req;
    struct response res;
};

struct i2c
{
    int addr, fd;
};

char *pwd, *file_unix;
int sock_inet, sock_unix, lock, con_count, con_cap, i2c_count, i2c_cap;
struct pollfd *pfds;
struct con *cons;
struct i2c *i2cs;
char path[UNIX_PATH_MAX];


int con_add(int fd)
{
    void *tmp;
    
    if(con_count == con_cap)
    {
        con_count += 10;
        if(!(tmp = realloc(pfds, (2+con_cap)*sizeof(struct pollfd))))
        {
            perror("Failed to realloc poll structs");
            return -1;
        }
        pfds = tmp;
        if(!(tmp = realloc(cons, con_cap*sizeof(struct con))))
        {
            perror("Failed to realloc con structs");
            return -1;
        }
        cons = tmp;
    }
    pfds[2+con_count].fd = fd;
    pfds[2+con_count].events = POLLIN;
    memset(&cons[con_count], 0, sizeof(struct con));
    con_count++;
    
    printf("client added [%i]\n", fd);
    
    return 0;
}

void con_del(int num)
{
    printf("client removed [%i]\n", pfds[2+num].fd);
    
    close(pfds[2+num].fd);
    
    for(; num<con_count-1; ++num)
    {
        memcpy(&pfds[2+num], &pfds[2+num+1], sizeof(struct pollfd));
        memcpy(&cons[num], &cons[num+1], sizeof(struct con));
    }
    
    con_count--;
}

int i2c_add(int addr)
{
    int x;
    void *tmp;
    
    for(x=0; x<i2c_count; ++x)
        if(i2cs[x].addr == addr)
            return i2cs[x].fd;
    
    if(i2c_count == i2c_cap)
    {
        i2c_count += 10;
        if(!(tmp = realloc(i2cs, i2c_cap*sizeof(struct i2c))))
        {
            perror("Failed to realloc i2c array");
            return -1;
        }
        i2cs = tmp;
    }
    
    if((i2cs[i2c_count].fd = wiringPiI2CSetup(addr)) == -1)
    {
        fprintf(stderr, "Failed to setup i2c for 0x%04hx: %s\n", addr, strerror(errno));
        return -2;
    }
    i2cs[i2c_count].addr = addr;
    i2c_count++;
    
    printf("i2c device added [%hhx]\n", addr);
    
    return i2cs[i2c_count-1].fd;
}

int con_read(int num)
{
    struct con *c = &cons[num];
    char *ptr = ((char*)&c->req)+c->count;
    int size = sizeof(struct request)-c->count;
    int count;
    
    if((count = recv(pfds[2+num].fd, ptr, size, 0)) == -1)
    {
        perror("Failed to recv");
        return -1;
    }
    if(!count)
        return 2;
    
    c->count += count;
    return c->count != sizeof(struct request);
}

int con_request(int num)
{
    struct con *con = &cons[num];
    int fd, ret;
    
    printf("client request [%i]: %02hhx %02hhx %02hhx %04hx\n",
        pfds[2+num].fd, con->req.cmd, con->req.addr, con->req.reg, con->req.data);
    
    switch((fd = i2c_add(con->req.addr)))
    {
    case -1:
        con->res.status = ERROR(INTERNAL);
        return -1;
    case -2:
        con->res.status = ERROR(ADDRESS);
        return -1;
    }
    
    con->res.data = 0;
    
    switch(con->req.cmd)
    {
    case I2CBRIDGE_CMD_READ8:
        ret = wiringPiI2CReadReg8(fd, con->req.reg) & 0xff;
        break;
    case I2CBRIDGE_CMD_WRITE8:
        ret = wiringPiI2CWriteReg8(fd, con->req.reg, con->req.data);
        break;
    case I2CBRIDGE_CMD_READ16:
        ret = wiringPiI2CReadReg16(fd, con->req.reg) & 0xffff;
        break;
    case I2CBRIDGE_CMD_WRITE16:
        ret = wiringPiI2CWriteReg16(fd, con->req.reg, con->req.data);
        break;
    default:
        con->res.status = ERROR(COMMAND);
        return -1;
    }
    
    if(ret == -1)
    {
        con->res.status = ERROR(I2C);
        fprintf(stderr, "Failed to access i2c for 0x%02hhx/0x%02hhx/0x%04hx: %s\n",
            con->req.addr, con->req.reg, con->req.data, strerror(errno));
        return -1;
    }
    
    con->res.status = ERROR(OK);
    con->res.data = ret;
    
    return 0;
}

void cleanup(int signal)
{
    printf("cleanup\n");
    
    if(sock_inet)
        close(sock_inet);
    if(sock_unix)
    {
        close(sock_unix);
        snprintf(path, UNIX_PATH_MAX, "%s/%s", pwd, file_unix);
        unlink(path);
    }
    if(lock)
    {
        close(lock);
        snprintf(path, UNIX_PATH_MAX, "%s/pid", pwd);
        unlink(path);
    }
    
    if(pfds && cons)
    {
        while(--con_count > 0)
            close(pfds[2+con_count].fd);
        free(pfds);
        free(cons);
    }
    if(i2cs)
    {
        while(--i2c_count >= 0)
            close(i2cs[i2c_count].fd);
        free(i2cs);
    }
}

int setup_socket_inet(int port, struct in_addr inaddr)
{
    struct sockaddr_in addr;
    int reuse;
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = inaddr;
    
    if((sock_inet = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Failed to create inet socket");
        return 6;
    }
    
    reuse = 1;
    setsockopt(sock_inet, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
    
    if(bind(sock_inet, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1)
    {
        perror("Failed to bind inet socket");
        return 7;
    }
    
    if(listen(sock_inet, BACKLOG) == -1)
    {
        perror("Failed to listen on inet socket");
        return 8;
    }
    
    return 0;
}

int setup_socket_unix(int mode)
{
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, UNIX_PATH_MAX, "%s/%s", pwd, file_unix);
    
    if((sock_unix = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        perror("Failed to create unix socket");
        return 9;
    }
    
    if(unlink(addr.sun_path) == -1 && errno != ENOENT)
    {
        perror("Failed to unlink unix socket");
        return 10;
    }
    
    if(bind(sock_unix, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)) == -1)
    {
        perror("Failed to bind unix socket");
        return 11;
    }
    
    if(mode != -1 && chmod(addr.sun_path, mode) == -1)
    {
        perror("Failed to set permissions on unix socket");
        return 12;
    }
    
    if(listen(sock_unix, BACKLOG) == -1)
    {
        perror("Failed to listen on unix socket");
        return 13;
    }
    
    return 0;
}

int main(int argc, char *argv[])
{
    int port = I2CBRIDGE_PORT;
    int daemon = 1, verbose = 0, service = 0, mode = -1;
    struct in_addr inaddr;
    
    int ret, x;
    
    struct sockaddr_in addr_in;
    struct sockaddr_un addr_un;
    size_t addrsize;
    
    struct response err;
    
    pwd = I2CBRIDGE_PWD;
    file_unix = I2CBRIDGE_UNIX;
    inaddr.s_addr = INADDR_ANY;
    sock_inet = sock_unix = 0;
    lock = 0;
    pfds = 0;
    cons = 0;
    i2cs = 0;
    
    while((ret = getopt(argc, argv, "hfiuvp:w:l:s:m:")) != -1)
    {
        switch(ret)
        {
        case 'f':
            daemon = 0;
            break;
        case 'i':
            service |= 1;
            break;
        case 'u':
            service |= 2;
            break;
        case 'v':
            verbose = 1;
            break;
        case 'p':
            if(strspn(optarg, "1234567890") != strlen(optarg))
            {
                printf("port not numeric\n");
                return 1;
            }
            port = atoi(optarg);
            break;
        case 'w':
            pwd = optarg;
            break;
        case 'l':
            if(inet_aton(optarg, &inaddr) == -1)
            {
                printf("listen ip malformed\n");
                return 1;
            }
            break;
        case 's':
            file_unix = optarg;
            break;
        case 'm':
            if(strspn(optarg, "12345670") != strlen(optarg))
            {
                printf("mode not octal\n");
                return 1;
            }
            mode = strtol(optarg, 0, 8);
            break;
        case 'h':
        default:
            printf("Usage: %s [-f] [-i] [-u] [-v] [-p <port>] [-w <pwd>] [-l <ip>] [-s <unix>] [-m <mode>]\n", argv[0]);
            return 0;
        }
    }
    
    if(!service)
    {
        printf("Neither -i nor -u specified\n");
        return 0;
    }
    
    if(mkdir(pwd, 0755) == -1 && errno != EEXIST)
    {
        perror("Failed to create working dir");
        return 1;
    }
    
    if(chdir(pwd) == -1)
    {
        perror("Failed to change working dir");
        return 2;
    }
        
    if(daemon && getppid() != 1)
    {
        ret = fork();
        if(ret < 0)
        {
            perror("Failed to fork process");
            return 3;
        }
        if(ret > 0)
            return 0;
        
        setsid();
        
        if((lock = open("pid", O_RDWR|O_CREAT, 0644)) == -1)
        {
            perror("Failed to open pidfile");
            return 4;
        }
        
        if(lockf(lock, F_TLOCK, 0) == -1)
        {
            perror("Daemon already running");
            close(lock);
            return 5;
        }
        
        snprintf(path, UNIX_PATH_MAX, "%i\n", getpid());
        write(lock, path, strlen(path));
        
        close(0);
        if(!verbose)
            close(1);
        ret = open("/dev/null", O_RDWR);
        if(!verbose)
            dup(ret);
    }
    
    signal(SIGTERM, cleanup);
    signal(SIGINT, cleanup);
    
    if(service&1 && (ret = setup_socket_inet(port, inaddr)))
        return ret;
    if(service&2 && (ret = setup_socket_unix(mode)))
        return ret;
    
    i2c_count = 0;
    i2c_cap = 10;
    i2cs = malloc(i2c_cap*sizeof(struct i2c));
    
    con_count = 0;
    con_cap = 10;
    cons = malloc(con_cap*sizeof(struct con));
    pfds = malloc(con_cap*sizeof(struct pollfd));
    
    pfds[0].fd = sock_inet;
    pfds[0].events = service&1 ? POLLIN : POLLNVAL;
    pfds[1].fd = sock_unix;
    pfds[1].events = service&2 ? POLLIN : POLLNVAL;
    
    while(1)
    {
        if(poll(pfds, 2+con_count, -1) == -1)
        {
            if(errno == EINTR)
                return 0;
            perror("Failed to poll");
            return 14;
        }
        if(pfds[0].revents & POLLHUP || pfds[0].revents & POLLERR)
        {
            fprintf(stderr, "hup/error on inet socket\n");
            close(sock_inet);
            if((ret = setup_socket_inet(port, inaddr)))
            {
                cleanup(0);
                return ret;
            }
        }
        else if(pfds[0].revents & POLLIN)
        {
            addrsize = sizeof(struct sockaddr_in);
            if((ret = accept(sock_inet, (struct sockaddr*)&addr_in, &addrsize)) == -1)
            {
                perror("Failed to accept inet connection");
                continue;
            }
conadd:     if(con_add(ret) == -1)
            {
                err.status = ERROR(INTERNAL);
                send(ret, &err, sizeof(struct response), 0);
                close(ret);
            }
        }
        else if(pfds[1].revents & POLLHUP || pfds[1].revents & POLLERR)
        {
            fprintf(stderr, "hup/error on unix socket\n");
            close(sock_unix);
            if((ret = setup_socket_unix(mode)))
            {
                cleanup(0);
                return ret;
            }
        }
        else if(pfds[1].revents & POLLIN)
        {
            addrsize = sizeof(struct sockaddr_un);
            if((ret = accept(sock_unix, (struct sockaddr*)&addr_un, &addrsize)) == -1)
            {
                perror("Failed to accept unix connection");
                continue;
            }
            goto conadd;
        }
        else for(x=0; x<con_count; ++x)
        {
            if(pfds[2+x].revents & POLLHUP || pfds[2+x].revents & POLLERR)
            {
                con_del(x);
            }
            else if(pfds[2+x].revents & POLLIN)
            {
                switch(con_read(x))
                {
                case 1: // incomplete request
                    break;
                case 2: // shutdown
                    con_del(x);
                    x = con_count;
                    break;
                case -1:
                    cons[x].res.status = ERROR(INTERNAL);
                    pfds[2+x].events = POLLOUT;
                    break;
                case 0:
                    con_request(x);
                    pfds[2+x].events = POLLOUT;
                    break;
                }
            }
            else if(pfds[2+x].revents & POLLOUT)
            {
                if(send(pfds[2+x].fd, &cons[x].res, sizeof(struct response), 0) == -1)
                {
                    perror("Failed to send");
                    con_del(x);
                    break;
                }
                switch(cons[x].res.status)
                {
                case ERROR(OK):
                case ERROR(I2C):
                    pfds[2+x].events = POLLIN;
                    memset(&cons[x], 0, sizeof(struct con));
                    break;
                default:
                    con_del(x);
                }
            }
        }
    }
    
    return 0;
}
