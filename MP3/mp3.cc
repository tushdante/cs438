/*
 * mp3.cc - source code for read refragmentation and logging
 *
 * "Copyright (c) 2001 by Steven S. Lumetta."
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE AUTHOR OR THE UNIVERSITY OF ILLINOIS BE LIABLE TO ANY
 * PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT  OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE
 * AUTHOR AND/OR THE UNIVERSITY OF ILLINOIS HAS BEEN ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 * 
 * THE AUTHOR AND THE UNIVERSITY OF ILLINOIS SPECIFICALLY DISCLAIM ANY
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED
 * HEREUNDER IS ON AN "AS IS" BASIS, AND NEITHER THE AUTHOR NOR THE UNIVERSITY
 * OF ILLINOIS HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES,
 * ENHANCEMENTS, OR MODIFICATIONS."
 *
 * Author:          Steve Lumetta
 * Version:         1
 * Creation Date:   Mon Sep  3 16:31:22 2001
 * Filename:        mp3.cc
 * History:
 *      SL  1   Mon Sep  3 16:31:22 2001
 *      First written.
 *      Feb 5 2009 by kung
 *      Read "mp3param" for 3 parameters.
 *      Add simulated router queues for testing congestion control.
 */

#include <queue>
#include <pthread.h>
/* #include <signal.h> */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stropts.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "mp3.hh"

#define BUF_SIZE     20000         /* limit on buffered data */
#define DEFAULT_SEED 0x18245189    /* default random seed    */
#define SEED_FILE    "mp3param"    /* random seed file name  */
#define RESULTS_FILE "mp3_results" /* file name for results  */

using namespace std;

/* add by Luchuan Kung           */
/* by default channel is perfect */
static int gl_drop = 0;
static int gl_garble = 0;

class Packet {
    public:
        Packet(int _sockfd, void *_buff, size_t _nbytes, int _flags,
               const struct sockaddr *_to, socklen_t _addrlen)
        {
            sockfd = _sockfd;
            buff = new (nothrow) char[_nbytes];
            memcpy(buff, _buff, _nbytes);
            nbytes = _nbytes;
            flags = _flags;
            to = *_to;
            addrlen = _addrlen;
        }

        ~Packet(void)
        {
            delete[] buff;
        }

        ssize_t send(void)
        {
            return sendto(sockfd, buff, nbytes, flags, &to, addrlen);
        }

    private:
        int sockfd;
        char *buff;
        size_t nbytes;
        int flags;
        struct sockaddr to;
        socklen_t addrlen;
};

static size_t MAX_INPUT_QUEUE_SIZE = 5;
static size_t MAX_OUTPUT_QUEUE_SIZE = 5;
static size_t MAX_PACKET_SIZE = 100;
static useconds_t PACKET_SWITCHING_TIME = 10000;
static useconds_t PACKET_TRANSMISSION_TIME = 10000;
static queue<Packet *> input_queue;
static pthread_mutex_t input_queue_mutex;
static queue<Packet *> output_queue;
static pthread_mutex_t output_queue_mutex;

static void init_mp3_seed(void);
static void *process_input_queue(void *arg);
static void *process_output_queue(void *arg);
static void record_results(int bytes);
/*
 * Code from
 * http://www.gnu.org/software/libtool/manual/libc/Elapsed-Time.html
 */
/* static int timeval_subtract (struct timeval *result, struct timeval *x, */
/*                              struct timeval *y);                        */

void
mp3_init(void)
{

    pthread_attr_t attr;
    pthread_t thread;

    pthread_mutex_init(&input_queue_mutex, NULL);
    pthread_mutex_init(&output_queue_mutex, NULL);

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&thread, &attr, process_input_queue, NULL);
    pthread_create(&thread, &attr, process_output_queue, NULL);
    pthread_attr_destroy(&attr);

    init_mp3_seed();
}

ssize_t
mp3_sendto(int sockfd, void *buff, size_t nbytes, int flags,
           const struct sockaddr *to, socklen_t addrlen)
{
    char charBuff[nbytes + 1];
    memcpy(charBuff, buff, nbytes);
    int percentage = (random() % 100) + 1;

    /* Input argument checking to help debugging */   
    if (sockfd < 0) {
        fprintf(stderr, "Socket descriptor %d passed to mp3_sendto\n", sockfd);
        exit(3);
    }

    if (buff == NULL) {
        fprintf(stderr, "Null buffer passed to mp3_sendto.\n");
        exit(3);
    }

    if (nbytes < 0) {
        fprintf(stderr, "Negative bytes %d requested to mp3_sendto.\n", nbytes);
        exit(3);
    }

    if (nbytes == 0) {
        fprintf(stderr, "Warning: Zero bytes requested to mp3_sendto.\n");
        return 0;
    }

    if (nbytes > MAX_PACKET_SIZE) {
        fprintf(stderr, "Warning: %u bytes requested to mp3_sendto.\n", nbytes);
        return 0;
    }
    
    if (percentage <= gl_drop) { /* drop message */
        /* do nothing */
        printf("message dropped.\n");
        return 0;
    } else if (percentage <= gl_garble) { /* garble a random byte of the buff */
        printf("message garbled\n");
        /* choose a single byte to garble */
        int tochg = random() % nbytes;
        unsigned char randbyte = (unsigned char)(random() & 0xff);
        charBuff[tochg] = charBuff[tochg] ^ randbyte; 
    }

    /* Push the packet into the input queue -- sp11 */
    pthread_mutex_lock(&input_queue_mutex);

    if (input_queue.size() < MAX_INPUT_QUEUE_SIZE) {
        Packet *p = new Packet(sockfd, buff, nbytes, flags, to, addrlen);
        input_queue.push(p);
    }
    pthread_mutex_unlock(&input_queue_mutex);

    return nbytes;
}

ssize_t 
mp3_read(int fildes, void *buf, size_t nbyte)
{
    int amt, avail, n_read;
    static size_t buf_cnt = 0;
    static int first_call = 1, mp3_fd = -1;
    static unsigned char local_buf[BUF_SIZE];

    /* Input argument checking, to help with debugging. */
    if (fildes < 0) {
        fprintf(stderr, "File descriptor %d passed to mp3_read.\n",
                fildes);
        exit (3);
    }

    if (buf == NULL) {
        fputs("Null buffer passed to mp3_read.\n", stderr); 
        exit(3);
    }

    if (nbyte < 0) {
        fprintf(stderr, "Negative bytes (%d) requested from mp3_read.\n",
                nbyte);
        exit(3);
    }

    if (nbyte == 0) {
        fputs("Warning: Zero bytes requested from mp3_read.\n", stderr);
        return 0;
    }

    if (first_call) {
        /* Record file descriptor. */
        mp3_fd = fildes;
        /* Initialize random seed. */
        init_mp3_seed();
        first_call = 0;
    } else if (fildes != mp3_fd) {
        fputs("mp3_read called on more than one descriptor!\n", stderr);
        fprintf(stderr, "   (first on %d, last on %d)\n", mp3_fd,
                fildes);
        exit(3);
    }

    /* Loop until we're ready to return some data. */
    while (1) { 
        /* Check for available bytes. */
        if (ioctl(fildes, I_NREAD, &avail) == -1) {
            perror("ioctl to read available bytes");
            exit(3);
        }

        /*
         * Wait for more data to arrive.  We can only wait forever (as might
         * occur with read) if the local buffer is empty, as we might otherwise
         * break external synchronization strategies.  We thus only call read
         * if the local buffer is empty or data are available.
         */
        if (buf_cnt == 0 || avail > 0) {
            /* Read data. */
            if ((n_read = read(fildes, local_buf + buf_cnt, 
                               BUF_SIZE - buf_cnt)) < 0) {
                /* error condition--just return it */
                return n_read;
            }

            /*
             * If buffer started out empty, but we received something from read,
             * wait a while for more data.
             */
            if (buf_cnt == 0 && n_read > 0) {
                buf_cnt += n_read;
                usleep(500); /* normally 500000 */
                continue;
            }

            buf_cnt += n_read;
        }

        break;
    }

    /*
     * Time to return some data.  The buffer can only be empty if it started
     * out empty and read returned 0, in which case (and only in which case) we
     * should now return 0, indicating that the other side has closed the TCP
     * connection.
     */
    if (buf_cnt == 0) {
        record_results (0);
        return 0;
    }

    /*
     * Pick uniform random amount from smaller of available and requested
     * sizes, record the amount returned, and copy the data into the buffer
     * provided.
     */
    amt = (random () % (buf_cnt < nbyte ? buf_cnt : nbyte)) + 1;
    record_results(amt);
    memcpy(buf, local_buf, amt);

    /*
     * Move remaining data forward in the buffer (a cyclic buffer would be more
     * efficient). Need to be safe about copying order, so use memmove rather
     * than memcpy.
     */
    memmove (local_buf, local_buf + amt, buf_cnt - amt);
    buf_cnt -= amt;

    return amt;
}


static void
init_mp3_seed(void)
{
    unsigned int seed = DEFAULT_SEED;

    /* Initialize pseudo-random number generator. */
    srandom(seed);
}

static void *
process_input_queue(void *arg)
{
    Packet *p = NULL;

    while (1) {
        pthread_mutex_lock(&input_queue_mutex);
        if (!input_queue.empty()) {
            p = input_queue.front();
            input_queue.pop();
        }
        pthread_mutex_unlock(&input_queue_mutex);

        if (p != NULL) {
            usleep(PACKET_SWITCHING_TIME);

            pthread_mutex_lock(&output_queue_mutex);
            if (output_queue.size() < MAX_OUTPUT_QUEUE_SIZE) {
                output_queue.push(p);
            }
            pthread_mutex_unlock(&output_queue_mutex);

            p = NULL;
        }
    }

    pthread_exit((void *) 0);
}

static void *
process_output_queue(void *arg)
{
    Packet *p = NULL;

    while (1) {
        pthread_mutex_lock(&output_queue_mutex);
        if (!output_queue.empty()) {
            p = output_queue.front();
            output_queue.pop();
        }
        pthread_mutex_unlock(&output_queue_mutex);

        if (p != NULL) {
            usleep(PACKET_TRANSMISSION_TIME);

            if (p->send() == -1) {
                perror("sendto");
            }

            delete p;
            p = NULL;

            /* if (gettimeofday(&end_time, NULL) == -1) */
            /*     perror("gettimeofday");              */

            /* ++number_of_packet_forwarded;            */
        }
    }

    pthread_exit((void *) 0);
}

static void
record_results(int bytes)
{
    static int n_calls = 0; /* number of calls to mp3_read */
    static int n_bytes = 0; /* number of bytes returned    */
    FILE *results;

    n_calls++;
    n_bytes += bytes;

    if ((results = fopen (RESULTS_FILE, "w")) == NULL) {
        perror("fopen results file");
        exit(3);
    }

    fprintf(results, "%d bytes returned to %d calls.\n", n_bytes, n_calls);

    if (fclose (results) == EOF) {
        perror ("fclose results file");
        exit (3);
    }
}

