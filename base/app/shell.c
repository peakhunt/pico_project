#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "hardware/pll.h"
#include "hardware/clocks.h"
#include "hardware/structs/pll.h"
#include "hardware/structs/clocks.h"

#include "app_common.h"
#include "shell.h"
#include "shell_if_usb.h"
#include "shell_if_uart.h"
#include "version.h"
#include "ws2812.h"

////////////////////////////////////////////////////////////////////////////////
//
// private definitions
//
////////////////////////////////////////////////////////////////////////////////

#define SHELL_MAX_COLUMNS_PER_LINE      128
#define SHELL_COMMAND_MAX_ARGS          8

typedef void (*shell_command_handler)(ShellIntf* intf, int argc, const char** argv);

typedef struct
{
  const char*           command;
  const char*           description;
  shell_command_handler handler;
} ShellCommand;

////////////////////////////////////////////////////////////////////////////////
//
// private prototypes
//
////////////////////////////////////////////////////////////////////////////////
static void shell_command_help(ShellIntf* intf, int argc, const char** argv);
static void shell_command_version(ShellIntf* intf, int argc, const char** argv);
static void shell_command_uptime(ShellIntf* intf, int argc, const char** argv);
static void shell_command_clock(ShellIntf* intf, int argc, const char** argv);
static void shell_command_ws2812(ShellIntf* intf, int argc, const char** argv);


////////////////////////////////////////////////////////////////////////////////
//
// private variables
//
////////////////////////////////////////////////////////////////////////////////
const uint8_t                 _welcome[] = "\r\n**** Welcome ****\r\n";
const uint8_t                 _prompt[]  = "\r\npico> ";

static char                   _print_buffer[SHELL_MAX_COLUMNS_PER_LINE + 1];

static LIST_HEAD(_shell_intf_list);

static const ShellCommand     _commands[] = 
{
  {
    "help",
    "show this command",
    shell_command_help,
  },
  {
    "version",
    "show version",
    shell_command_version,
  },
  {
    "uptime",
    "show system uptime",
    shell_command_uptime,
  },
  {
    "clock",
    "show system clock info",
    shell_command_clock,
  },
  {
    "ws2812",
    "control ws2812 led",
    shell_command_ws2812,
  },
};


////////////////////////////////////////////////////////////////////////////////
//
// shell utilities
//
////////////////////////////////////////////////////////////////////////////////
static inline void
shell_prompt(ShellIntf* intf)
{
  shell_printf(intf, "%s", _prompt);
}

////////////////////////////////////////////////////////////////////////////////
//
// shell command handlers
//
////////////////////////////////////////////////////////////////////////////////
static void
shell_command_help(ShellIntf* intf, int argc, const char** argv)
{
  size_t i;

  shell_printf(intf, "\r\n");

  for(i = 0; i < sizeof(_commands)/sizeof(ShellCommand); i++)
  {
    shell_printf(intf, "%-20s: ", _commands[i].command);
    shell_printf(intf, "%s\r\n", _commands[i].description);
  }
}

static void
shell_command_version(ShellIntf* intf, int argc, const char** argv)
{
  shell_printf(intf, "\r\n");
  shell_printf(intf, "%s\r\n", VERSION);
}

static void
shell_command_uptime(ShellIntf* intf, int argc, const char** argv)
{
  shell_printf(intf, "\r\n");
  shell_printf(intf, "System Uptime: %lu\r\n", __uptime);
}

static void
shell_command_clock(ShellIntf* intf, int argc, const char** argv)
{
  shell_printf(intf, "\r\n");

  uint f_pll_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY);
  uint f_pll_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY);
  uint f_rosc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC);
  uint f_clk_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
  uint f_clk_peri = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);
  uint f_clk_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);
  uint f_clk_adc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_ADC);
  uint f_clk_rtc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_RTC);

  shell_printf(intf, "F_PLL_SYS   = %dKHz\r\n", f_pll_sys);
  shell_printf(intf, "F_PLL_USB   = %dKHz\r\n", f_pll_usb);
  shell_printf(intf, "F_ROSC      = %dKHz\r\n", f_rosc);
  shell_printf(intf, "F_CLK_SYS   = %dKHz\r\n", f_clk_sys);
  shell_printf(intf, "F_CLK_PERI  = %dKHz\r\n", f_clk_peri);
  shell_printf(intf, "F_CLK_USB   = %dKHz\r\n", f_clk_usb);
  shell_printf(intf, "F_CLK_ADC   = %dKHz\r\n", f_clk_adc);
  shell_printf(intf, "F_CLK_RTC   = %dKHz\r\n", f_clk_rtc);

  shell_printf(intf, "\r\n");

  shell_printf(intf, "clk_gpout0  = %dKHz\r\n", clock_get_hz(clk_gpout0) / 1000);
  shell_printf(intf, "clk_gpout1  = %dKHz\r\n", clock_get_hz(clk_gpout1) / 1000);
  shell_printf(intf, "clk_gpout2  = %dKHz\r\n", clock_get_hz(clk_gpout2) / 1000);
  shell_printf(intf, "clk_gpout3  = %dKHz\r\n", clock_get_hz(clk_gpout3) / 1000);
  shell_printf(intf, "clk_ref     = %dKHz\r\n", clock_get_hz(clk_ref) / 1000);
  shell_printf(intf, "clk_sys     = %dKHz\r\n", clock_get_hz(clk_sys) / 1000);
  shell_printf(intf, "clk_peri    = %dKHz\r\n", clock_get_hz(clk_peri) / 1000);
  shell_printf(intf, "clk_usb     = %dKHz\r\n", clock_get_hz(clk_usb) / 1000);
  shell_printf(intf, "clk_adc     = %dKHz\r\n", clock_get_hz(clk_adc) / 1000);
  shell_printf(intf, "clk_rtc     = %dKHz\r\n", clock_get_hz(clk_rtc) / 1000);
}

static void
shell_command_ws2812(ShellIntf* intf, int argc, const char** argv)
{
  shell_printf(intf, "\r\n");

  if(argc < 2)
  {
    goto invalid_cmd;
  }

  if(strcmp(argv[1], "color") == 0)
  {
    if(argc != 6)
    {
      goto invalid_cmd;
    }

    uint32_t ndx = atoi(argv[2]);

    if(ndx >= WS2812_NUM_LEDS)
    {
      goto invalid_cmd;
    }

    uint8_t   r, g, b;
    uint32_t  color;

    r = (uint8_t)atoi(argv[3]);
    g = (uint8_t)atoi(argv[4]);
    b = (uint8_t)atoi(argv[5]);

    color = ((uint32_t)(r) << 8) |
            ((uint32_t)(g) << 16) |
            (uint32_t)(b);

    color = color << 8;

    ws2812_set_led(ndx, color);

    shell_printf(intf, "set %d to %d:%d:%d\r\n", ndx, r,g,b);
  }
  else if(strcmp(argv[1], "rotate") == 0)
  {
    if(argc != 3)
    {
      goto invalid_cmd;
    }

    uint8_t tf = atoi(argv[2]);

    ws2812_set_rotate(tf);
    shell_printf(intf, "set rotated to %d\r\n", tf);
  }
  else
  {
    goto invalid_cmd;
  }
  return;

invalid_cmd:
  shell_printf(intf, "syntax error\r\n");
  shell_printf(intf, "%s color [index 0-%d] r g b\r\n", argv[0], WS2812_NUM_LEDS -1);
  shell_printf(intf, "%s rotate 0|1\r\n", argv[0]);
}

////////////////////////////////////////////////////////////////////////////////
//
// shell core
//
////////////////////////////////////////////////////////////////////////////////
static void
shell_execute_command(ShellIntf* intf, char* cmd)
{
  static const char*    argv[SHELL_COMMAND_MAX_ARGS];
  int                   argc = 0;
  size_t                i;
  char                  *s, *t;

  while((s = strtok_r(argc  == 0 ? cmd : NULL, " \t", &t)) != NULL)
  {
    if(argc >= SHELL_COMMAND_MAX_ARGS)
    {
      shell_printf(intf, "\r\nError: too many arguments\r\n");
      return;
    }
    argv[argc++] = s;
  }

  if(argc == 0)
  {
    return;
  }

  for(i = 0; i < sizeof(_commands)/sizeof(ShellCommand); i++)
  {
    if(strcmp(_commands[i].command, argv[0]) == 0)
    {
      shell_printf(intf, "\r\nExecuting %s\r\n", argv[0]);
      _commands[i].handler(intf, argc, argv);
      return;
    }
  }
  shell_printf(intf, "%s", "\r\nUnknown Command: ");
  shell_printf(intf, "%s", argv[0]);
  shell_printf(intf, "%s", "\r\n");
}


void
shell_printf(ShellIntf* intf, const char* fmt, ...)
{
  va_list   args;
  int       len;

  va_start(args, fmt);
  len = vsnprintf(_print_buffer, SHELL_MAX_COLUMNS_PER_LINE, fmt, args);
  va_end(args);

  do
  {
  } while(intf->put_tx_data(intf, (uint8_t*)_print_buffer, len) == false);
}


////////////////////////////////////////////////////////////////////////////////
//
// public interface
//
////////////////////////////////////////////////////////////////////////////////
void
shell_init(void)
{
  shell_if_usb_init();
  shell_if_uart_init();
}

void
shell_start(void)
{
  ShellIntf* intf;

  list_for_each_entry(intf, &_shell_intf_list, lh)
  {
    shell_printf(intf, "%s", _welcome);
    shell_prompt(intf);
  }
}


void
shell_if_register(ShellIntf* intf)
{
  list_add_tail(&intf->lh, &_shell_intf_list);
}

void
shell_handle_rx(ShellIntf* intf)
{
  uint8_t   b;

  while(1)
  {
    if(intf->get_rx_data(intf, &b) == false)
    {
      return;
    }

    if(b != '\r' && intf->cmd_buffer_ndx < SHELL_MAX_COMMAND_LEN)
    {
      if(b == '\b' || b == 0x7f)
      {
        if(intf->cmd_buffer_ndx > 0)
        {
          shell_printf(intf, "%c%c%c", b, 0x20, b);
          intf->cmd_buffer_ndx--;
        }
      }
      else
      {
        shell_printf(intf, "%c", b);
        intf->cmd_buffer[intf->cmd_buffer_ndx++] = b;
      }
    }
    else if(b == '\r')
    {
      intf->cmd_buffer[intf->cmd_buffer_ndx++] = '\0';

      shell_execute_command(intf, (char*)intf->cmd_buffer);

      intf->cmd_buffer_ndx = 0;
      shell_prompt(intf);
    }
  }
}

struct list_head*
shell_get_intf_list(void)
{
  return &_shell_intf_list;
}
