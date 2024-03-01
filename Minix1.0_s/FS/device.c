
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               device.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Toby Du, 2015/2/10
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "../h/const.h"
#include "../h/type.h"
#include "../h/com.h"
#include "../h/error.h"
#include "const.h"
#include "type.h"
#include "dev.h"
#include "file.h"
#include "fproc.h"
#include "glo.h"
#include "inode.h"
#include "param.h"

PRIVATE message dev_mess;
PRIVATE major, minor, task;
extern max_major;

/*===========================================================================*
 *				find_dev				     *
 *===========================================================================*/
PRIVATE void find_dev(dev_nr dev)
//dev_nr dev;			/* device */
{
/* Extract the major and minor device number from the parameter. */

  major = (dev >> MAJOR) & BYTE;	/* major device number */
  minor = (dev >> MINOR) & BYTE;	/* minor device number */
  if (major == 0 || major >= max_major) printk("bad major dev");
  task = dmap[major].dmap_task;	/* which task services the device */
  dev_mess.DEVICE = minor;
}



/*===========================================================================*
 *				dev_open				     *
 *===========================================================================*/
PUBLIC int dev_open(dev_nr dev, int mod)
//dev_nr dev;			/* which device to open */
//int mod;			/* how to open it */
{
/* Special files may need special processing upon open. */

  find_dev(dev);
  (*dmap[major].dmap_open)(task, &dev_mess);
  return(dev_mess.REP_STATUS);
}


/*===========================================================================*
 *				dev_close				     *
 *===========================================================================*/
PUBLIC void dev_close(dev_nr dev)
//dev_nr dev;			/* which device to close */
{
/* This procedure can be used when a special file needs to be closed. */

  find_dev(dev);
  (*dmap[major].dmap_close)(task, &dev_mess);
}


/*===========================================================================*
 *				rw_dev					     *
 *===========================================================================*/
PUBLIC void rw_dev(int task_nr, message *mess_ptr)
//int task_nr;			/* which task to call */
//message *mess_ptr;		/* pointer to message for task */
{
/* All file system I/O ultimately comes down to I/O on major/minor device
 * pairs.  These lead to calls on the following routines via the dmap table.
 */
	
	int proc_nr;
	
	proc_nr = mess_ptr->PROC_NR;
	if (sendrec(task_nr, mess_ptr) != OK) printk("rw_dev: can't send");
	while (mess_ptr->REP_PROC_NR != proc_nr) {
		/* Instead of the reply to this request, we got a message for an
		 * earlier request.  Handle it and go receive again.
		 */
		revive(mess_ptr->REP_PROC_NR, mess_ptr->REP_STATUS);
		receive(task_nr, mess_ptr);
	}

}

/*===========================================================================*
 *				rw_dev2					     *
 *===========================================================================*/
PUBLIC void rw_dev2(int dummy, message *mess_ptr)
//int dummy;			/* not used - for compatibility with rw_dev() */
//message *mess_ptr;		/* pointer to message for task */
{
/* This routine is only called for one device, namely /dev/tty.  It's job
 * is to change the message to use the controlling terminal, instead of the
 * major/minor pair for /dev/tty itself.
 */
 
	int task_nr, major_device;
	

	major_device = (fp->fs_tty >> MAJOR) & BYTE;
	task_nr = dmap[major_device].dmap_task;	/* task for controlling tty */
	mess_ptr->DEVICE = (fp->fs_tty >> MINOR) & BYTE;
	rw_dev(task_nr, mess_ptr);
}

/*===========================================================================*
 *				no_call					     *
 *===========================================================================*/
PUBLIC int no_call(int task_nr, message *m_ptr)
//int task_nr;			/* which task */
//message *m_ptr;			/* message pointer */
{
/* Null operation always succeeds. */

  m_ptr->REP_STATUS = OK;
}

/*===========================================================================*
 *				do_ioctl				     *
 *===========================================================================*/
PUBLIC int do_ioctl()
{
/* Perform the ioctl(ls_fd, request, argx) system call (uses m2 fmt). */
	struct filp *f;
    struct inode *rip;
	extern struct filp *get_filp();
	if ( (f = get_filp(ls_fd)) == NIL_FILP) return(err_code);
	rip = f->filp_ino;		/* get inode pointer */
	if ( (rip->i_mode & I_TYPE) != I_CHAR_SPECIAL) return(ENOTTY);
	find_dev(rip->i_zone[0]);

	dev_mess.m_type  = TTY_IOCTL;
	dev_mess.PROC_NR = who;
	dev_mess.TTY_LINE = minor;	
	dev_mess.TTY_REQUEST = m.TTY_REQUEST;
	dev_mess.TTY_SPEK = m.TTY_SPEK;
	dev_mess.TTY_FLAGS = m.TTY_FLAGS;

	/* Call the task. */
	(*dmap[major].dmap_rw)(task, &dev_mess);

	/* Task has completed.  See if call completed. */
	if (dev_mess.m_type == SUSPEND) suspend(task);  /* User must be suspended. */
	m1.TTY_SPEK = dev_mess.TTY_SPEK;	/* erase and kill */
	m1.TTY_FLAGS = dev_mess.TTY_FLAGS;	/* flags */
	return(dev_mess.REP_STATUS);
}


/*===========================================================================*
 *				dev_io					     *
 *===========================================================================*/
PUBLIC int dev_io(int rw_flag, dev_nr dev, long pos, int bytes, int proc, char *buff)
//int rw_flag;			/* READING or WRITING */
//dev_nr dev;			/* major-minor device number */
//long pos;			/* byte position */
//int bytes;			/* how many bytes to transfer */
//int proc;			/* in whose address space is buff? */
//char *buff;			/* virtual address of the buffer */
{
/* Read or write from a device.  The parameter 'dev' tells which one. */

  //find_dev(dev);
	printk("devio called\n");
  /* Set up the message passed to task. */
  dev_mess.m_type   = (rw_flag == READING ? DISK_READ : DISK_WRITE);
  dev_mess.DEVICE   = (dev >> MINOR) & BYTE;
  dev_mess.POSITION = pos;
  dev_mess.PROC_NR  = proc;
  dev_mess.ADDRESS  = buff;
  dev_mess.COUNT    = bytes;
  

  printk("dev_mess.PROC_NR %d\n",dev_mess.PROC_NR);

  major=3;
  task = dmap[major].dmap_task;	/* which task services the device */
  printk("task is %d\n",task);
  /* Call the task. */
  (*dmap[major].dmap_rw)(task, &dev_mess);

  /* Task has completed.  See if call completed. */
  if (dev_mess.REP_STATUS == SUSPEND) suspend(task);	/* suspend user */

  return(dev_mess.REP_STATUS);
}

