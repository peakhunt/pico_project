#include "RP2040.h"
#include "app_common.h"

#include "event_dispatcher.h"
#include "mainloop_timer.h"
#include "sys_timer.h"
#include "misc.h"
#include "shell.h"
#include "shell_if_usb.h"
#include "ws2812.h"

void
app_init(void)
{
  __disable_irq();

  event_dispatcher_init();
  mainloop_timer_init();

  sys_timer_init();

  ws2812_init();
  misc_init();

  shell_init();

  __enable_irq();
}

void
app_task(void)
{
  event_dispatcher_dispatch();
}
