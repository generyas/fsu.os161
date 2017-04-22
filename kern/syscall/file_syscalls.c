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
	size_t len = strlen((char*)upath) + 1;

	actual = (size_t *)kmalloc(sizeof(size_t));
	kpath = (char *)kmalloc(sizeof(char) * (len));
	
	if ( (err = copyinstr(upath, kpath, len, actual)) ){
		*retval = -1;
		return err;
	}

	// 3. Open the file
	
	file = (struct openfile *)kmalloc(sizeof(struct openfile));
	
	if ( (err = openfile_open(kpath, flags, mode, &file)) ){
		*retval = -1;
		return err;
	}

	openfile_incref(file);
	
	// 4. Place the file in curproc's file table

	int * fd_ret = NULL;
	
	fd_ret = (int *)kmalloc(sizeof(int));
	
	if ( (err = filetable_place(curproc->p_filetable, file, fd_ret)) ){
		*retval = -1;
		return err;
	}
	
	*retval = *fd_ret;
	
	kfree(kpath);

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

	if (file->of_accmode == O_WRONLY){
		*retval = -1;
		return EACCES;	
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
	
    //DEBUG(DB_SYSCALL,"Offsets R Pre: %d %d\n", (int)file->of_offset, (int)u.uio_offset);
    
	file->of_offset = u.uio_offset;
    
    //DEBUG(DB_SYSCALL,"Offsets R Post: %d %d\n", (int)file->of_offset, (int)u.uio_offset);

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

	if (file->of_accmode == O_RDONLY){
		*retval = -1;
		return EACCES;	
	}

	// cons up a uio    
    
	struct iovec iov;
	struct uio u;
	uio_kinit(&iov, &u, buf, nbytes, file->of_offset, UIO_WRITE);
    
    VOP_WRITE(file->of_vnode, &u);
    
	// update the seek position afterwards
    //DEBUG(DB_SYSCALL,"Offsets W Pre: %d %d\n", (int)file->of_offset, (int)u.uio_offset);
    
	file->of_offset = u.uio_offset;
    
    //DEBUG(DB_SYSCALL,"Offsets W Post: %d %d\n", (int)file->of_offset, (int)u.uio_offset);

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
	struct openfile * file;

	// 1. validate the fd number (use filetable_okfd)

	if( !filetable_okfd(curproc->p_filetable, fd) ){
		*retval = -1;
  		return EBADF;
	}
	// 2. use filetable_placeat to replace curproc's file table... entry with NULL.

	filetable_placeat(curproc->p_filetable, NULL, fd, &file);

	// 3. check if the previous entry in the file table was also NULL... (this means no such file was open)

	if(file == NULL){
 		*retval = -1;
  		return EFAULT;	
	}

	// 4. decref the open file returned by filetable_placeat

	openfile_decref(file);

    *retval = 0;
    return result;
}


//Not the right way to seek!!!
static int 
sys_seek(int fd, off_t offset, int *retval)
{  
	struct openfile * file;
    (void) offset;

	if ( (err = filetable_get(curproc->p_filetable, fd, &file)) ){
		*retval = err;
		return -1;
	}
    
	if (VOP_ISSEEKABLE(file->of_vnode)) 
    {
		lock_acquire(file->of_offsetlock);
        
        //DEBUG(DB_SYSCALL,"Am seeking %d to %d\n", (int)file->of_offset, (int)offset);
        
        file->of_offset = file->of_offset + offset;
        
        //DEBUG(DB_SYSCALL,"Have seeked %d\n", (int)file->of_offset);
        
        lock_release(file->of_offsetlock);
        
        filetable_put(curproc->p_filetable, fd, file);
        return 0;
    }
    
    return -1;
}


#define BUF_LEN 4

int
sys_encrypt(int fd, int *retval)
{    
    unsigned char buf[BUF_LEN];
    unsigned int buf_shift = 0;
   
    while (true)
    {
        buf[0] = 0;
        buf[1] = 0;
        buf[2] = 0;
        buf[3] = 0;
       
        //DEBUG(DB_SYSCALL,"T1\n");
 
        if ((err = sys_read(fd, (userptr_t) buf, BUF_LEN, retval)))
        {
            *retval = -1;
            return err;
        }
        if (buf[0] == 0)
            break;
        
        //DEBUG(DB_SYSCALL,"read _ret: %d\n", *retval);
 
        DEBUG(DB_SYSCALL,"BUF  PRE: %d %d %d %d\n", buf[0], buf[1], buf[2], buf[3]);
 
        buf_shift = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
 
        //DEBUG(DB_SYSCALL,"SHIFT PRE: %d\n", buf_shift);
 
        buf_shift = (buf_shift >> 10) | (buf_shift << 22);
 
        buf[3] = (buf_shift & 0xFF000000) >> 24;
        buf[0] = (buf_shift & 0x00FF0000) >> 16;
        buf[1] = (buf_shift & 0x0000FF00) >> 8;
        buf[2] = (buf_shift & 0x000000FF);
 
        //DEBUG(DB_SYSCALL,"SHIFT POST: %d\n", buf_shift);
        
        sys_seek(fd, -BUF_LEN, retval);
 
        if ((err = sys_write(fd, (userptr_t) buf, BUF_LEN, retval)))
        {
            *retval = -1;
            return err;
        }
	    //DEBUG(DB_SYSCALL,"write _ret: %d\n", *retval);
        
        DEBUG(DB_SYSCALL,"BUF POST: %d %d %d %d\n", buf[0], buf[1], buf[2], buf[3]);
 
        //DEBUG(DB_SYSCALL,"T2\n");
    }
   
    return 0;
}
