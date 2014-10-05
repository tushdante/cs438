/*
 * myftclient.c
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
#include <netdb.h>
#include <sys/stat.h>

#include "dropper.h"
#include "libcommon.h"
ENABLE_DEBUG()


static char buffer[MAXBUFFER];
static int buffsize = 0, buffptr = 0;
static unsigned int seqnum;

/* does this need to be external, yes for get & transmit */
static struct sockaddr_in saddr; /* the server as the client sees them */
static unsigned int slen = sizeof(saddr);

int sock = -1;
FILE *fp = NULL;

extern int errno;


/* handler for SIGALRM */
void udpalarm() {}

int transmit(void *buff, int len) {
  int ret;
  
  /* UDP must send msg in one datagram because it preserves msg boundaries,
     so no loop send */
  if (-1 == (ret = sendto_dropper(sock, (void *) buff, len, 0, 
				  (struct sockaddr *) &saddr, slen))) {
    eprint("error sending!\n");
  } else if (ret != len) {
    eprint("sendto didn't send everything!\n");
    ret = -1;
  }
  return ret;
}

/* transmit the given data over the given socket */
/* IN UDP
   client is assumed to have connected b4 calling this
   server is assumed to have received b4 calling this
*/
int transmitinit(void *buff, int len) {
  int ret, retry = RETRY;

  do {
    aprint("sending init packet %u\n", seqnum);
    if (-1 == (ret = transmit(buff, len))) break;

    /* (re)set trap to catch alarm */
    if (SIG_ERR == signal(SIGALRM, udpalarm)) {
      eprint("couldn't set SIGALRM trap!\n");
      ret = -1;
      break;
    }
    /* set alarm in seconds */
    alarm(1);
    /* wait for any response */
    ret = recvfrom(sock, buff, MAXBUFFER, 0,
		   (struct sockaddr *) &saddr, &slen);
    /* check ERRNO for alarm interrupt */
    if (errno == EINTR) { /* got interrupt, ret will be = -1 also */
      aprint("init timed out, %d retries left\n", retry-1);
      ret = -2;
      errno = 0; /* reset errno */
    } else if (-1 == ret) {
      eprint("error receiving init ACK!\n");
      alarm(0); /* shut off the alarm */
      break;
    } else {
      alarm(0); /* shut off the alarm */
      print("init packet succcessful\n");
      break; /* success, dont retry */
    }
  } while (--retry);
  if (!retry && ret == -2) {
    print("abandoning init packet, it failed too many times\n");
    ret = -1;
  }
  return ret;
}

/*
given the hostname of the server in dotted decimal notation and a port #
attempts to create a socket and connect to the server returning the socket
handler
*/
int sconnect(char *hostname, int port) {
    int socket;
    struct hostent *he;
    struct in_addr a;

    socket = createUDPsocket();

    /* setup server socket address */
    memset((void *) &saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);

    /* get 32-bit IP address from host name - already in network byte order */
    if (NULL != (he = gethostbyname(hostname))) {
      /* printf("name: %s\n", he->h_name); */
      /*
	while (*he->h_aliases)
	printf("alias: %s\n", *he->h_aliases++);
      */
      while (*he->h_addr_list) {
	memcpy((char *) &a, *he->h_addr_list++, sizeof(a));
	aprint("set address: %s\n", inet_ntoa(a));
      }
    } else {
      eprint("unable to resolve hostname!\n");
      sclose(socket);
      exit(-1);
    }
    saddr.sin_addr.s_addr = a.s_addr;

    return socket;
}

void cleanexit() {
  fclose(fp); sclose(sock); exit(-1);
}

int main(int argc, char* argv[]) {
  int ret;
  unsigned int max_packet_size, packet_size, namelen;
 
  setDebug(&argc, argv);

  if (argc < 6) {
    eprint("Usage: myftclient <server_name> <server_port_number> ");
    eprint("<local_filename> <max_packet_size> <loss_percent>\n");
    exit(0);
  }
  aprint("server: %s", argv[1]); aprint(":%s\n", argv[2]);
  aprint("file: %s\n", argv[3]);
  aprint("max packet size: %s\n", argv[4]);
  aprint("loss percent: %s\n", argv[5]);

  set_dropper(atoi(argv[5]));
  max_packet_size = atoi(argv[4]);
  seqnum = 0;
  packet_size = (max_packet_size < MINBUFFER) ? max_packet_size : MINBUFFER;
  namelen = strlen(argv[3]);
  
  if (namelen + sizeof(initarq) > max_packet_size) {
    eprint("max packet size is too small for the given filename\n");
    exit(-1);
  }
  if (namelen + sizeof(initarq) > MINBUFFER) {
    eprint("protocol minimum buffer is too small for the given filename\n");
    exit(-1);
  }
  if (sizeof(initarq) > max_packet_size) {
    eprint("max packet size is too small, ridiculously small in fact...\n");
    exit(-1);
  }

  if (SIG_ERR == signal(SIGINT, sigint)) {
    eprint("couldn't set SIGINT trap!\n");
    exit(-1);
  }

  /* connect to send */
  sock = sconnect(argv[1], atoi(argv[2]));

  if (NULL == (fp = fopen(argv[3], "r"))) {
    perror("error opening file");
    cleanexit();
  } else {
    /* everything past here should clase fp and sock before exiting */
    
    /* send an inital max MINBUFFER sized packet until we get data from the
       server on credit, server is guaranteed to have a buffer of at least
       MINBUFFER bytes */
    int done = 0;
    unsigned int expects, credit;
    ack sack;

    initarq iarq; /* init ARQ control structure */
    iarq.seqnum = htonl(0);
    { /* get file size */
      struct stat finfo;
      if (-1 == stat(argv[3], &finfo)) {
	eprint("error stating file!\n");
	cleanexit();
      }
      iarq.flen = htonl(finfo.st_size);
    }
    iarq.namelen = htonl(namelen);
    aprint("file size: %u\n", ntohl(iarq.flen));
    
    buffptr = sizeof(initarq) + namelen;
    /* fill the buffer for the init packet */
    memcpy((void *) buffer, (void *) &iarq, sizeof(initarq));
    memcpy((void *) &buffer[sizeof(iarq)], (void *) argv[3], namelen);
    /* fill any empty space w/ file data */
    if (packet_size - buffptr > 0) {
      buffsize = fread((void *) &buffer[buffptr], 1, packet_size-buffptr, fp);
      if (ferror(fp)) {
	eprint("error reading from file!\n");
	cleanexit();
      } else if (feof(fp)) {
	done = 1;
	/* adjust packet_size since i may not have read enough to fill it */
	packet_size = buffsize + buffptr;
      }
    }
    /* transmits the init packet and waits for an ACK */
    if (-1 == transmitinit((void *) buffer, packet_size)) cleanexit();
    /* credit & next seqnum should be in buffer now */
    memcpy((void *) &sack, (void *) buffer, sizeof(ack));
    expects = ntohl(sack.expects);
    credit = ntohl(sack.credit);
    aprint("received ACK for %u", expects); aprint(" with %u credit\n", credit);

    /* do a sanity check */
    if (((int) expects) != buffsize) {
      eprint("server expected sequence does not match next seqnum!\n");
      aprint("next seqnum: %d\n", buffsize);
      cleanexit();
    }

    {
      arq carq; /* ARQ control structure */
      int retry = RETRY;
      unsigned int oldexpects = expects, startcredit, next;

      buffptr = sizeof(arq);
      while (!done && ret != -1) {
	startcredit = credit;
	next = expects;
	
	if (next == oldexpects) {
	  /* we are retrying the same window of packets */
	  if (--retry == 0) {
	    /* ran out of retries! */
	    eprint("send retry limit exceeded!\n");
	    cleanexit();
	  }
	} else {
	  /* reset the retry count */
	  retry = RETRY;
	  oldexpects = expects;
	}

	while (credit > 0 && !done) {
	  /* should be a rare case where credit is less than max packet size */
	  packet_size = (max_packet_size < credit) ? max_packet_size : credit;
	  /* if it is though the protocol will just act like Stop & Wait ARQ */

	  /* create a new packet */
	  carq.seqnum = htonl(next);
	  /* fill the buffer for the packet */
	  memcpy((void *) buffer, (void *) &carq, sizeof(arq));
	  buffsize = fread((void *) &buffer[buffptr], 1, 
			   packet_size - buffptr, fp);
	  if (ferror(fp)) {
	    eprint("error reading from file!\n");
	    cleanexit();
	  } else if (feof(fp)) {
	    done = 1;
	    /* adjust packet_size since i may not have read enough to fill it */
	    packet_size = buffsize + buffptr;
	  }
	  credit -= packet_size;

	  /* send the packet */
	  aprint("sending packet %u", next);
	  aprint(", %u credit left\n", credit);
	  transmit((void *) buffer, packet_size);
	  next += (unsigned int) buffsize;
	}
	/* ran out of credit - check for ACKs for more credit and that server
	   got all the packets, otherwise we must resend some packets */
	while(1) {
	  if (-1 == (ret = recvfrom(sock, buffer, MAXBUFFER, MSG_DONTWAIT,
				    (struct sockaddr *) &saddr, &slen))) {
	    if (errno != EWOULDBLOCK) {
	      eprint("error receiving on non-block!\n");
	      cleanexit();
	    } else { /* if it would block then i dont care just yet */
	      errno = ret = 0; /* not an error, just no data */
	    }
	  }

	  if (0 == ret && credit > 0 && !done) {
	    /* no ACKs to be processed and i have credit and not done,
	       so do some more transmitting */
	    break;
	  } else if ((0 == ret && credit <= 0) ||
		     (0 == ret && done && expects != next)) {
	    /* either no credit so we wait for some by letting it block or
	       we are done but we let it block a little to see if we can get
	       the final ACK */
	    /* set an alarm and wait a bit for some, if alarm goes off though
	       resend from last ACK */
	    /* (re)set trap to catch alarm */
	    if (SIG_ERR == signal(SIGALRM, udpalarm)) {
	      eprint("couldn't set SIGALRM trap!\n");
	      cleanexit();
	    }
	    /* set alarm in seconds */
	    alarm(1);
	    /* wait for any response */
	    ret = recvfrom(sock, buffer, MAXBUFFER, 0,
			   (struct sockaddr *) &saddr, &slen);
	    /* check ERRNO for alarm interrupt */
	    if (errno == EINTR) { /* got interrupt, ret will be = -1 also */
	      print("ACK timeout, resending packets\n");
	      errno = 0; /* reset errno */
	      /* reset credit so resend all last packet set */
	      credit = startcredit; 
	      break;
	    } else if (-1 == ret) { /* got receiving error */
	      eprint("error receiving ACK!\n");
	      cleanexit();
	    } /* else we received something! */
	    alarm(0); /* shut off the alarm */
	  } else if (0 == ret && done && expects == next) {
	    print("received last ACK, closing...\n");
	    break;
	  }
	  /* parse the ACK */
	  memcpy((void *) &sack, (void *) buffer, sizeof(ack));
	  /* since UDP doesnt guarantee order only take most recent values */
	  /* so just make sure its larger than the last ACK we got */
	  /* which means a larger seqnum expectation */
	  if (ntohl(sack.expects) > expects) {
	    expects = ntohl(sack.expects);
	    startcredit = credit = ntohl(sack.credit);
	    aprint("received ACK for %u", expects); 
	    aprint(" with %u credit\n", credit);
	  }
	}

	/* check if we have to resend */
	if (expects < next) {
	  /* reset fp to expects seqnum to resend */
	  if (-1 == (ret = fseek(fp, expects, SEEK_SET))) {
	    eprint("resend fseek on file failed!\n");
	    cleanexit();
	  }
	  done = 0; /* we have to resend so we aren't done yet */
	}
      }
    }
    fclose(fp);
  }
  sclose(sock);

  return 0;
}
