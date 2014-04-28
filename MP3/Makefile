TARGETS = reliable_receiver reliable_sender

CC = gcc
CCOPTS= -Wall -Wextra -pthread -g

.PHONY: all clean

all: $(TARGETS)

clean:
	        rm -f $(TARGETS)

reliable_receiver: receiver_main.c
	        $(CC) $(CCOPTS) -o reliable_receiver receiver_main.c

reliable_sender: sender_main.c
	        $(CC) $(CCOPTS) -o reliable_sender sender_main.c
