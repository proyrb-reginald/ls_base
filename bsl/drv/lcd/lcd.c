#include <drv.h>

#define LCD_FB_SIZE ((uint32_t)((800 * 1280)))
static __attribute__((section(".sdram.fb"))) volatile uint16_t lcd_fb[LCD_FB_SIZE];
static volatile uint8_t lcd_async_done_flag = 1;

void lcd_async_handler(void)
{
    lcd_async_done_flag = 1;
    LL_DMA2D_ClearFlag_TC(DMA2D);
}

void lcd_init(void)
{
    LL_GPIO_SetOutputPin(LCD_RST_GPIO_Port, LCD_RST_Pin);
    LL_mDelay(1);
    LL_GPIO_ResetOutputPin(LCD_RST_GPIO_Port, LCD_RST_Pin);
    LL_mDelay(1);
    LL_GPIO_SetOutputPin(LCD_RST_GPIO_Port, LCD_RST_Pin);
    LL_mDelay(1);
    LL_GPIO_SetOutputPin(LCD_BL_GPIO_Port, LCD_BL_Pin);
}

void lcd_fill_color_sync(int32_t sx, int32_t sy, uint32_t width, uint32_t height, uint16_t color)
{
    for (int32_t i = 0; i < height; i++)
    {
        sdram_write_16b_cover((uint32_t)lcd_fb + (sy + i) * 800 * 2 + sx * 2, color, width);
    }
}

void lcd_fill_color_async(int32_t sx, int32_t sy, uint32_t width, uint32_t height, uint16_t color)
{
    LL_DMA2D_SetMode(DMA2D, LL_DMA2D_MODE_R2M);
    LL_DMA2D_SetOutputColorMode(DMA2D, LL_DMA2D_OUTPUT_MODE_RGB565);
    LL_DMA2D_SetNbrOfPixelsPerLines(DMA2D, width);
    LL_DMA2D_SetNbrOfLines(DMA2D, height);
    LL_DMA2D_SetLineOffset(DMA2D, 800 - width);
    LL_DMA2D_SetOutputColor(DMA2D, color);
    LL_DMA2D_SetOutputMemAddr(DMA2D, ((uint32_t)lcd_fb + 2 * (sy * 800 + sx)));
    LL_DMA2D_EnableIT_TC(DMA2D);
    LL_DMA2D_Start(DMA2D);
}

void lcd_fill_data_sync(int sx, int sy, uint32_t width, uint32_t height, uint16_t color,
                        void *dst_data)
{
    for (int32_t i = 0; i < height; i++)
    {
        sdram_write_16b_cover((uint32_t)lcd_fb + (sy + i) * 800 * 2 + sx * 2, color, width);
    }
}

void lcd_fill_data_async(int sx, int sy, uint32_t width, uint32_t height, void *src_data,
                         void *dst_data)
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

void lcd_test(void)
{
#define TEST_MODE 0 // 0为同步填充 1为异步填充
    static volatile uint8_t cnt = 0;
    static uint16_t colors[4] = { 0xF800, 0x07E0, 0x001F, 0xFFFF };

    uint8_t index = cnt % 4;
    for (uint8_t i = 0; i < 4; i++)
    {
#if TEST_MODE
        lcd_fill_color_sync(0, i * 320, 800, 320, colors[(index + i) % 4]);
#else
        while (lcd_async_done_flag == 0);
        lcd_async_done_flag = 0;
        lcd_fill_color_async(0, i * 320, 800, 320, colors[(index + i) % 4]);
#endif
    }
    cnt++;
}
