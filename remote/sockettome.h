#ifndef _SOCKETTOME
#define _SOCKETTOME

extern int serve_socket(int port);
extern int accept_connection(int s);
extern int request_connection(char *hn, int port);

#endif