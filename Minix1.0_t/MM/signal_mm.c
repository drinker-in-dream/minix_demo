/* This file handles signals, which are asynchronous events and are generally
 * a messy and unpleasant business.  Signals can be generated by the KILL
 * system call, or from the keyboard (SIGINT) or from the clock (SIGALRM).
 * In all cases control eventually passes to check_sig() to see which processes
 * can be signalled.  The actual signalling is done by sig_proc().
 *
 * The entry points into this file are:
 *   do_signal:	perform the SIGNAL system call
 *   do_kill:	perform the KILL system call
 *   do_ksig:	accept a signal originating in the kernel (e.g., SIGINT)
 *   sig_proc:	interrupt or terminate a signalled process
 *   do_alarm:	perform the ALARM system call by calling set_alarm()
 *   set_alarm:	tell the clock task to start or stop a timer
 *   do_pause:	perform the PAUSE system call
 *   unpause:	check to see if process is suspended on anything
 */

#include "../H/const.h"
#include "../H/type.h"
#include "../H/callnr.h"
#include "../H/com.h"
#include "../H/error.h"
#include "../H/signal.h"
#include "../H/stat.h"
#include "const.h"
#include "glo.h"
#include "mproc.h"
#include "param.h"

#define DUMP_SIZE	 256	/* buffer size for core dumps */
#define CORE_MODE	0777	/* mode to use on core image files */
#define DUMPED          0200	/* bit set in status when core dumped */

PRIVATE message m_sig;


/*===========================================================================*
 *				do_signal				     *
 *===========================================================================*/
PUBLIC int do_signal()
{
/* Perform the signal(sig, func) call by setting bits to indicate that a signal
 * is to be caught or ignored.
 */

  int mask;

  if (sig < 1 || sig > NR_SIGS) return(EINVAL);
  if (sig == SIGKILL) return(OK);	/* SIGKILL may not ignored/caught */
  mask = 1 << (sig - 1);	/* singleton set with 'sig' bit on */

  /* All this func does is set the bit maps for subsequent sig processing. */
  if (func == SIG_IGN) {
	mp->mp_ignore |= mask;
	mp->mp_catch &= ~mask;
  } else if (func == SIG_DFL) {
	mp->mp_ignore &= ~mask;
	mp->mp_catch &= ~mask;
  } else {
	mp->mp_ignore &= ~mask;
	mp->mp_catch |= mask;
	mp->mp_func = func;
  }
  return(OK);
}

/*===========================================================================*
 *				do_alarm				     *
 *===========================================================================*/
PUBLIC int do_alarm()
{
/* Perform the alarm(seconds) system call. */

  register int r;
  unsigned sec;

  sec = (unsigned) seconds;
  //r = set_alarm(who, sec);
  return(r);
}

/*===========================================================================*
 *				do_kill					     *
 *===========================================================================*/
PUBLIC int do_kill()
{
/* Perform the kill(pid, kill_sig) system call. */

  //return check_sig(pid, kill_sig, mp->mp_effuid);
  return 0;
}

/*===========================================================================*
 *				do_ksig					     *
 *===========================================================================*/
PUBLIC int do_ksig()
{
/* Certain signals, such as segmentation violations and DEL, originate in the
 * kernel.  When the kernel detects such signals, it sets bits in a bit map.
 * As soon is MM is awaiting new work, the kernel sends MM a message containing
 * the process slot and bit map.  That message comes here.  The File System
 * also uses this mechanism to signal writing on broken pipes (SIGPIPE).
 */
	
	return(OK);
}

/*===========================================================================*
 *				do_pause				     *
 *===========================================================================*/
PUBLIC int do_pause()
{
/* Perform the pause() system call. */

  mp->mp_flags |= PAUSED;	/* turn on PAUSE bit */
  dont_reply = TRUE;
  return(OK);
}




