#define _POSIX_X_SOURCE 199309 // for nanosleep(2)

#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "mp1.h"


// MP2_read -- wrapper for read system call for ECE438 F10 MP2
//   fragments incoming data (from a TCP connection) to simulate a more 
//   realistic network; parameters are identical to read(2)
ssize_t 
MP1_read (int fd, void* buf, size_t count)
{
    static unsigned int mp1_seed = READ_RANDOM_SEED;
    size_t ideal;          // amount to try to read
    ssize_t one_read;	   // amount returned from read system call
    ssize_t num_read;      // amount read so far
    struct timespec delay; // pause between checks for file

    // Initialize random seed if necessary.
    if (0 == mp1_seed) {
        mp1_seed = time (NULL);
    }

    // Select number of bytes to receive.
    ideal = rand_r (&mp1_seed) % (2 * count + 1) + 4;
    if (count < ideal) {
        ideal = count;
    }

    // Try once...
    if (0 >= (one_read = read (fd, buf, ideal)) || ideal == one_read) {
        return one_read;
    }
    num_read = one_read;

    // Pause to wait for more data to arrive.
    delay.tv_sec = READ_DELAY;
    delay.tv_nsec = 0;
    (void)nanosleep (&delay, NULL);

    // Try reading one more time.  Squash errors.
    if (0 > (one_read = read (fd, buf + num_read, ideal - num_read))) {
        return num_read;
    }
    return (num_read + one_read);
}

// MP1_fwrite -- wrapper for fwrite library call for ECE438 F10 MP2
//   stalls thread when a file (named "PAUSE") is present in current 
//   working directory; parameters are identical to fwrite(3)
size_t 
MP1_fwrite (const void* ptr, size_t size, size_t nmemb, FILE* stream)
{
    struct stat trash;     // results of stat call not used
    struct timespec delay; // pause between checks for file

    while (0 == stat (WRITE_PAUSE_FILENAME, &trash)) {
        delay.tv_sec = WRITE_CHECK_PERIOD;
	delay.tv_nsec = 0;
	(void)nanosleep (&delay, NULL);
    }
    return fwrite (ptr, size, nmemb, stream);
}

