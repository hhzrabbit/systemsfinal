compile: 
	#gcc server.c networking.c -o server.out
	gcc -g3 -o server.out chat_server.c networking.c memctl.c
	gcc client_ncurses.c networking.c -lncurses -o client.out

clean:
	rm -f *.out
	rm -f *~
