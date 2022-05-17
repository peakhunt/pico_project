#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "soft_timer.h"
#include "mainloop_timer.h"
#include "misc.h"

#define BLINK_INTERVAL      50

#define LED_PIN   25

static SoftTimerElem  _blink_timer;

static void
blink_callback(SoftTimerElem* te)
{
  gpio_put(LED_PIN, !gpio_get(LED_PIN));
  mainloop_timer_schedule(&_blink_timer, BLINK_INTERVAL);
}

void
misc_init(void)
{
  // init LED gpio
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  // init blink timer
  soft_timer_init_elem(&_blink_timer);
  _blink_timer.cb    = blink_callback;
  mainloop_timer_schedule(&_blink_timer, BLINK_INTERVAL);
}
