#include <board.h>

#define LCD_FB_SIZE ((uint32_t)((800 * 1280)))
static __attribute__((section(".sdram.fb_main"))) volatile uint16_t lcd_fb_main[LCD_FB_SIZE];
static __attribute__((section(".sdram.fb_back"))) volatile uint16_t lcd_fb_back[LCD_FB_SIZE];
static volatile uint8_t lcd_async_done_flag = 1;
static lv_display_t *lv_disp_drv = NULL;

void lcd_async_handler(void)
{
    lcd_async_done_flag = 1;
    LL_DMA2D_ClearFlag_TC(DMA2D);
}

void lcd_lvgl_async_handler(void)
{
    lcd_async_done_flag = 1;
    lv_display_flush_ready(lv_disp_drv);
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
        sdram_write_16b_cover((uint32_t)lcd_fb_main + (sy + i) * 800 * 2 + sx * 2, color, width);
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
    LL_DMA2D_SetOutputMemAddr(DMA2D, ((uint32_t)lcd_fb_main + 2 * (sy * 800 + sx)));
    LL_DMA2D_EnableIT_TC(DMA2D);
    LL_DMA2D_Start(DMA2D);
}

void lcd_fill_data_sync(int32_t sx, int32_t sy, uint32_t width, uint32_t height, uint16_t *data)
{
    for (int32_t i = 0; i < height; i++)
    {
        sdram_write_16b_stream((uint32_t)lcd_fb_main + (sy + i) * 800 * 2 + sx * 2,
                               (uint16_t *)((uint32_t)data + i * width * 2), width);
    }
}

void lcd_fill_data_async(int32_t sx, int32_t sy, uint32_t width, uint32_t height, uint16_t *data)
{
    LL_DMA2D_SetMode(DMA2D, LL_DMA2D_MODE_M2M);
    LL_DMA2D_FGND_SetColorMode(DMA2D, LL_DMA2D_INPUT_MODE_RGB565);
    LL_DMA2D_SetOutputColorMode(DMA2D, LL_DMA2D_OUTPUT_MODE_RGB565);
    LL_DMA2D_SetNbrOfPixelsPerLines(DMA2D, width);
    LL_DMA2D_SetNbrOfLines(DMA2D, height);
    LL_DMA2D_FGND_SetLineOffset(DMA2D, 800 - width);
    LL_DMA2D_SetLineOffset(DMA2D, 800 - width);
    LL_DMA2D_FGND_SetMemAddr(DMA2D, ((uint32_t)data + 2 * (sy * 800 + sx)));
    LL_DMA2D_SetOutputMemAddr(DMA2D, ((uint32_t)lcd_fb_main + 2 * (sy * 800 + sx)));
    LL_DMA2D_EnableIT_TC(DMA2D);
    LL_DMA2D_Start(DMA2D);
}

void lcd_fill_lvgl_sync(lv_display_t *disp_drv, int32_t sx, int32_t sy, uint32_t width,
                        uint32_t height, uint16_t *data)
{
    lv_disp_drv = disp_drv;
    for (int32_t i = 0; i < height; i++)
    {
        sdram_write_16b_stream((uint32_t)lcd_fb_main + ((sy + i) * 800 + sx) * 2,
                               (uint16_t *)((uint32_t)data + i * width * 2), width);
    }
}

void lcd_fill_lvgl_async(lv_display_t *disp_drv, int32_t sx, int32_t sy, uint32_t width,
                         uint32_t height, uint16_t *data)
{
    LV_LOG_USER("w:%ld h:%ld", width, height);
    lv_disp_drv = disp_drv;
    LL_DMA2D_SetMode(DMA2D, LL_DMA2D_MODE_M2M);
    LL_DMA2D_FGND_SetColorMode(DMA2D, LL_DMA2D_INPUT_MODE_RGB565);
    LL_DMA2D_SetOutputColorMode(DMA2D, LL_DMA2D_OUTPUT_MODE_RGB565);
    LL_DMA2D_SetNbrOfPixelsPerLines(DMA2D, width);
    LL_DMA2D_SetNbrOfLines(DMA2D, height);
    LL_DMA2D_FGND_SetLineOffset(DMA2D, 0);
    LL_DMA2D_SetLineOffset(DMA2D, 800 - width);
    LL_DMA2D_FGND_SetMemAddr(DMA2D, (uint32_t)data);
    LL_DMA2D_SetOutputMemAddr(DMA2D, ((uint32_t)lcd_fb_main + 2 * (sy * 800 + sx)));
    LL_DMA2D_EnableIT_TC(DMA2D);
    LL_DMA2D_Start(DMA2D);
}

void lcd_test(void)
{
#define TEST_MODE 1 // 0:寄存器到内存, 1:内存到内存
    static volatile uint8_t cnt = 0;
    static uint16_t colors[4] = { 0xF800, 0x07E0, 0x001F, 0xFFFF };

    uint8_t index = cnt % 4;
#if TEST_MODE
    for (uint8_t row = 0; row < 4; row++)
    {
        for (int32_t i = 0; i < 320; i++)
        {
            sdram_write_16b_cover((uint32_t)lcd_fb_back + (row * 320 + i) * 800 * 2 + 0 * 2,
                                  colors[(index + row) % 4], 800);
        }
    }
    if (index % 2 == 0)
    {
        lcd_fill_data_sync(0, 0, 800, 1280, (uint16_t *)lcd_fb_back);
    }
    else
    {
        while (lcd_async_done_flag == 0);
        lcd_async_done_flag = 0;
        lcd_fill_data_async(0, 0, 800, 1280, (uint16_t *)lcd_fb_back);
    }
#else
    for (uint8_t row = 0; row < 4; row++)
    {
        if (index % 2 == 0)
        {
            lcd_fill_color_sync(0, row * 320, 800, 320, colors[(index + row) % 4]);
        }
        else
        {
            while (lcd_async_done_flag == 0);
            lcd_async_done_flag = 0;
            lcd_fill_color_async(0, row * 320, 800, 320, colors[(index + row) % 4]);
        }
    }
#endif
    cnt++;
}
