#ifndef SDRAM_H
#define SDRAM_H

#include <interface.h>

/**
 * @brief 初始化SDRAM
 */
extern void sdram_init(void);

/**
 * @brief 返回SDRAM首地址
 */
extern uint32_t sdram_get_addr_head(void);

/**
 * @brief 返回SDRAM地址空间大小
 */
extern uint32_t sdram_get_addr_size(void);

/**
 * @brief 用指定数据覆盖SDRAM
 */
extern void sdram_cover_data(uint8_t value);

/**
 * @brief 向指定地址写入单字节数据
 */
extern void sdram_write_byte(uint32_t addr, uint8_t value);

/**
 * @brief 向指定地址写入字节流数据
 */
extern void sdram_write_stream(uint32_t addr, uint8_t *data, uint32_t size);

/**
 * @brief 测试SDRAM
 * @retval 0 功能正常
 * @retval 1 SDRAM覆盖读写出错
 * @retval 2 SDRAM单字节读写出错
 * @retval 3 SDRAM字节流读写出错
 */
extern uint8_t sdram_test(void);

#endif
