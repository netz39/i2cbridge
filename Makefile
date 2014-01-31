
.PHONY: all, local, clean

NAME=i2cbridge
CFLAGS=-Wall -ggdb

all: INCLUDE=-I/usr/local/include
all: LDFLAGS=-L/usr/local/lib -lwiringPi
all: $(NAME)

local: INCLUDE=-I$(HOME)/pro/lib/wiringPi/wiringPi
local: LDFLAGS=-L$(HOME)/pro/lib/wiringPi/wiringPi -lwiringPi
local: $(NAME)

$(NAME): $(NAME).c
	gcc $(CFLAGS) $(INCLUDE) $(LDFLAGS) -o $@ $<

clean: $(NAME)
	rm $(NAME)
