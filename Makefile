
.PHONY: all, local, client, clean

NAME=i2cbridge
CFLAGS=-Wall -ggdb

all: INCLUDE=-I/usr/local/include
all: LDFLAGS=-L/usr/local/lib -lwiringPi
all: $(NAME)

local: INCLUDE=-I$(HOME)/pro/lib/wiringPi/wiringPi
local: LDFLAGS=-L$(HOME)/pro/lib/wiringPi/wiringPi -lwiringPi
local: $(NAME)

client: $(NAME)_iclient $(NAME)_uclient

$(NAME)_iclient: client_inet.c
	gcc $(CFLAGS) -o $@ $<

$(NAME)_uclient: client_unix.c
	gcc $(CFLAGS) -o $@ $<

$(NAME): $(NAME).c
	gcc $(CFLAGS) $(INCLUDE) $(LDFLAGS) -o $@ $<

clean: $(NAME)
	rm $(NAME)
