#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
 
#define BUF1 40
#define BUF2 42
#define BUF3 7 

static
int encrypt_test(const char *file, int encr_times, char * writebuf, const int size) {
    int fd, rv;
 
    fd = open(file, O_RDWR|O_CREAT|O_TRUNC, 0664);
 
    if (fd<0) {
        err(1, "%s: open for write", file);
    }
 
    rv = write(fd, writebuf, size);
    if (rv<0) {
        err(1, "%s: write", file);
    }
 
    rv = close(fd);
    if (rv<0) {
        err(1, "%s: close (1st time)", file);
    }
   
    int i;
    for (i = 0; i < encr_times; ++i)
    {
        fd = open(file, O_RDWR, 0664);
 
        if (fd<0) {
            err(1, "%s: open for read", file);
        }
 
        rv = encrypt(fd);
 
        rv = close(fd);
        if (rv<0) {
            err(1, "%s: close (1st time)", file);
        }
    }
   
    printf("Passed encrypting.\n");
   
    return 0;
}
 
int
main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;
   
    static char writebuf1[BUF1] = "Twiddle dee dee, Twiddle dum dum.......\n";
    static char writebuf2[BUF2] = "Twiddle dee dee, Twiddle dum dum.......zf\n";
    //static char writebuf3[BUF3] = "Twiddl\n";
   
    //static char readbuf[41];
    //encrypt_test("testfilea1", 1, writebuf3, BUF3);

    encrypt_test("testfilea1", 1, writebuf1, BUF1);
    encrypt_test("testfilea2", 16, writebuf1, BUF1);
    encrypt_test("testfilea3", 17, writebuf1, BUF1);
    encrypt_test("testfileb1", 1, writebuf2, BUF2);
    encrypt_test("testfileb2", 16, writebuf2, BUF2);
    encrypt_test("testfileb3", 17, writebuf2, BUF2);
  
    return 0;  
}
