#ifndef __WS2812_DEF_H__
#define __WS2812_DEF_H__

#include "app_common.h"

#define WS2812_NUM_LEDS         8

extern void ws2812_init(void);
extern void ws2812_set_led(uint32_t ndx, uint32_t value);

#endif /* !__WS2812_DEF_H__ */
