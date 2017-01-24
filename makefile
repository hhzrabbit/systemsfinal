compile: 
	gcc -o server.out server.c networking.c memctl.c
	gcc client.c networking.c -lncurses -o client.out

clean:
	rm -f *.out*
	rm -f *~
