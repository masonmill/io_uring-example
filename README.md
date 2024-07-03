# io_uring-example

This repository contains `io_uring-example.c`, a simple program demonstrating
the use of io_uring to asynchronously read a file on Linux.

In this program, we leverage the liburing library to setup and teardown the 
io_uring instance, utilizing its simplified API to prepare, submit, and 
complete I/O requests.

## liburing

The liburing library was originally written by Jens Axboe, and is maintained
at the link below.

https://github.com/axboe/liburing

## Compile

Before compiling this example, you must build and install liburing. For more
information, please visit the liburing GitHub.

```
$ gcc -Wall -O2 -D_GNU_SOURCE -o io_uring-example io_uring-example.c -luring
```

## Run the example

In this example, `input` is the file we want to read from. Pass `input` as
the program's first argument.

```
$ ./io_uring-example input
```

## Additional info

This project was developed on Debian Bookworm with Linux kernel version 6.9.3
and Liburing version 2.6.
