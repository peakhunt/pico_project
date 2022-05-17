#ifndef __APP_COMMON_DEF_H__
#define __APP_COMMON_DEF_H__

#include <stdint.h>
#include <stdio.h>

#ifndef bool
# define bool      uint8_t
#endif

#ifndef true
# define true      1
#endif

#ifndef false
# define false     0
#endif

extern volatile uint32_t    __uptime;

#endif //!__APP_COMMON_DEF_H__
