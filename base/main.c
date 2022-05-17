#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"

#include "bsp/board.h"
#include "tusb.h"

#include "app/app.h"

int
main()
{
  board_init();
  tusb_init();

  bi_decl(bi_program_description("This is a test binary."));
  //bi_decl(bi_1pin_with_name(LED_PIN, "On-board LED"));

  //stdio_init_all();

  app_init();

  while(1)
  {
    tud_task();
    app_run();
  }
}
