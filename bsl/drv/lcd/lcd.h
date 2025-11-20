#ifndef LCD_H
#define LCD_H

void lcd_init(void);
void dma2d_irq_handler(void);
void lcd_fill_color(int sx, int sy, uint32_t width, uint32_t height, uint16_t color,
                    void *dst_data);
void lcd_fill_data(int sx, int sy, uint32_t width, uint32_t height, void *src_data, void *dst_data);

#if USE_TEST
/* 测试LCD设备 */
void lcd_test(void);
#endif

#endif