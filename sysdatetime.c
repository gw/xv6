// System calls related to dates and times.

#include "types.h"
#include "defs.h"
#include "date.h"

int
sys_date(void)
{
  char *r;

  if (argptr(0, &r, sizeof(struct rtcdate)) < 0)
    return -1;

  cmostime((struct rtcdate*)r);
  return 0;
}
