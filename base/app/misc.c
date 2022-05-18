#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"

#include "soft_timer.h"
#include "mainloop_timer.h"
#include "misc.h"

#define USE_LED_PIO   1

#ifdef USE_LED_PIO
#include "led.pio.h"
#endif

#define BLINK_INTERVAL      50

#define LED_PIN   25

static SoftTimerElem  _blink_timer;

#ifdef USE_LED_PIO
static PIO pio = pio0;
static uint sm;
static uint8_t _on = 0;
#endif

static void
blink_callback(SoftTimerElem* te)
{
#ifndef USE_LED_PIO
  gpio_put(LED_PIN, !gpio_get(LED_PIN));
  mainloop_timer_schedule(&_blink_timer, BLINK_INTERVAL);
#else

  pio_sm_put_blocking(pio, sm, _on);
  _on = !_on;

  mainloop_timer_schedule(&_blink_timer, BLINK_INTERVAL);
#endif
}

void
misc_init(void)
{
#ifndef USE_LED_PIO
  // init LED gpio
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  // init blink timer
  soft_timer_init_elem(&_blink_timer);
  _blink_timer.cb    = blink_callback;
  mainloop_timer_schedule(&_blink_timer, BLINK_INTERVAL);
#else
  uint offset = pio_add_program(pio, &led_program);

  sm = pio_claim_unused_sm(pio, true);
  led_program_init(pio, sm, offset, LED_PIN);

  soft_timer_init_elem(&_blink_timer);
  _blink_timer.cb    = blink_callback;
  mainloop_timer_schedule(&_blink_timer, BLINK_INTERVAL);
#endif
}
