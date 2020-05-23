#include "common.h"
_RegSet* do_syscall();
extern _RegSet* schedule(_RegSet *prev);

static _RegSet* do_event(_Event e, _RegSet* r)
{
	 _RegSet *ret = NULL;
    switch (e.event) {
    default: panic("Unhandled event ID = %d", e.event);
    case _EVENT_SYSCALL: do_syscall(r); break;
    case _EVENT_TRAP: ret = schedule(r); break;
    case _EVENT_IRQ_TIME: ret = schedule(r); Log("Time_IRQ");
			  break;
  }

  return ret;
}

void init_irq(void) {
  _asye_init(do_event);
}
