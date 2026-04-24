#ifndef BSP_OLED_H
#define BSP_OLED_H

#include <stdint.h>

#define BSP_OLED_LINE_COUNT      (4U)
#define BSP_OLED_LINE_CHARS      (21U)

void bsp_oled_init(void);
void bsp_oled_clear(void);
void bsp_oled_draw_text(uint8_t line, const char *text);
void bsp_oled_refresh(void);
const char *bsp_oled_mock_get_line(uint8_t line);

#endif
