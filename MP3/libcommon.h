/*
 * libcommon.h
 * CS 3251 Project 2
 * Andrew Trusty
 *
 * Licensed under the 
 * Creative Commons Attribution-NonCommercial-ShareAlike 2.5 License
 * For details of this license see:
 * http://creativecommons.org/licenses/by-nc-sa/2.5/
 */ 

#ifndef __LIBCOMMON__
#define __LIBCOMMON__

/* print functions for debug mode only */
#define ENABLE_DEBUG() extern int debug;
#define print(x) if (debug) { printf(x); fflush(stdout); }
#define aprint(x,a) if (debug) { printf(x, a); fflush(stdout); }

/* error or non-debug printing */
#define eprint(x) printf(x); fflush(stdout);

#define MAXBUFFER 65536 /* double the max size project says */
#define RETRY 25 /* in UDP the number of re-transmissions of an un-ACKed msg */
#define MINBUFFER 1024 /* smallest buffer the server has */

/* Control structures used as headers on payloads */
/* all fields should be in network byte order */
/* can be sent across network w/o padding concerns 
   b/c all fields are same 4 byte */
/* header for the initial packet */
typedef struct {
  unsigned int seqnum, flen, cwnd;
  /*  char filename[namelen]; <-- implicit */
} initarq;
/* header for all other packets */
typedef struct {
  unsigned int seqnum, cwnd;  
} arq;
/* server ACK packets */
typedef struct {
  unsigned int expects;
} ack;

/* handles interrupts - closes fp and sock and exits */
void sigint();

/* set the debug flag according to command line arguments and 
   shift down the other arguments overwriting the debug argument */
void setDebug(int *argc, char* argv[]);

/*
given the type of socket, creates a socket and
returns the socket handler
*/
int createUDPsocket();

/* close the given socket if it is open */
void sclose(int sock);


#endif
