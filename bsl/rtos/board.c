#include <board.h>

void output(const char *str)
{
    while (*str != '\0')
    {
        while (!LL_USART_IsActiveFlag_TXE(USART2));
        LL_USART_TransmitData8(USART2, *str++);
    }
}
