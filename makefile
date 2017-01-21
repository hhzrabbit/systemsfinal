all: server client

server: server.o networking.o memctl.o
	gcc -o server server.o networking.o memctl.o

client: client.o networking.o memctl.o
	gcc -o client client.o networking.o memctl.o

server.o: server.c networking.h
	gcc -c server.c 

client.o: client.c networking.h
	gcc -c client.c

networking.o: networking.c networking.h
	gcc -c networking.c

memctl.o: memctl.c memctl.h
	gcc -c memctl.c

clean:
	rm -f *.o
	rm -f *~
