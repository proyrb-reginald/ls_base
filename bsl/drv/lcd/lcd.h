#ifndef LCD_H
#define LCD_H

#include <lvgl.h>

/**
 * @brief LCD异步完成传输处理函数
 */
void lcd_async_handler(void);

/**
 * @brief LCD异步完成LVGL传输处理函数
 */
void lcd_lvgl_async_handler(void);

/**
 * @brief 初始化LCD屏幕
 */
void lcd_init(void);

/**
 * @brief LCD同步填充单色函数
 */
void lcd_fill_color_sync(int32_t sx, int32_t sy, uint32_t width, uint32_t height, uint16_t color);

/**
 * @brief LCD异步填充单色函数
 */
void lcd_fill_color_async(int32_t sx, int32_t sy, uint32_t width, uint32_t height, uint16_t color);

/**
 * @brief LCD同步内存搬运函数
 */
void lcd_fill_data_sync(int32_t sx, int32_t sy, uint32_t width, uint32_t height, uint16_t *data);

/**
 * @brief LCD异步内存搬运函数
 */
void lcd_fill_data_async(int32_t sx, int32_t sy, uint32_t width, uint32_t height, uint16_t *data);

/**
 * @brief LCD同步内存搬运LVGL函数
 */
void lcd_fill_lvgl_sync(lv_display_t *disp_drv, int32_t sx, int32_t sy, uint32_t width,
                        uint32_t height, uint16_t *data);

/**
 * @brief LCD异步内存搬运LVGL函数
 */
void lcd_fill_lvgl_async(lv_display_t *disp_drv, int32_t sx, int32_t sy, uint32_t width,
                         uint32_t height, uint16_t *data);

/**
 * @brief 测试LCD屏幕
 */
void lcd_test(void);

#endif
