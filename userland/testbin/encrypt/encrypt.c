#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
 
#define BUF1 40
#define BUF2 42
#define BUF3 7 

static
int encrypt_test(const char *file, int encr_times, char * writebuf, const int size) {
    int fd, rv; 
    //char* readbuf = (char*)malloc(sizeof(char) * size);
	static char readbuf[41];
	//static char readbuf2[BUF2];

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

	//if i ==0 print out last 16 bytes
	//int x;  	
	//for(x=BUFF1-16;x<BUFF1;x++);
	if(i==0)
		printf ("last 16 bytes after first encrypt:\n%s\n", &(writebuf[strlen (writebuf) - 16]));

        rv = close(fd);
        if (rv<0) {
            err(1, "%s: close (1st time)", file);
        }
    }
    
    //when we break from for loop print last 16 bytes
    printf ("last 16 bytes after encrypt:\n%s\n", &(writebuf[strlen (writebuf) - 16]));

    //check if old print is equivalent to new
	//open file again read its contents and compare them
	fd = open(file, O_RDWR, 0664);

	if (fd<0) {
		err(1, "%s: open for read", file);
	}	

	//then read from file
	rv = read(fd, readbuf, 41);
	if (rv<0) {
		err(1, "%s: read", file);
	}

	//then close
	rv = close(fd);
	printf("Closing fd=%d retval=%d.\n", fd, rv);
	if (rv<0) {
		err(1, "%s: close (2nd time)", file);
	}
	//to check if they are the same as before
	printf ("\nlast 16 bytes after read, readbuf contents:\n%s\n", &(readbuf[strlen (readbuf) - 16]));

	if (strcmp(&(readbuf[strlen (readbuf) - 16]), &(writebuf[strlen (writebuf) - 16]))!=0) {
		err(1, "Buffer data mismatch!");
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
   
    //encrypt_test("testfilea1", 1, writebuf3, BUF3);

    encrypt_test("testfilea1", 1, writebuf1, BUF1);
    encrypt_test("testfilea2", 16, writebuf1, BUF1);
    encrypt_test("testfilea3", 17, writebuf1, BUF1);
    encrypt_test("testfileb1", 1, writebuf2, BUF2);
    encrypt_test("testfileb2", 16, writebuf2, BUF2);
    encrypt_test("testfileb3", 17, writebuf2, BUF2);
  
    return 0;  
}
