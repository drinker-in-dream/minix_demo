
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               system.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Toby Du, 2014/8/22
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


#include "../H/const.h"
#include "../H/type.h"
#include "../H/com.h"
#include "../H/signal.h"
#include "../H/error.h"
#include "../H/callnr.h"
#include "const.h"
#include "type.h"
#include "protect.h"
#include "proc.h"
#include "glo.h"

#define COPY_UNIT     65534L	/* max bytes to copy at once */

PRIVATE message m;

PUBLIC void inform(int proc_nr);
phys_bytes umap();

/*===========================================================================*
 *				sys_task				     *
 *===========================================================================*/
PUBLIC void sys_task()
{
/* Main entry point of sys_task.  Get the message and dispatch on type. */

	int r;
	printk(" system task \n");
	while (TRUE) {
		receive(ANY, &m);

		switch (m.m_type) {	/* which system call */
			case SYS_FORK:	r = do_fork(&m);	break;
			//case SYS_NEWMAP:	r = do_newmap(&m);	break;
			case SYS_EXEC:	r = do_exec(&m);	break;
			case SYS_XIT:	r = do_xit(&m);		break;
			case SYS_GETSP:	r = do_getsp(&m);	break;
			case SYS_TIMES:	r = do_times(&m);	break;
			case SYS_ABORT:	r = do_abort(&m);	break;
			//case SYS_SIG:	r = do_sig(&m);		break;
			case SYS_COPY:	r = do_copy(&m);	break;
			default:		r = E_BAD_FCN;
		}

		m.m_type = r;		/* 'r' reports status of call */
		send(m.m_source, &m);	/* send reply to caller */
  }
}


/*===========================================================================*
 *				do_fork					     * 
 *===========================================================================*/
PRIVATE int do_fork(message *m_ptr)
//message *m_ptr;			/* pointer to request message */
{
/* Handle sys_fork().  'k1' has forked.  The child is 'k2'. */
	struct s_proc *rpc;
	char *sptr, *dptr;	/* pointers for copying proc struct */
	int k1;			/* number of parent process */
	int k2;			/* number of child process */
	int pid;			/* process id of child */
	int bytes;			/* counter for copying proc struct */

	k1 = m_ptr->PROC1;		/* extract parent slot number from msg */
	k2 = m_ptr->PROC2;		/* extract child slot number */
	pid = m_ptr->PID;		/* extract child process id */

	if (k1 < 0 || k1 >= NR_PROCS || k2 < 0 || k2 >= NR_PROCS)return(E_BAD_PROC);
	rpc = proc_addr(k2);

	/* Copy parent 'proc' struct to child. */
	sptr = (char *) proc_addr(k1);	/* parent pointer */
	dptr = (char *) proc_addr(k2);	/* child pointer */
	bytes = sizeof(struct s_proc);		/* # bytes to copy */
	while (bytes--) *dptr++ = *sptr++;	/* copy parent struct to child */

	rpc->p_flags |= NO_MAP;	/* inhibit the process from running */
	rpc->p_pid = pid;		/* install child's pid */
	rpc->regs.eax = 0;	/* child sees pid = 0 to know it is child */

	rpc->user_time = 0;		/* set all the accounting times to 0 */
	rpc->sys_time = 0;
	rpc->child_utime = 0;
	rpc->child_stime = 0;

	return(OK);
}

/*===========================================================================*
 *				do_exec					     * 
 *===========================================================================*/
PRIVATE int do_exec(message *m_ptr)
//message *m_ptr;			/* pointer to request message */
{
	/* Handle sys_exec().  A process has done a successful EXEC. Patch it up. */
	struct s_proc *rp;
	int k;			/* which process */
	int *esp;			/* new esp */

	k = m_ptr->PROC1;		/* 'k' tells which process did EXEC */
	esp = (int *) m_ptr->STACK_PTR;
	if (k < 0 || k >= NR_PROCS) return(E_BAD_PROC);
	rp = proc_addr(k);
	rp->regs.esp	= esp;				/* set the stack pointer */
	rp->regs.eip	= (int (*)()) 0;	/* reset pc */
	rp->p_alarm = 0;		/* reset alarm timer */
	rp->p_flags &= ~RECEIVING;	/* MM does not reply to EXEC call */
	if (rp->p_flags == 0) ready(rp);
	//set_name(k, esp);		/* save command string for F1 display */
	return(OK);
}

/*===========================================================================*
 *				do_xit					     * 
 *===========================================================================*/
PRIVATE int do_xit(message *m_ptr)
//message *m_ptr;			/* pointer to request message */
{
/* Handle sys_xit().  A process has exited. */
	struct s_proc *rp, *rc;
	struct s_proc *np, *xp;
	int parent;			/* number of exiting proc's parent */
	int proc_nr;			/* number of process doing the exit */

	parent = m_ptr->PROC1;	/* slot number of parent process */
	proc_nr = m_ptr->PROC2;	/* slot number of exiting process */
	if (parent < 0 || parent >= NR_PROCS || proc_nr < 0 || proc_nr >= NR_PROCS)
		return(E_BAD_PROC);
	rp = proc_addr(parent);
	rc = proc_addr(proc_nr);
	rp->child_utime += rc->user_time + rc->child_utime;	/* accum child times */
	rp->child_stime += rc->sys_time + rc->child_stime;
	unready(rc);
	rc->p_alarm = 0;		/* turn off alarm timer */
	//set_name(proc_nr, (char *) 0);	/* disable command printing for F1 */
	
	/* If the process being terminated happens to be queued trying to send a
	 * message (i.e., the process was killed by a signal, rather than it doing an
     * EXIT), then it must be removed from the message queues.
     */

	if (rc->p_flags & SENDING) {
		/* Check all proc slots to see if the exiting process is queued. */
		for (rp = &proc[0]; rp < &proc[NR_TASKS + NR_PROCS]; rp++) {
			if (rp->p_callerq == NIL_PROC) continue;
			if(rp->p_callerq == rc){
				/* Exiting process is on front of this queue. */
				rp->p_callerq = rc->p_sendlink;
				break;
			}
			else{
				/* See if exiting process is in middle of queue. */
				np = rp->p_callerq;
				while ( ( xp = np->p_sendlink) != NIL_PROC){
					if (xp == rc) {
						np->p_sendlink = xp->p_sendlink;
						break;
					} else {
						np = xp;
					}
				}
			}
		}
	}

}

/*===========================================================================*
 *				do_times				     * 
 *===========================================================================*/
PRIVATE int do_times(message *m_ptr)
//message *m_ptr;			/* pointer to request message */
{
/* Handle sys_times().  Retrieve the accounting information. */
	struct s_proc *rp;
	int k;
	
	k = m_ptr->PROC1;		/* k tells whose times are wanted */
	if (k < 0 || k >= NR_PROCS) return(E_BAD_PROC);
	rp = proc_addr(k);

	/* Insert the four times needed by the TIMES system call in the message. */
	m_ptr->USER_TIME   = rp->user_time;
	m_ptr->SYSTEM_TIME = rp->sys_time;
	m_ptr->CHILD_UTIME = rp->child_utime;
	m_ptr->CHILD_STIME = rp->child_stime;
	return(OK);
}

/*===========================================================================*
 *				do_getsp				     * 
 *===========================================================================*/
PRIVATE int do_getsp(message *m_ptr)
//message *m_ptr;			/* pointer to request message */
{
/* Handle sys_getsp().  MM wants to know what sp is. */

	struct s_proc *rp;
	int k;				/* whose stack pointer is wanted? */

	k = m_ptr->PROC1;
	if (k < 0 || k >= NR_PROCS) return(E_BAD_PROC);
	rp = proc_addr(k);
	m.STACK_PTR = (char *) rp->regs.esp;	/* return esp here */
	return(OK);
}

/*===========================================================================*
 *				do_abort				     * 
 *===========================================================================*/
PRIVATE int do_abort(message *m_ptr)
//message *m_ptr;			/* pointer to request message */
{
/* Handle sys_abort.  MINIX is unable to continue.  Terminate operation. */

  printk("\n          Minix Abort            \n");
  return (OK);
}


/*===========================================================================*
 *				do_copy					     *
 *===========================================================================*/
PRIVATE int do_copy(message *m_ptr)
//message *m_ptr;			/* pointer to request message */
{
/* Handle sys_copy().  Copy data for MM or FS. */

	int src_proc, dst_proc, src_space, dst_space;
	vir_bytes src_vir, dst_vir;
	phys_bytes src_phys, dst_phys, bytes;

	/* Dismember the command message. */
	src_proc = m_ptr->SRC_PROC_NR;
	dst_proc = m_ptr->DST_PROC_NR;
	src_space = m_ptr->SRC_SPACE;
	dst_space = m_ptr->DST_SPACE;
	src_vir = (vir_bytes) m_ptr->SRC_BUFFER;
	dst_vir = (vir_bytes) m_ptr->DST_BUFFER;
	bytes = (phys_bytes) m_ptr->COPY_BYTES;

	/* Compute the source and destination addresses and do the copy. */
	if (src_proc == ABS)
		src_phys = (phys_bytes) m_ptr->SRC_BUFFER;
	 else
		src_phys = umap(src_vir,(vir_bytes)bytes);

	if (dst_proc == ABS)
		dst_phys = (phys_bytes) m_ptr->DST_BUFFER;
	else
		dst_phys = umap(dst_vir,(vir_bytes)bytes);

	if (src_phys == 0 || dst_phys == 0) return(EFAULT);
		phys_copy(src_phys, dst_phys, bytes);
	return(OK);
}

/*===========================================================================*
 *				cause_sig				     * 
 *===========================================================================*/
PUBLIC void cause_sig(int proc_nr, int sig_nr)
//int proc_nr;			/* process to be signalled */
//int sig_nr;			/* signal to be sent in range 1 - 16 */
{
/* A task wants to send a signal to a process.   Examples of such tasks are:
 *   TTY wanting to cause SIGINT upon getting a DEL
 *   CLOCK wanting to cause SIGALRM when timer expires
 * Signals are handled by sending a message to MM.  The tasks don't dare do
 * that directly, for fear of what would happen if MM were busy.  Instead they
 * call cause_sig, which sets bits in p_pending, and then carefully checks to
 * see if MM is free.  If so, a message is sent to it.  If not, when it becomes
 * free, a message is sent.  The calling task always gets control back from 
 * cause_sig() immediately.
 */

  struct s_proc *rp;

  rp = proc_addr(proc_nr);
  if (rp->p_pending == 0) sig_procs++;	/* incr if a new proc is now pending */
  rp->p_pending |= 1 << (sig_nr - 1);
  inform(MM_PROC_NR);		/* see if MM is free */
}


/*===========================================================================*
 *				inform					     * 
 *===========================================================================*/
PUBLIC void inform(int proc_nr)
//int proc_nr;			/* MM_PROC_NR or FS_PROC_NR */
{
/* When a signal is detected by the kernel (e.g., DEL), or generated by a task
 * (e.g. clock task for SIGALRM), cause_sig() is called to set a bit in the
 * p_pending field of the process to signal.  Then inform() is called to see
 * if MM is idle and can be told about it.  Whenever MM blocks, a check is
 * made to see if 'sig_procs' is nonzero; if so, inform() is called.
 */

  struct s_proc *rp, *mmp;

  /* If MM is not waiting for new input, forget it. */
  mmp = proc_addr(proc_nr);
  if ( ((mmp->p_flags & RECEIVING) == 0) || mmp->p_getfrom != ANY) return;

  /* MM is waiting for new input.  Find a process with pending signals. */
  for (rp = proc_addr(0); rp < proc_addr(NR_PROCS); rp++)
	if (rp->p_pending != 0) {
		m.m_type = KSIG;
		m.PROC1 = rp - proc - NR_TASKS;
		m.SIG_MAP = rp->p_pending;
		sig_procs--;
		if (mini_send(HARDWARE, proc_nr, &m) != OK) 
			; //panic("can't inform MM", NO_NUM);
		rp->p_pending = 0;	/* the ball is now in MM's court */
		return;
	}
}


/*===========================================================================*
 *				umap					     * 
 *===========================================================================*/
PUBLIC phys_bytes umap(vir_bytes vir_addr, vir_bytes bytes)			
//vir_bytes vir_addr;		/* virtual address in bytes within the seg */
//vir_bytes bytes;		/* # of bytes to be copied */
{
	return (vir_addr + bytes);
}

