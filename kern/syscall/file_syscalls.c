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
	if (allflags != (allflags | flags) ){
		errno = EINVAL;
		return -1;
	}
	
	// 2. Copy in supplied file name
	size_t *actual = NULL;
	size_t len = strlen((char*)upath);
	
	//int ret = copyinstr(upath, kpath, len, actual);
	
	if (copyinstr(upath, kpath, len, actual) == EFAULT){
		errno = EFAULT;
		return -1;
	}
	// 3. Open the file

	int err = openfile_open(dest, flags, mode, &file);
	
	
	//Possible alternative is to check if err != 0 and return err. Need to discuss with group
		
	if (err == ENOMEM  ||  // ENOMEM: openfile_open -> ENOMEM
	    err == ENODEV  ||  // ENODEV: vfs_open -> vfs_lookparent -> getdevice -> vfs_getroot -> ENODEV
            err == ENOTDIR ||  // We assume this error gets check here
	    err == ENOENT  ||  // ENOENT: vfs_open -> vfs_lookparent -> getdevice -> ENOENT
	    err == EISDIR  ||  // We assume this error gets check here
	    err == ENFILE  ||  // We assume this error gets check here
	    err == ENXIO   ||  // ENXIO: vfs_open -> vfs_lookparent -> getdevice -> vfs_getroot -> ENXIO
	    err == ENOSPC  ||  // We assume this error gets check here
	    err == EIO)        // We assume this error gets check here
)	{   
		errno = err;
		return -1;
	}


	openfile_incref(file);
	
	// 4. Place the file in curproc's file table
	
	//int ret_f = filetable_place(curproc->p_filetable, file, fd_ret);

	int * fd_ret = NULL;
	
	if (filetable_place(curproc->p_filetable, file, fd_ret) == EMFILE){
		errno = EMFILE;
		return -1;
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

	if(EBADF == filetable_get(curproc->p_filetable, fd, &file)){
		errno = EBADF;
		return -1;
	}

	// 2. lock the seek position in the open file
	
	int seekable = lseek(fd, 0, SEEK_CUR);
	if (seekable != -1) // Check if seekable
		spinlock_acquire(&file->of_offsetlock);

	// 3. check for files opened write-only 
	if (file->of_accmode != O_WRONLY){
		errno = EBADF;
		return -1;	
	}

	// 4. cons up a uio
	struct iovec iov;
	struct uio u;	
	
	iov.iov_ubase = buf;         // here is where we use buff 
	iov.iov_len = size;          // length that we will read
	u.uio_iov = &iov;
	u.uio_iovcnt = 1;
	u.uio_resid = size;          // amount to read from the file
	u.uio_offset = file->of_offset;
	u.uio_segflg = UIO_USERSPACE;
	u.uio_rw = UIO_READ;
	u.uio_space = curproc->p_addrspace;

	
	// 5. call VOP_READ	
	int vop_ret = VOP_READ(file->of_vnode, &u);
	
	result = u.uio_offset // setting result to amount of bits read
 
	// 6. update the seek position afterwards
	
	int success = lseek(fd, size, SEEK_CUR);

	// 7. unlock and filetable_put()
	if (seekable != -1)
		spinlock_release(&file->of_offsetlock);
	
	filetable_put(curproc->p_filetable, fd, file);
 

       return result;
}

/*
 * write() - write data to a file
 */

/*
 * close() - remove from the file table.
 */
int
sys_close(struct filetable *ft, int fd)
{
	int result = 0;
	struct openfile * file;

	//1. validate the fd number (use filetable_okfd)
	if(!filetable_okfd(ft, fd))
	{
		errno = EBADF;
		return(-1);
	}
	//2. use filetable_placeat to replace curproc's file table 
	//	entry with NULL.
	filetable_placeat(ft, NULL, fd, file);

	//3. check if the previous entry in the file table was also NULL
	//	(this means no such file was open)
	if((filetable_get(ft, fd, &file))==EBADF)
	{
		errno = EBADF;
		return(-1);
	}
	
	//4. decref the open file returned by filetable_placeat
	openfile_decref(file);
}

/* 
* encrypt() - read and encrypt the data of a file
*/
