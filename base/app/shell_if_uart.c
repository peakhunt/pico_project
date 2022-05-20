#include "RP2040.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

#include "app_common.h"
#include "shell_if_uart.h"

#include "circ_buffer.h"
#include "event_list.h"
#include "event_dispatcher.h"

////////////////////////////////////////////////////////////////////////////////
//
// private definitions
//
////////////////////////////////////////////////////////////////////////////////
#define UART_ID                         uart1
#define BAUD_RATE                       115200
#define DATA_BITS                       8
#define STOP_BITS                       1
#define PARITY                          UART_PARITY_NONE

#define UART_TX_PIN                     8
#define UART_RX_PIN                     9


////////////////////////////////////////////////////////////////////////////////
//
// private variables
//
////////////////////////////////////////////////////////////////////////////////
static CircBuffer             _rx_cb;
static volatile uint8_t       _rx_buffer[CLI_RX_BUFFER_LENGTH];

static CircBuffer             _tx_cb;
static volatile uint8_t       _tx_buffer[CLI_RX_BUFFER_LENGTH];

static ShellIntf              _shell_uart_if;

int UART_IRQ = UART1_IRQ;

static void uart_read_callback(void* huart, bool error);

////////////////////////////////////////////////////////////////////////////////
//
// RX callback
//
////////////////////////////////////////////////////////////////////////////////
static
void on_uart_interrupt(void)
{
  //
  // RX first
  //
  if (uart_is_readable(UART_ID))
  {
    do
    {
      uint8_t ch = uart_getc(UART_ID);

      if(circ_buffer_enqueue(&_rx_cb, &ch, 1, true) == false)
      {
        // f*cked up. overflow
        // do something meaningful here
      }
    } while (uart_is_readable(UART_ID));
    event_set(1 << DISPATCH_EVENT_UART_CLI_RX);
  }

  //
  // TX check
  //
  if (uart_is_writable(UART_ID))
  {
    uint8_t c;

    if(circ_buffer_dequeue(&_tx_cb, &c, 1, true) == false)
    {
      uart_set_irq_enables(UART_ID, true, false);
      return;
    }
    uart_putc(UART_ID, c);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// private utilities
//
////////////////////////////////////////////////////////////////////////////////
static void
shell_if_uart_enter_critical(CircBuffer* cb)
{
  NVIC_DisableIRQ(UART_IRQ);
  __DSB();
  __ISB();
}

static void
shell_if_uart_leave_critical(CircBuffer* cb)
{
  NVIC_EnableIRQ(UART_IRQ);
}

////////////////////////////////////////////////////////////////////////////////
//
// callbacks for core shell and rx interrupt
//
////////////////////////////////////////////////////////////////////////////////
static bool
shell_if_uart_get_rx_data(ShellIntf* intf, uint8_t* data)
{
  if(circ_buffer_dequeue(&_rx_cb, data, 1, false) == false)
  {
    return false;
  }
  return true;
}

static bool
shell_if_uart_put_tx_data(ShellIntf* intf, uint8_t* data, uint16_t len)
{
  if(circ_buffer_enqueue(&_tx_cb, data, len, false) == false)
  {
    // XXX overflow. do something
    return false;
  }

  //
  // XXX IRQ disable for atomic operation
  //
  shell_if_uart_enter_critical(NULL);
  uart_set_irq_enables(UART_ID, true, true);
  shell_if_uart_leave_critical(NULL);

  return true;
}

static void
shell_if_uart_event_handler(uint32_t event)
{
  shell_handle_rx(&_shell_uart_if);
}

////////////////////////////////////////////////////////////////////////////////
//
// public interfaces
//
////////////////////////////////////////////////////////////////////////////////
void
shell_if_uart_init(void)
{
  _shell_uart_if.cmd_buffer_ndx    = 0;
  _shell_uart_if.get_rx_data       = shell_if_uart_get_rx_data;
  _shell_uart_if.put_tx_data       = shell_if_uart_put_tx_data;

  INIT_LIST_HEAD(&_shell_uart_if.lh);

  circ_buffer_init(&_rx_cb, _rx_buffer, CLI_RX_BUFFER_LENGTH,
      shell_if_uart_enter_critical,
      shell_if_uart_leave_critical);

  circ_buffer_init(&_tx_cb, _tx_buffer, CLI_RX_BUFFER_LENGTH,
      shell_if_uart_enter_critical,
      shell_if_uart_leave_critical);

  event_register_handler(shell_if_uart_event_handler, DISPATCH_EVENT_UART_CLI_RX);
  shell_if_register(&_shell_uart_if);

  //
  // uart hardware init
  //
  uart_init(UART_ID, 2400);

  gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
  gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

  uart_set_baudrate(UART_ID, BAUD_RATE);
  uart_set_hw_flow(UART_ID, false, false);
  uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
  uart_set_fifo_enabled(UART_ID, false);

  irq_set_exclusive_handler(UART_IRQ, on_uart_interrupt);
  irq_set_enabled(UART_IRQ, true);

  uart_set_irq_enables(UART_ID, true, false);
}
