#include "RP2040.h"
#include "tusb.h"

#include "app_common.h"

#include "shell_if_usb.h"
#include "shell.h"

#include "event_list.h"
#include "event_dispatcher.h"
#include "circ_buffer.h"

#include "hardware/gpio.h"

#define TX_BUFFER_SIZE      1024

////////////////////////////////////////////////////////////////////////////////
//
// private variables
//
////////////////////////////////////////////////////////////////////////////////
static void shellif_usb_tx_usb(void);

static CircBuffer    _rx_cb;
static uint8_t       _rx_buffer[CLI_RX_BUFFER_LENGTH];

static CircBuffer    _tx_cb;
static uint8_t       _tx_buffer[TX_BUFFER_SIZE];

static ShellIntf     _shell_usb_if;

static uint8_t       _tx_in_prog = false;

////////////////////////////////////////////////////////////////////////////////
//
// called from USB CDC task
//
////////////////////////////////////////////////////////////////////////////////
static void
shell_if_usb_rx_notify(uint8_t* buf, uint32_t len)
{
  //
  // runs in mainloop context
  //
  if(circ_buffer_enqueue(&_rx_cb, buf, len, true) == false)
  {
    // fucked up. overflow mostly.
    // do something here
  }

  shell_handle_rx(&_shell_usb_if);
}

////////////////////////////////////////////////////////////////////////////////
//
// shell callback
//
////////////////////////////////////////////////////////////////////////////////
static bool
shell_if_usb_get_rx_data(ShellIntf* intf, uint8_t* data)
{
  if(circ_buffer_dequeue(&_rx_cb, data, 1, true) == false)
  {
    return false;
  }
  return true;
}

static void
shell_if_usb_tx_usb(void)
{
  uint32_t num_bytes;
  uint32_t navailable;
  static uint8_t buf[64];

  if(circ_buffer_is_empty(&_tx_cb, true))
  {
    _tx_in_prog = false;
    return;
  }

  navailable = tud_cdc_write_available();

  num_bytes = _tx_cb.num_bytes > navailable ? navailable : _tx_cb.num_bytes;
  num_bytes = num_bytes > 64 ? 64 : num_bytes;

  circ_buffer_dequeue(&_tx_cb, buf, num_bytes, true);

  tud_cdc_write(buf, num_bytes);
  tud_cdc_write_flush();

  _tx_in_prog = true;
}

static bool
shell_if_usb_put_tx_data(ShellIntf* intf, uint8_t* data, uint16_t len)
{
  if(circ_buffer_enqueue(&_tx_cb, data, (uint8_t)len, true) == false)
  {
    //
    // no space left in queue. what should we do?
    //
    return false;
  }

  if(_tx_in_prog == false)
  {
    shell_if_usb_tx_usb();
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////
//
// public interfaces
//
////////////////////////////////////////////////////////////////////////////////
void
shell_if_usb_init(void)
{
  _shell_usb_if.cmd_buffer_ndx    = 0;
  _shell_usb_if.get_rx_data       = shell_if_usb_get_rx_data;
  _shell_usb_if.put_tx_data       = shell_if_usb_put_tx_data;

  INIT_LIST_HEAD(&_shell_usb_if.lh);

  circ_buffer_init(&_rx_cb, _rx_buffer, CLI_RX_BUFFER_LENGTH,
      NULL,
      NULL);

  circ_buffer_init(&_tx_cb, _tx_buffer, TX_BUFFER_SIZE,
      NULL,
      NULL);

  shell_if_register(&_shell_usb_if);
}

void
tud_cdc_tx_complete_cb(uint8_t itf)
{
  shell_if_usb_tx_usb();
}

void
tud_cdc_rx_cb(uint8_t itf)
{
  if(tud_cdc_available())
  {
    // read datas
    static char buf[64];
    uint32_t count = tud_cdc_read(buf, sizeof(buf));

    shell_if_usb_rx_notify(buf, count);
  }
}
