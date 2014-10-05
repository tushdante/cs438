/*
 * libcommon.c
 * CS 3251 Project 2
 * Andrew Trusty
 *
 * Licensed under the 
 * Creative Commons Attribution-NonCommercial-ShareAlike 2.5 License
 * For details of this license see:
 * http://creativecommons.org/licenses/by-nc-sa/2.5/
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "libcommon.h"


/* the debug flag all other files reference */
int debug = 0;

extern int sock;
extern FILE *fp;

//handle interrupt signals 
void sigint() {
  eprint("caught SIGINT, terminating...\n");
  if (NULL != fp) fclose(fp);
  sclose(sock);
  exit(0);
}

/* set the debug flag according to command line arguments and 
   shift down the other arguments overwriting the debug argument */
void setDebug(int *argc, char* argv[]) {
  if (*argc > 1) { /* debug is the only supported extra parameter */
    int i;

    for (i = 1; i < *argc; i++) {
      if (strncmp(argv[i], "-d", 2) == 0) {
	debug = 1;
	break;
      }
    }
    /* move other parameters down in array */
    if (debug) {
      for (; i < *argc; i++)
	if (i < (*argc)-1)
	  argv[i] = argv[i+1];
      (*argc)--;
    }
  }
}

/*
given the type of socket, creates a socket and
returns the socket handler
*/
int createUDPsocket() {
  int sock, socktype;

  socktype = SOCK_DGRAM;
  
  if (-1 == (sock = socket(PF_INET, socktype, IPPROTO_UDP))) {
    eprint("socket creation failed!\n");
    exit(-1);
  }
  print("created socket\n");  
  
  return sock;
}

/* close the given socket if it is open */
void sclose(int sock) {
  if (-1 == sock) return;
  if (-1 == close(sock)) {
    eprint("error closing socket! \n");
    exit(-1);
  }
  print("closed socket\n");
}
