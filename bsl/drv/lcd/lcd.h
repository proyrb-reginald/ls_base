#ifndef LCD_H
#define LCD_H

/**
 * @brief 初始化LCD屏幕
 */
void lcd_init(void);

/**
 * @brief 测试LCD屏幕
 * @retval 0 功能正常
 * @retval 1 SDRAM覆盖读写出错
 * @retval 2 SDRAM单字节读写出错
 * @retval 3 SDRAM字节流读写出错
 */
void lcd_test(void);

#endif
