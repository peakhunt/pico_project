#include "RP2040.h"
#include "app_common.h"

#include "event_dispatcher.h"
#include "mainloop_timer.h"
#include "sys_timer.h"
#include "misc.h"

void
app_init(void)
{
  __disable_irq();

  event_dispatcher_init();
  mainloop_timer_init();

  sys_timer_init();

  misc_init();

  __enable_irq();
}

void
app_run(void)
{
  event_dispatcher_dispatch();
}
