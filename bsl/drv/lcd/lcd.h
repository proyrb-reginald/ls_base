#ifndef LCD_H
#define LCD_H

/**
 * @brief LCD异步完成传输处理函数
 */
void lcd_async_handler(void);

/**
 * @brief 初始化LCD屏幕
 */
void lcd_init(void);

/**
 * @brief 测试LCD屏幕
 */
void lcd_test(void);

#endif
