#if !defined(_ECE438_MP1_H)
#define _ECE438_MP1_H

#include <stdio.h>
#include <sys/types.h>

#ifdef  __cplusplus
extern "C" {
#endif

// MP1_read control parameters
#define READ_RANDOM_SEED    1231411  // random seed; set to 0 for non-determ.
#define READ_DELAY          1        // delay in sec to wait for more data

// MP1_read -- wrapper for read system call for ECE438 F10 MP1
//   fragments incoming data (from a TCP connection) to simulate a more 
//   realistic network; parameters are identical to read(2)
extern ssize_t MP1_read (int fd, void* buf, size_t count);

// MP1_write control parameters
#define WRITE_PAUSE_FILENAME "PAUSE" // filename used to pause writes
#define WRITE_CHECK_PERIOD   1       // delay in sec between checks for file

// MP1_fwrite -- wrapper for fwrite library call for ECE438 F10 MP1
//   stalls thread when a file (named "PAUSE") is present in current 
//   working directory; parameters are identical to fwrite(3)
extern size_t MP1_fwrite (const void* ptr, size_t size, size_t nmemb, 
			  FILE* stream);

#ifdef  __cplusplus
}
#endif

#endif // _ECE438_MP1_H
