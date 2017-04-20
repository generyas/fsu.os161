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

//TEMP
static int err;

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
	
	if (allflags != (allflags | flags) ){
		*retval = -1;
		return EINVAL;
	}
	
	// 2. Copy in supplied file name

	size_t *actual = NULL;
	size_t len = strlen((char*)upath);
	
	if ( (err = copyinstr(upath, kpath, len, actual)) ){
		*retval = -1;
		return err;
	}

	// 3. Open the file

	int err = openfile_open(kpath, flags, mode, &file);
	
	if (err){   
		*retval = -1;
		return err;
	}

	openfile_incref(file);
	
	// 4. Place the file in curproc's file table

	int * fd_ret = NULL;
	
	if ( (err = filetable_place(curproc->p_filetable, file, fd_ret)) ){
		*retval = -1;
		return err;
	}
	
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

	// 1. Translate the file descriptor number to an open file object

	struct openfile * file;

	if( (err = filetable_get(curproc->p_filetable, fd, &file)) ){
		*retval = -1;
		return err;
	}

	// 2. lock the seek position in the open file
	
	int seekable = VOP_ISSEEKABLE(file->of_vnode);
    
	if (seekable) // Check if seekable
		lock_acquire(file->of_offsetlock);

	// 3. check for files opened write-only

	if (file->of_accmode != O_WRONLY){
		*retval = -1;
		return EBADF;	
	}

	// 4. cons up a uio

	struct iovec iov;
	struct uio u;
	
	uio_kinit(&iov, &u, buf, size, file->of_offset, UIO_READ);
	
	// 5. call VOP_READ
	
	if( (err = VOP_READ(file->of_vnode, &u)) ){
		*retval = -1;
		return err;
	}
	
	*retval = u.uio_offset; // setting result to amount of bits read

	// 6. update the seek position afterwards
	
	//int success = lseek(fd, size, SEEK_CUR);

	// 7. unlock and filetable_put()

	if (seekable)
		lock_release(file->of_offsetlock);
	
	filetable_put(curproc->p_filetable, fd, file);
 
    return result;
}

/*
 * write() - write data to a file
 */

int
sys_write(int fd, userptr_t buf, size_t nbytes, int *retval)
{
	// translate the file descriptor number to an open file object
	//  (use filetable_get)
	struct openfile* file;

	if ( (err = filetable_get(curproc->p_filetable, fd, &file)) ){
		*retval = err;
		return -1;
	}
    
	// lock the seek position in the open file (but only for seekable
	//  objects)

	int can_seek = VOP_ISSEEKABLE(file->of_vnode);
    
	if (can_seek)
		lock_acquire(file->of_offsetlock);
    
	// check for files opened read-only

	if (file->of_accmode != O_RDONLY){
		*retval = -1;
		return EBADF;	
	}

	// cons up a uio    
    
	struct iovec iov;
	struct uio u;
	uio_kinit(&iov, &u, buf, nbytes, file->of_offset, UIO_WRITE);
    
    	VOP_WRITE(file->of_vnode, &u);
    
	// update the seek position afterwards
    
	/*if (can_seek != -1) {
		if ((errno = lseek(fd, nbytes, SEEK_CUR)))
			return -1;
	}*/
    
	// unlock and filetable_put()
	if (can_seek)
		lock_release(file->of_offsetlock);
	
	filetable_put(curproc->p_filetable, fd, file);
    
	// set the return value correctly

    *retval = u.uio_offset;
    return 0;
}


/*
 * close() - remove from the file table.
 */
int
sys_close(int fd, int *retval)
{
	int result = 0;	
	retval = NULL;
	struct openfile * file;

	// 1. validate the fd number (use filetable_okfd)

	if( (err = filetable_okfd(curproc->p_filetable, fd)) )
	{
		*retval = -1;
		return err;
	}
	// 2. use filetable_placeat to replace curproc's file table... entry with NULL.

	filetable_placeat(curproc->p_filetable, NULL, fd, &file);

	// 3. check if the previous entry in the file table was also NULL... (this means no such file was open)

	if( (err = filetable_get(curproc->p_filetable, fd, &file)) )
	{
		*retval = -1;
		return err;
	}
	
	// 4. decref the open file returned by filetable_placeat

	openfile_decref(file);

    *retval = 0;
    return result;
}

/* 
* encrypt() - read and encrypt the data of a file
*/
