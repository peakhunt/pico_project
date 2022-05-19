#include <string.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "ws2812.pio.h"

#include "soft_timer.h"
#include "mainloop_timer.h"
#include "ws2812.h"

#define WS2812_PIN    22
#define IS_RGBW       false

#define WS2812_UPDATE_INTERVAL      50      // 20 Hz

static PIO pio = pio1;
static uint sm;

static uint32_t   _led_mem[WS2812_NUM_LEDS]; 
static uint32_t   _led_mem_dma[WS2812_NUM_LEDS]; 

static SoftTimerElem  _update_tmr;

static int _dma_chan;
static bool _rotate = false;

static void
ws2812_update(SoftTimerElem* te)
{
  if (_rotate)
  {
    uint32_t last = _led_mem[WS2812_NUM_LEDS - 1];

    for(int i = WS2812_NUM_LEDS - 2; i >= 0; i--)
    {
      _led_mem[i + 1] = _led_mem[i];
    }
    _led_mem[0] = last;
  }

  memcpy(_led_mem_dma, _led_mem, sizeof(_led_mem));

#if 0
  for(int i = 0; i < WS2812_NUM_LEDS; i++)
  {
    pio_sm_put_blocking(pio, 0, _led_mem_dma[i]);
  }
#else
  //
  // FIXME a simple mechanism to avoid DMA transfer if it's already in progress.
  // realistically it won't happen but it doesn't hurt anyway
  //
  dma_channel_set_read_addr(_dma_chan, _led_mem_dma, true);
#endif

  mainloop_timer_schedule(&_update_tmr, WS2812_UPDATE_INTERVAL);
}

//
// XXX
// in IRQ context
//
static void
dma_handler(void)
{
  // Clear the interrupt request.
  dma_hw->ints0 = 1u << _dma_chan;
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

  // DMA init
  _dma_chan = dma_claim_unused_channel(true);
  dma_channel_config c = dma_channel_get_default_config(_dma_chan);
  channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
  //channel_config_set_read_increment(&c, false);
  channel_config_set_dreq(&c, DREQ_PIO1_TX0);

  dma_channel_configure(
      _dma_chan,
      &c,
      &pio1_hw->txf[0], // Write address (only need to set this once)
      NULL,             // Don't provide a read address yet
      WS2812_NUM_LEDS, 
      false             // Don't start yet
  );

  dma_channel_set_irq0_enabled(_dma_chan, true);

  irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
  irq_set_enabled(DMA_IRQ_0, true);
}

void
ws2812_set_led(uint32_t ndx, uint32_t value)
{
  _led_mem[ndx] = value;
}

void
ws2812_set_rotate(bool tf)
{
  _rotate = tf;
}
