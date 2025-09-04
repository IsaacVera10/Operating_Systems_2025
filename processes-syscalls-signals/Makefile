cc		= gcc
cflags	:= -Wall -g

exec	:= zombie fork1 fork2 sigaction
obj		:= $(foreach name, $(exec), $name.c)

all: $(exec)

%.o: %.c
	$(cc) $(cflags) $< -o $@

.PHONY: clean

clean:
	rm -f $(exec)
