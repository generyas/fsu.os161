#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

int
main(int argc, char *argv[])
{
	//printf("asjdnakjsdnakjdn");
	static char writebuf[40] = "Twiddle dee dee, Twiddle dum dum.......\n";
	//static char readbuf[41];

	const char *file;
	int fd, rv;

	if (argc == 0) {
		/*warnx("No arguments - running on \"testfile\"");*/
		file = "testfile";
	}
	else if (argc == 2) {
		file = argv[1];
	}
	else {
		errx(1, "Usage: filetest <filename>");
	}

	fd = open(file, O_RDWR|O_CREAT|O_TRUNC, 0664);

	if (fd<0) {
		err(1, "%s: open for write", file);
	}


	rv = write(fd, writebuf, 40);
	if (rv<0) {
		err(1, "%s: write", file);
	}

	rv = close(fd);
	if (rv<0) {
		err(1, "%s: close (1st time)", file);
	}

	fd = open(file, O_RDWR, 0664);

	if (fd<0) {
		err(1, "%s: open for read", file);
	}
	
	rv = encrypt(fd);
	
	

/*
	rv = read(fd, readbuf, 40);
	if (rv<0) {
		err(1, "%s: read", file);
	}

	rv = close(fd);
	printf("Closing fd=%d retval=%d.\n", fd, rv);

	if (rv<0) {
		err(1, "%s: close (2nd time)", file);
	}

	// ensure null termination 
	readbuf[40] = 0;

	if (strcmp(readbuf, writebuf)) {
		errx(1, "Buffer data mismatch!");
	}
*/
/*	rv = remove(file);
	if (rv<0) {
		err(1, "%s: remove", file);
	} */
	printf("Passed encrypting.\n");
	return 0;
}
