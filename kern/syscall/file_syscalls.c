/*
 * File-related system call implementations.
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/limits.h>
#include <kern/seek.h>
#include <kern/stat.h>
#include <lib.h>
#include <uio.h>
#include <proc.h>
#include <current.h>
#include <synch.h>
#include <copyinout.h>
#include <vfs.h>
#include <vnode.h>
#include <openfile.h>
#include <filetable.h>
#include <syscall.h>

/*
 * open() - get the path with copyinstr, then use openfile_open and
 * filetable_place to do the real work.
 */
int
sys_open(const_userptr_t upath, int flags, mode_t mode, int *retval)
{
	const int allflags = O_ACCMODE | O_CREAT | O_EXCL | O_TRUNC | O_APPEND | O_NOCTTY;

	char *kpath = NULL;
	struct openfile *file = NULL;
	int result = 0;

	// 1. Check for invalid flags
	if (allflags != (allflags | flags) )
		return EINVAL;
	
	// 2. Copy in supplied file name
	size_t *actual = NULL;
	size_t len = strlen((char*)upath);
	
	//int ret = copyinstr(upath, kpath, len, actual);
	
	if (copyinstr(upath, kpath, len, actual) == EFAULT)
		return EFAULT;
	
	// 3. Open the file

	//int err = openfile_open(dest, flags, mode, &file);
	
	if (openfile_open(kpath, flags, mode, &file) == ENOMEM)
		return ENOMEM;

	openfile_incref(file);
	
	int * fd_ret = NULL;
	
	// 4. Place the file in curproc's file table
	
	//int ret_f = filetable_place(curproc->p_filetable, file, fd_ret);
	
	if (filetable_place(curproc->p_filetable, file, fd_ret) == EMFILE)
		return EMFILE;
	
	*retval = *fd_ret;

	return result;
}

/*
 * read() - read data from a file
 */
int
sys_read(int fd, userptr_t buf, size_t size, int *retval)
{
       int result = 0;

       /* 
        * Your implementation of system call read starts here.  
        *
        * Check the design document design/filesyscall.txt for the steps
        */
       (void) fd; // suppress compilation warning until code gets written
       (void) buf; // suppress compilation warning until code gets written
       (void) size; // suppress compilation warning until code gets written
       (void) retval; // suppress compilation warning until code gets written

       return result;
}

/*
 * write() - write data to a file
 */

/*
 * close() - remove from the file table.
 */

/* 
* encrypt() - read and encrypt the data of a file
*/
