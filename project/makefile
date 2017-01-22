compile: 
	#gcc server.c networking.c -o server.out
	gcc chat_server.c networking.c memctl.c -g -o server.out
	gcc client.c networking.c -o client.out

clean:
	rm -f *.out
	rm -f *~
