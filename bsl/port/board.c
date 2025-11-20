#include <board.h>

void print(lv_log_level_t level, const char *buf)
{
    if (level >= LV_LOG_LEVEL)
    {
        while (*buf != '\0')
        {
            while (!LL_USART_IsActiveFlag_TXE(USART2));
            LL_USART_TransmitData8(USART2, *buf++);
        }
    }
}
