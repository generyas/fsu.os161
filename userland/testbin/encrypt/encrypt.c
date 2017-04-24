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

	static char readbuf[41];

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

	//if i ==0 print out last 16 bytes
	if(i==0){
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
		printf ("\nlast 16 bytes after encrypt:\n%s\n", &(readbuf[strlen (writebuf) - 16]));
		
		rv = close(fd);
        	if (rv<0) {
            		err(1, "%s: close (1st time)", file);
        	}
	
	}
    }
    

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
	//printf("Closing fd=%d retval=%d.\n", fd, rv);

	if (rv<0) {
		err(1, "%s: close (2nd time)", file);
	}
	//to check if they are the same as before

	if (strcmp(&(readbuf[strlen (readbuf) - 16]), &(writebuf[strlen (writebuf) - 16]))!=0) {
		err(1, "Buffer data mismatch!");
	}

    printf("Passed encrypting.\n");
   
    return 0;
}
 
int
main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
   
    static char writebuf1[BUF1] = "Twiddle dee dee, Twiddle dum dum.......\n";

    encrypt_test("testfilea2", 16, writebuf1, BUF1);
  
    return 0;  
}
