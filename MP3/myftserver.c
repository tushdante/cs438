/*
 * myftserver.c
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
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <limits.h>

#include "dropper.h"
#include "libcommon.h"
ENABLE_DEBUG()


static char buffer[MAXBUFFER];
static char *buff = buffer;
static unsigned int windowsize;

int sock = -1; /* initialize with a static ptr */
FILE *fp = NULL;

extern int errno;


/* handler for SIGALRM */
void udpalarm() {}

/* write everything in the buffer to the file */
void flushbuffer() {
  char *buffwrite = buffer;
  unsigned int writelen;
  print("emptying buffer to file\n");
  
  /* write out all buffer data */
  while (buffwrite < buff) {
    memcpy((void *) &writelen, (void *) buffwrite, sizeof(arq));
    if (fwrite((void *) &buffwrite[sizeof(arq)], 1, writelen, fp) < writelen) {
      eprint("error writing to file!\n");
      continue; /* what else can i do? */
    }
    /* increment credit */
    windowsize += writelen + sizeof(arq);
    /* increment buffwrite */
    buffwrite = &buffwrite[writelen + sizeof(arq)];
  }
  /* reset buffer pointer to start of buffer */
  buff = buffer;	    
}

int main(int argc, char* argv[]) {
  struct sockaddr_in caddr; /* the client as the server sees them */
  unsigned int seqnum, newseqnum, ackseqnum = 0, clen = sizeof(caddr);
  unsigned int sendacknum, eofacknum = UINT_MAX;
  unsigned int flen = 0, datalen;
  int waiting = 1; /* flag for waiting for a new client or handling a client */
  int buffsize, buffptr = 0;
  ack sack;

  setDebug(&argc, argv);

  if (argc < 4) {
    eprint("Usage: myftserver <server_port_number> ")
    eprint("<buffer_size> <loss_percent>\n");
    return 0;
  }
  aprint("port: %s\n", argv[1]);
  aprint("buffer size: %s\n", argv[2]);
  aprint("loss percent: %s\n", argv[3]);

  set_dropper(atoi(argv[3]));
  windowsize = atoi(argv[2]);
  seqnum = 0;

  if (SIG_ERR == signal(SIGINT, sigint)) {
    eprint("couldn't set SIGINT trap!\n");
    exit(-1);
  }

  sock = createUDPsocket();
  /* set socket option to allow port to be reusable right after server closes */
  {
    int reuseaddr = 1;
    if (-1 == setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
			 &reuseaddr, sizeof(reuseaddr))) {
      eprint("error setting socket options!\n");
      sclose(sock);
      exit(-1);
    }
  }

  {
    struct sockaddr_in saddr;
    /* setup server socket address */
    memset((void *) &saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(atoi(argv[1]));
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (-1 == bind(sock, (struct sockaddr *) &saddr, sizeof(saddr))) {
      eprint("binding socket to port failed, port may already be in use.\n");
      sclose(sock);
      exit(-1);
    }
  }
  eprint("Listening for connections...\n");

  /* at this point the server should never crash unless closed by user */
  /* service assumes one client at a time in an iterative manner */
  while (1) {
    /* (re)set trap to catch alarm */
    if (!waiting) {
      if (SIG_ERR == signal(SIGALRM, udpalarm)) {
	eprint("couldn't set SIGALRM trap!\n");
      } else {
	/* set alarm in seconds */
	alarm(RETRY+5);
      }
    }
    /* receive some data */
    buffsize = recvfrom(sock, buff, windowsize, 0,
		       (struct sockaddr *) &caddr, &clen);
    if (errno == EINTR) { 
      /* got interrupt, client is having too many problems so give up on them */
      print("server is lonely, resetting state\n");
      flushbuffer();
      fclose(fp); 
      fp = NULL;
      waiting = 1;
      ackseqnum = seqnum = 0;
      eofacknum = UINT_MAX; 
      errno = 0;
      continue;
    } else if (-1 == buffsize) {
      alarm(0); /* shut off the alarm */
      eprint("error receiving from client!\n");
    } else {
      alarm(0); /* shut off the alarm */
      /* get seqnum of the new data */
      memcpy((void *) &newseqnum, (void *) buff, sizeof(unsigned int));
      newseqnum = ntohl(newseqnum);

      /* assuming one client at a time, so a seqnum == 0 during a client session
	 trashes the current clients session in favor of the new client */
      if (newseqnum == 0) { /* new file or client */
	/* parse the initarq */
	unsigned int namelen;
	initarq iarq;

	/* could detect and report concurrent client connections by checking 
	   if buff != buffer or fp != NULL but this would also catch false 
	   positives of a lost ACK on the initial packet */

	memcpy((void *) &iarq, (void *) buff, sizeof(initarq));
	flen = ntohl(iarq.flen);
	aprint("file length: %u\n", flen);
	namelen = ntohl(iarq.namelen);
	/* check for sane namelen */
	if (namelen > buffsize - sizeof(initarq)) {
	  /* filename string couldnt have fit in the first packet! */
	  eprint("received malformed init packet, file name length too long!");
	  continue;
	}

	if (NULL != fp) {
	  /* dont flush buffer b/c unnecessary & it will change buff pointer */
	  fclose(fp);
	  fp = NULL;
	}
	/* reset seqnums in case this is a repeat of packet 0 */
	ackseqnum = seqnum = 0;
	/* too late for previous client to get its final re-ACK so reset */
	eofacknum = UINT_MAX; 

	/* move stuff from buff where it was received to buffer which is
	   the beginning of the buffer which might trash the previous client
	   which might not have finished but oh well they are supposed to be
	   iterative and not concurrent */
	/* going to trash beginning of buffer to get a string for 
	   the filename */
	memmove((void *) buffer, (void *) &buff[sizeof(initarq)], namelen);
	/* dont overwrite the file being transmitted in case client and server
	   are on the same machine and in the same folder by concatenating 2 */
	buffer[namelen] = '2';
	buffer[namelen+1] = '\0'; /* null terminate the filename string */

	aprint("creating file %s\n", buffer);
	/* open file truncating to 0 for writing */
	if (NULL == (fp = fopen(buffer, "w"))) {
	  perror("error creating file");
	  errno = 0; /* reset error number */
	  /* reset buff pointer since we trashed beginning of buffer already */
	  buff = buffer;
	  /* pretend i didnt get this client.. */
	  continue;
	}
	waiting = 0; /* new client so dont wait */

	buffptr = sizeof(initarq) + namelen;
	datalen = buffsize - buffptr;

	/* move all the data down to make it look like a normal arq */
	memmove((void *) &buffer[sizeof(arq)], (void *) &buff[buffptr], 
		datalen);
	/* save size of data */
	memcpy((void *) buffer, (void *) &datalen, sizeof(arq));
	/* set buff to beginning in case of duplicate first */
	buff = buffer;
      } else { /* arg only has seqnum so its alread parsed */
	buffptr = sizeof(arq);
	datalen = buffsize - buffptr;
	/* save size of data  */
	memcpy((void *) buff, (void *) &datalen, sizeof(arq));	  
      }

      if (newseqnum + datalen == eofacknum && waiting) {
	/* last client didnt get the final ACK, 
	   so re-ACK only if not handling a client */
	print("duplicate last packet\n");
	sendacknum = eofacknum;
      } else if (newseqnum < ackseqnum) {
	/* we got a duplicate packet, ACK it*/
	aprint("duplicate packet %u,", newseqnum);
	aprint(" expecting %u!\n", ackseqnum);
      } else if (newseqnum > ackseqnum) {
	/* we missed a packet and this one is too far ahead */
	aprint("discarding future packet %u,", newseqnum);
	aprint(" expecting %u!\n", ackseqnum);
	/* DONT ACK IT */
	continue;
      } else { /* got right packet */
	/* move the buffer pointer past the data we just read */
	buff = &buff[datalen + sizeof(arq)];
	/* decrement the credit accordingly */
	windowsize -= datalen + sizeof(arq);

	/* ACK w/ next expected and check for EOF */
	seqnum = newseqnum;
	aprint("received packet %u\n", newseqnum);
	sendacknum = ackseqnum = newseqnum + datalen;


	if (ackseqnum > flen) {
	  /* client sent too much data according to their first packet flen */
	  datalen -= (ackseqnum - flen);
	  eprint("client sent extra data, only writing up to file length\n");
	  /* ACK them on how much i expected not how much they sent */
	  sendacknum = ackseqnum = flen; /* so file will close */
	}

	/* check if we need to write the buffer to the file */
	if (windowsize < datalen) {
	  flushbuffer();
	}

	if (ackseqnum == flen) { /* handle eof */
	  print("received last packet, closing file\n");
	  flushbuffer();
	  fclose(fp);
	  fp = NULL;
	  waiting = 1; /* finished w/ client wait for another */
	  eofacknum = flen; /* in case client misses final ACK */
	  ackseqnum = seqnum = 0; /* reset seqnums */
	}
      }
      /* prepare the ACK for the next seqnum we expect */
      aprint("sending ACK for %u", sendacknum); aprint(" with %u credit\n", windowsize);
      sack.expects = htonl(sendacknum);
      sack.credit = htonl(windowsize);
      /* send the ACK */
      if (-1 == sendto_dropper(sock, (void *) &sack, sizeof(ack), 0,
			       (struct sockaddr *) &caddr, clen))
	eprint("error sending ACK!\n");
    }
  }
  return 0;
}
