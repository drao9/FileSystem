#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <signal.h>
#include <strings.h>

static char *
inadport_decimal(sad)
        struct sockaddr_in *sad;
{
        static char buf[32];
        int a;

        a = ntohl(0xffffffff & sad->sin_addr.s_addr);
        sprintf(buf, "%d.%d.%d.%d:%d",
                        0xff & (a >> 24),
                        0xff & (a >> 16),
                        0xff & (a >> 8),
                        0xff & a,
                        0xffff & (int)ntohs(sad->sin_port));
        return buf;
}

int serve_socket(int port)
{
  int s;
	struct sockaddr_in sn;
  struct hostent *he;

  if (!(he = gethostbyname("localhost"))) {
    puts("can't gethostname");
    exit(1);
  }

  bzero((char*)&sn, sizeof(sn));
  sn.sin_family = AF_INET;
  sn.sin_port = htons((short)port);
	sn.sin_addr.s_addr = htonl(INADDR_ANY);

  if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket()");
    exit(1);
  }
	
  if (bind(s, (struct sockaddr *)&sn, sizeof(sn)) == -1) {
    perror("bind()");
    exit(1);
  }

  return s;
}

int accept_connection(int s)
{
  int l;
  int x;
	struct sockaddr_in sn;

  if(listen(s, 1) == -1) {
    perror("listen()");
    exit(1);
  }

	bzero((char *)&sn, sizeof(sn));
  l = sizeof(sn);
  if((x = accept(s, (struct sockaddr *)NULL, NULL)) == -1) {
    perror("accept()");
    exit(1);
  }
  return x;
}

int request_connection(char *hn, int port)
{
  struct sockaddr_in sn;
  int s, ok;
  struct hostent *he;

  if (!(he = gethostbyname(hn))) {
    puts("can't gethostname");
    exit(1);
  } 
  ok = 0;
  while (!ok) {
		bzero((char *)&sn,sizeof(sn));
    sn.sin_family = AF_INET;
    sn.sin_port  = htons((short)port);
		sn.sin_addr = *(struct in_addr *)(he->h_addr_list[0]);

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      perror("socket()");
      exit(1);
    }
    ok = (connect(s, (struct sockaddr*)&sn, sizeof(sn)) != -1);
    if (!ok) { sleep (1); perror("connect():"); }
  }
  return s;
}