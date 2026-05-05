#ifndef UI_KEY_INPUT_H
#define UI_KEY_INPUT_H

#include "hw_platform_port.h"
#include "ui_input_event.h"

typedef int (*ui_key_input_read_fn_t)(hw_key_id_t key, void *ctx);

void ui_key_input_init(void);
void ui_key_input_poll_100ms(void);
int ui_key_input_pop_event(ui_key_event_t *key);
void ui_key_input_inject_event(ui_key_event_t key);
void ui_key_input_set_read_fn(ui_key_input_read_fn_t fn, void *ctx);

#endif
