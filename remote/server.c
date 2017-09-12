#include <stdlib.h>
#include <stdio.h>
#include "sockettome.h"
#include <unistd.h>

int main(int argc, char *argv[]) {
	int port,sock;
	FILE * fin, *fout;
	char s[1000];
	int pid;
	int fd;

	//set port to serve
	port = atoi(argv[1]);
	sock = serve_socket(port);


	fd = accept_connection(sock);

	fin = fdopen(fd, "r+");

	pid = fork();
	if(pid==0) {
		if (dup2(fileno(fin), 0) != 0) {
			perror("Error: dup2(0) failed");
			exit(1);
		}
		if (dup2(fileno(fin), 1) != 1) {
			perror("Error: dup2(1) failed");
			exit(1);
		}

		fclose(fin);
		execl("../bin/sh",NULL);
		perror("messed up");
		exit(1);
	}
	else {
		wait(pid);
		exit(0);
	}		
}
