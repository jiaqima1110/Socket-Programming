.PHONY: all
all: serverM serverA serverB client

serverM: serverM.c
	gcc -o serverM -std=c11 -Wall -ggdb3 serverM.c
serverA: serverA.c
	gcc -o serverA -std=c11 -Wall -ggdb3 serverA.c
serverB: serverB.c
	gcc -o serverB -std=c11 -Wall -ggdb3 serverB.c
client: client.c
	gcc -o client -std=c11 -Wall -ggdb3 client.c

.PHONY: clean
clean:
	rm serverM serverA serverB client