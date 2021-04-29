PORT=55272
FLAGS= -g -Wall -Werror -fsanitize=address
CFLAGS= -DPORT=\$(PORT) -g -Wall -Werror -fsanitize=address

xmodemserver: xmodemserver.o crc16.o
	gcc ${CFLAGS} -o $@ $^

client: client1.o crc16.o
	gcc ${FLAGS} -o $@ $^

%.o: %.c
	gcc ${FLAGS} -c $<

clean:
	rm -f *.o muffinman xmodemserver client1