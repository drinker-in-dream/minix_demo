#include "../INCLUDE/lib.h"

PUBLIC int kill(int proc, int sig)
//int proc;			/* which process is to be sent the signal */
//int sig;			/* signal number */
{
  return callm1(MM, KILL, proc, sig, 0, NIL_PTR, NIL_PTR, NIL_PTR);
}
