#include "../INCLUDE/lib.h"

PUBLIC int chown(char *name, int owner, int grp)
//char *name;
//int owner, grp;
{
  return callm1(FS, CHOWN, len(name), owner, grp, name, NIL_PTR, NIL_PTR);
}
