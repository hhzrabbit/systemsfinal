all: client server

client: chat_client.c networking.c
	gcc chat_client.c networking.c -o client.out

server: chat_server.c networking.c
	gcc chat_server.c networking.c -o server.out

clean:
	rm -f *.out
	rm -f *~
