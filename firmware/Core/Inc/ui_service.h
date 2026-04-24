#ifndef UI_SERVICE_H
#define UI_SERVICE_H

#include "app_state.h"
#include "param_store.h"
#include "temp_manager.h"

typedef enum
{
	UI_KEY_NONE = 0,
	UI_KEY_SET,
	UI_KEY_UP,
	UI_KEY_DOWN,
	UI_KEY_BACK,
	UI_KEY_SET_LONG,
	UI_KEY_UP_REPEAT,
	UI_KEY_DOWN_REPEAT
} ui_key_event_t;

typedef enum
{
	UI_PAGE_HOME = 0,
	UI_PAGE_SET_TEMP,
	UI_PAGE_PID,
	UI_PAGE_ALARM,
	UI_PAGE_SCHEDULE,
	UI_PAGE_EXPORT,
	UI_PAGE_INFO,
	UI_PAGE_COUNT
} ui_page_t;

void ui_service_init(void);
void ui_service_tick_100ms(app_params_t *params);
void ui_service_tick_200ms(app_mode_t mode, const temp_snapshot_t *temp, const app_params_t *params, float pid_out, int heater_on, int alarm_on);
void ui_service_inject_key_event(ui_key_event_t key);
ui_page_t ui_service_get_page(void);
int ui_service_is_editing(void);
unsigned int ui_service_get_pid_field(void);

#endif
