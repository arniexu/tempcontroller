#include "app_config.h"

#if (APP_USE_LVGL_UI == 1U)

/*
 * Single translation unit build for LVGL minimal bring-up on Keil.
 * This avoids maintaining hundreds of file entries in uvprojx.
 */

#include "src/lv_init.c"

#include "src/core/lv_group.c"
#include "src/core/lv_obj.c"
#include "src/core/lv_obj_class.c"
#include "src/core/lv_obj_draw.c"
#include "src/core/lv_obj_event.c"
#include "src/core/lv_obj_id_builtin.c"
#include "src/core/lv_obj_pos.c"
#include "src/core/lv_obj_property.c"
#include "src/core/lv_obj_scroll.c"
#include "src/core/lv_obj_style.c"
#include "src/core/lv_obj_style_gen.c"
#include "src/core/lv_obj_tree.c"
#include "src/core/lv_observer.c"
#include "src/core/lv_refr.c"

#include "src/display/lv_display.c"

#include "src/draw/lv_draw.c"
#include "src/draw/lv_draw_arc.c"
#include "src/draw/lv_draw_buf.c"
#include "src/draw/lv_draw_image.c"
#include "src/draw/lv_draw_label.c"
#include "src/draw/lv_draw_line.c"
#include "src/draw/lv_draw_mask.c"
#include "src/draw/lv_draw_rect.c"
#include "src/draw/lv_draw_triangle.c"
#include "src/draw/lv_image_decoder.c"

#include "src/draw/sw/lv_draw_sw.c"
#include "src/draw/sw/lv_draw_sw_arc.c"
#include "src/draw/sw/lv_draw_sw_border.c"
#include "src/draw/sw/lv_draw_sw_box_shadow.c"
#include "src/draw/sw/lv_draw_sw_fill.c"
#include "src/draw/sw/lv_draw_sw_grad.c"
#include "src/draw/sw/lv_draw_sw_img.c"
#include "src/draw/sw/lv_draw_sw_letter.c"
#include "src/draw/sw/lv_draw_sw_line.c"
#include "src/draw/sw/lv_draw_sw_mask.c"
#include "src/draw/sw/lv_draw_sw_mask_rect.c"
#include "src/draw/sw/lv_draw_sw_triangle.c"
#include "src/draw/sw/lv_draw_sw_utils.c"

#include "src/draw/sw/blend/lv_draw_sw_blend.c"
#include "src/draw/sw/blend/lv_draw_sw_blend_to_rgb565.c"

#include "src/font/lv_font.c"
#include "src/font/lv_font_montserrat_14.c"
#include "src/font/lv_font_montserrat_20.c"

#include "src/indev/lv_gridnav.c"
#include "src/indev/lv_indev.c"
#include "src/indev/lv_indev_gesture.c"
#include "src/indev/lv_indev_scroll.c"

#include "src/layouts/lv_layout.c"

#include "src/misc/lv_anim.c"
#include "src/misc/lv_anim_timeline.c"
#include "src/misc/lv_area.c"
#include "src/misc/lv_array.c"
#include "src/misc/lv_async.c"
#include "src/misc/lv_bidi.c"
#include "src/misc/lv_circle_buf.c"
#include "src/misc/lv_color.c"
#include "src/misc/lv_color_op.c"
#include "src/misc/lv_event.c"
#include "src/misc/lv_fs.c"
#include "src/misc/lv_grad.c"
#include "src/misc/lv_iter.c"
#include "src/misc/lv_ll.c"
#include "src/misc/lv_log.c"
#include "src/misc/lv_lru.c"
#include "src/misc/lv_math.c"
#include "src/misc/lv_matrix.c"
#include "src/misc/lv_palette.c"
#include "src/misc/lv_pending.c"
#include "src/misc/lv_rb.c"
#include "src/misc/lv_style.c"
#include "src/misc/lv_style_gen.c"
#include "src/misc/lv_text.c"
#include "src/misc/lv_text_ap.c"
#include "src/misc/lv_timer.c"
#include "src/misc/lv_tree.c"
#include "src/misc/lv_utils.c"

#include "src/osal/lv_os.c"
#include "src/osal/lv_os_none.c"

#include "src/stdlib/lv_mem.c"
#include "src/stdlib/builtin/lv_mem_core_builtin.c"
#include "src/stdlib/builtin/lv_sprintf_builtin.c"
#include "src/stdlib/builtin/lv_string_builtin.c"
#include "src/stdlib/builtin/lv_tlsf.c"
#include "src/stdlib/clib/lv_mem_core_clib.c"
#include "src/stdlib/clib/lv_sprintf_clib.c"
#include "src/stdlib/clib/lv_string_clib.c"

#include "src/themes/lv_theme.c"

#include "src/tick/lv_tick.c"

#include "src/widgets/button/lv_button.c"
#include "src/widgets/label/lv_label.c"

#endif
