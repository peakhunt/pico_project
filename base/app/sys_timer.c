#include "pico/stdlib.h"

#include "app_common.h"
#include "event_dispatcher.h"
#include "event_list.h"

static struct repeating_timer _timer;

volatile uint32_t     __uptime = 0;

//
// XXX
// mostly IRQ context
//
static bool
repeating_timer_callback(struct repeating_timer *t)
{
  static uint16_t   count = 0;

  count++;
  if(count >= 1000)
  {
    __uptime++;
    count = 0;
  }

  event_set(1 << DISPATCH_EVENT_TIMER_TICK);
  return true;
}

void
sys_timer_init(void)
{
  add_repeating_timer_ms(-1, repeating_timer_callback, NULL, &_timer);
  // nothing to do
}
