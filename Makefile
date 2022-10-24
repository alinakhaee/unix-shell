all:
	chmod 755 unixShell.c
	gcc unixShell.c -pthread -o shell

clean:
	rm *o shell