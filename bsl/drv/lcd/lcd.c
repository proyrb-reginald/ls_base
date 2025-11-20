/* 导入接口层 */
#include <board.h>

void lcd_init(void)
{
    LL_GPIO_SetOutputPin(LCD_RST_GPIO_Port, LCD_RST_Pin);
    LL_mDelay(1);
    LL_GPIO_ResetOutputPin(LCD_RST_GPIO_Port, LCD_RST_Pin);
    LL_mDelay(1);
    LL_GPIO_SetOutputPin(LCD_RST_GPIO_Port, LCD_RST_Pin);
}

void lcd_fill_color(int sx, int sy, uint32_t width, uint32_t height, uint16_t color, void *dst_data)
{
    LL_DMA2D_SetMode(DMA2D, LL_DMA2D_MODE_R2M);
    LL_DMA2D_SetOutputColorMode(DMA2D, LL_DMA2D_OUTPUT_MODE_RGB565);
    LL_DMA2D_SetNbrOfPixelsPerLines(DMA2D, width);
    LL_DMA2D_SetNbrOfLines(DMA2D, height);
    LL_DMA2D_SetLineOffset(DMA2D, 800 - width);
    LL_DMA2D_SetOutputColor(DMA2D, color);
    LL_DMA2D_SetOutputMemAddr(DMA2D, ((uint32_t)dst_data + 2 * (sy * 800 + sx)));
    LL_DMA2D_EnableIT_TC(DMA2D);
    LL_DMA2D_Start(DMA2D);
}

void lcd_fill_data(int sx, int sy, uint32_t width, uint32_t height, void *src_data, void *dst_data)
{
    LL_DMA2D_SetMode(DMA2D, LL_DMA2D_MODE_M2M);
    LL_DMA2D_FGND_SetColorMode(DMA2D, LL_DMA2D_INPUT_MODE_RGB565);
    LL_DMA2D_SetOutputColorMode(DMA2D, LL_DMA2D_OUTPUT_MODE_RGB565);
    LL_DMA2D_SetNbrOfPixelsPerLines(DMA2D, width);
    LL_DMA2D_SetNbrOfLines(DMA2D, height);
    LL_DMA2D_FGND_SetLineOffset(DMA2D, 0);
    LL_DMA2D_SetLineOffset(DMA2D, 800 - width);
    LL_DMA2D_FGND_SetMemAddr(DMA2D, ((uint32_t)src_data + 2 * (sy * 800 + sx)));
    LL_DMA2D_SetOutputMemAddr(DMA2D, ((uint32_t)dst_data + 2 * (sy * 800 + sx)));
    LL_DMA2D_EnableIT_TC(DMA2D);
    LL_DMA2D_Start(DMA2D);
}

#if USE_TEST
#define LCD_BUF_SIZE ((uint32_t)((800 * 1280)))
static __attribute__((section(".sdram.lcd.0"))) volatile uint16_t lcd_buf_0[LCD_BUF_SIZE];
static __attribute__((section(".sdram.lcd.1"))) volatile uint16_t lcd_buf_1[LCD_BUF_SIZE];

void dma2d_irq_handler(void)
{
    LL_DMA2D_ClearFlag_TC(DMA2D);
}

void lcd_test(void)
{
    static volatile uint8_t cnt = 0;
    while (1)
    {
        rt_thread_mdelay(5);

        if (cnt % 2 == 0)
        {
            lcd_fill_color(0, 0, 800, 1280, 0xF800, (void *)lcd_buf_1);
        }
        else
        {
            lcd_fill_color(0, 0, 800, 1280, 0x001F, (void *)lcd_buf_1);
        }

        rt_thread_mdelay(25);
        lcd_fill_data(0, 0, 32, 32, (void *)lcd_buf_1, (void *)lcd_buf_0);

        cnt++;
    }
}
#endif