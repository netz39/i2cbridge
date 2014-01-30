
.PHONY: all, clean

NAME=i2cbridge

INCLUDE=-I../../lib/wiringPi
LDFLAGS=-L../../lib/wiringPi/wiringPi -lwiringPi
CFLAGS=-Wall $(INCLUDE)

$(NAME): $(NAME).c
	gcc $(CFLAGS) $(LDFLAGS) -o $@ $<

clean: $(NAME)
	rm $(NAME)
