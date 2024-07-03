/*
 *  This program is a simple demonstration of how
 *  to use io_uring via the liburing library to
 *  read a file asynchronously on Linux.
 *
 *  The liburing library was originally written by
 *  Jens Axboe, and is maintained at the link below.
 *
 *  https://github.com/axboe/liburing
 *
 *  Compile:
 *      gcc -Wall -O2 -D_GNU_SOURCE -o io_uring-example io_uring-example.c -luring
 *  Run:
 *      io_uring-example <input>
 */

#include <stdio.h>          // printf, fprintf, perror
#include <fcntl.h>          // open, O_RDONLY, O_DIRECT
#include <string.h>         // strerror
#include <stdlib.h>         // calloc, posix_memalign
#include <sys/types.h>      // ssize_t, off_t
#include <sys/stat.h>       // fstat, struct stat
#include <unistd.h>         // close

#include "liburing.h"

#define QUEUE_DEPTH 8

int main(int argc, char *argv[]) {
    struct io_uring ring;
    int i, fd, res, pending, done;
    struct io_uring_sqe *sqe;
    struct io_uring_cqe *cqe;
    struct iovec *iovecs;
    struct stat sb;
    ssize_t fsize;
    off_t offset;
    void *buf;

    // Argument checking
    if (argc < 2) {
        printf("%s: file\n", argv[0]);
        return 1;
    }

    // Initialize io_uring
    res = io_uring_queue_init(QUEUE_DEPTH, &ring, 0);
    if (res < 0) {
        fprintf(stderr, "queue_init: %s\n", strerror(-res));
        return 1;
    }

    // Open input file in read-only mode
    fd = open(argv[1], O_RDONLY | O_DIRECT);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    // Get file status
    if (fstat(fd, &sb) < 0) {
        perror("fstat");
        return 1;
    }

    // Allocate I/O buffers
    fsize = 0;
    iovecs = calloc(QUEUE_DEPTH, sizeof(struct iovec));
    for (i = 0; i < QUEUE_DEPTH; i++) {
        if (posix_memalign(&buf, 4096, 4096)) {
            return 1;
        }
        iovecs[i].iov_base = buf;
        iovecs[i].iov_len = 4096;
        fsize += 4096;
    }

    // Prepare read requests
    offset = 0;
    i = 0;
    do {
        sqe = io_uring_get_sqe(&ring);
        if (!sqe)
            break;
        io_uring_prep_readv(sqe, fd, &iovecs[i], 1, offset);
        offset += iovecs[i].iov_len;
        i++;

        // Break if end of file is reached
        if (offset >= sb.st_size)
            break;
    } while (1);

    // Submit requests
    res = io_uring_submit(&ring);
    if (res < 0) {
        fprintf(stderr, "io_uring_submit: %s\n", strerror(-res));
        return 1;
    } 
    else if (res != i) {
        fprintf(stderr, "io_uring_submit submitted less %d\n", res);
        return 1;
    }

    // Complete requests
    done = 0;
    pending = res;
    fsize = 0;
    for (i = 0; i < pending; i++) {
        // Wait for a completion queue entry
        res = io_uring_wait_cqe(&ring, &cqe);
        if (res < 0) {
            fprintf(stderr, "io_uring_wait_cqe: %s\n", strerror(-res));
            return 1;
        }

        done++;

        // Ensure result matches expected size
        res = 0;
        if (cqe->res != 4096 && cqe->res + fsize != sb.st_size) {
            fprintf(stderr, "ret=%d, wanted 4096\n", cqe->res);
            res = 1;
        }

        // Accumulate size read and mark the entry as seen
        fsize += cqe->res;
        io_uring_cqe_seen(&ring, cqe);

        if (res)
            break;
    }

    // Output summary
    printf("Submitted=%d, completed=%d, bytes=%lu\n", pending, done,
                        (unsigned long) fsize);
    
    // Clean up
    close(fd);
    io_uring_queue_exit(&ring);

    return 0;
}
