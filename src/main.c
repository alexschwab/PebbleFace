#include <pebble.h>
#include "face_handler.h"

// forward declarations
void handle_init(void);
void handle_deinit(void);

// main entry point
int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}

void handle_init(void)
{
  show_main_face();
}

void handle_deinit(void)
{
  hide_main_face();
}