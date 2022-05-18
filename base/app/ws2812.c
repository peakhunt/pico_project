#include <string.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"

#include "soft_timer.h"
#include "mainloop_timer.h"
#include "ws2812.h"

#define WS2812_PIN    24
#define IS_RGBW       false

#define WS2812_UPDATE_INTERVAL      100      // 10 Hz

static PIO pio = pio1;
static uint sm;

static uint32_t   _led_mem[WS2812_NUM_LEDS]; 
static uint32_t   _led_mem_dma[WS2812_NUM_LEDS]; 

static SoftTimerElem  _update_tmr;

static void
ws2812_update(SoftTimerElem* te)
{
  memcpy(_led_mem_dma, _led_mem, sizeof(_led_mem));

  // FIXME update via DMA

  mainloop_timer_schedule(&_update_tmr, WS2812_UPDATE_INTERVAL);
}

void
ws2812_init(void)
{
  uint offset = pio_add_program(pio, &ws2812_program);
  sm = pio_claim_unused_sm(pio, true);

  ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

  soft_timer_init_elem(&_update_tmr);
  _update_tmr.cb    = ws2812_update;
  mainloop_timer_schedule(&_update_tmr, WS2812_UPDATE_INTERVAL);

  // FIXME DMA init
}

void
ws2812_set_led(uint32_t ndx, uint32_t value)
{
  _led_mem[ndx] = value;
}
