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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <linux/un.h>
#include <arpa/inet.h>

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
    return 0;
}

void con_del(int num)
{
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
            return 0;
    
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
    
    if(!(i2cs[i2c_count].fd = wiringPiI2CSetup(addr)))
    {
        fprintf(stderr, "Failed to setup i2c for 0x%04hx: %s\n", addr, strerror(errno));
        return -2;
    }
    i2cs[i2c_count].addr = addr;
    i2c_count++;
    return 0;
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
    c->count += count;
    
    return c->count != sizeof(struct request);
}

int con_request(int num)
{
    struct con *con = &cons[num];
    int fd;
    
    con->req.cmd = ntohs(con->req.cmd);
    con->req.addr = ntohs(con->req.addr);
    con->req.reg = ntohs(con->req.reg);
    con->req.data = ntohs(con->req.data);
    
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
        con->res.data = wiringPiI2CReadReg8(fd, con->req.reg) & 0xff;
        break;
    case I2CBRIDGE_CMD_WRITE8:
        con->res.data = wiringPiI2CWriteReg8(fd, con->req.reg, con->req.data);
        break;
    case I2CBRIDGE_CMD_READ16:
        con->res.data = wiringPiI2CReadReg16(fd, con->req.reg) & 0xffff;
        break;
    case I2CBRIDGE_CMD_WRITE16:
        con->res.data = wiringPiI2CWriteReg16(fd, con->req.reg, con->req.data);
        break;
    default:
        con->res.status = ERROR(COMMAND);
        return -1;
    }
    
    if(con->res.data == -1)
    {
        con->res.status = ERROR(I2C);
        fprintf(stderr, "Failed to access i2c for 0x%02hhx/0x%02hhx/0x%04hx: %s\n",
            con->req.addr, con->req.reg, con->req.data, strerror(errno));
        return -1;
    }
    
    con->res.status = ERROR(OK);
    
    return 0;
}

void cleanup(int signal)
{
    
    if(sock_inet)
        close(sock_inet);
    if(sock_unix)
    {
        close(sock_unix);
        snprintf(path, UNIX_PATH_MAX, "%s/%s.ipc", pwd, file_unix);
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
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = inaddr;
    
    if((sock_inet = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Failed to create inet socket");
        return 6;
    }
    
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

int setup_socket_unix()
{
    struct sockaddr_un addr;
    snprintf(addr.sun_path, UNIX_PATH_MAX, "%s/%s.ipc", pwd, file_unix);
    
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
    
    if(listen(sock_unix, BACKLOG) == -1)
    {
        perror("Failed to listen on unix socket");
        return 12;
    }
    
    return 0;
}

int main(int argc, char *argv[])
{
    int port = I2CBRIDGE_PORT;
    int daemon = 1;
    struct in_addr inaddr;
    
    int ret, x;
    
    char buf[10];
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
    
    while((ret = getopt(argc, argv, "hfp:w:l:u:")) != -1)
    {
        switch(ret)
        {
        case 'f':
            daemon = 0;
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
        case 'u':
            file_unix = optarg;
            break;
        case 'h':
        default:
            printf("Usage: %s [-f] [-p <port>] [-w <pwd>] [-l <ip>] [-u <unix>] \n", argv[0]);
            return 0;
        }
    }
    
    if(mkdir(pwd, 0766) == -1 && errno != EEXIST)
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
        write(ret, path, strlen(path));
        
        close(0);
        //close(1);
        //close(2);
        ret = open("/dev/null", O_RDWR);
        //dup(ret);
        //dup(ret);
    }
    
    signal(SIGTERM, cleanup);
    signal(SIGINT, cleanup);
    
    if((ret = setup_socket_inet(port, inaddr)))
        return ret;
    if((ret = setup_socket_unix()))
        return ret;
    
    i2c_count = 0;
    i2c_cap = 10;
    i2cs = malloc(i2c_cap*sizeof(struct i2c));
    
    con_count = 0;
    con_cap = 10;
    cons = malloc(con_cap*sizeof(struct con));
    pfds = malloc(con_cap*sizeof(struct pollfd));
    
    pfds[0].fd = sock_inet;
    pfds[0].events = POLLIN;
    pfds[1].fd = sock_unix;
    pfds[1].events = POLLIN;
    
    while(1)
    {
        if(poll(pfds, 2+con_count, -1) == -1)
        {
            perror("Failed to poll");
            return 13;
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
            if((ret = setup_socket_unix()))
            {
                cleanup(0);
                return ret;
            }
        }
        else if(pfds[1].revents & POLLIN)
        {
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
