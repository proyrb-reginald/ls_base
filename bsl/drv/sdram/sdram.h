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
 * @brief 向指定地址写入单字节数据
 * @param addr 首地址
 * @param data 8b数值
 */
extern void sdram_write_8b(uint32_t addr, uint8_t data);

/**
 * @brief 向指定地址用指定单字节数据覆盖
 * @param addr 首地址
 * @param data 8b数值
 * @param size 8b数量
 */
extern void sdram_write_8b_cover(uint32_t addr, uint8_t data, uint32_t size);

/**
 * @brief 向指定地址写入单字节流数据
 * @param addr 首地址
 * @param data 8b数据
 * @param size 8b数量
 */
extern void sdram_write_8b_stream(uint32_t addr, uint8_t *data, uint32_t size);

/**
 * @brief 向指定地址写入双字节数据
 * @param addr 首地址
 * @param data 16b数值
 */
extern void sdram_write_16b(uint32_t addr, uint16_t data);

/**
 * @brief 向指定地址用指定双字节数据覆盖
 * @param addr 首地址
 * @param data 16b数值
 * @param size 16b数量
 */
extern void sdram_write_16b_cover(uint32_t addr, uint16_t data, uint32_t size);

/**
 * @brief 向指定地址写入双字节流数据
 * @param addr 首地址
 * @param data 16b数据
 * @param size 16b数量
 */
extern void sdram_write_16b_stream(uint32_t addr, uint16_t *data, uint32_t size);

/**
 * @brief 测试SDRAM
 * @retval 0 功能正常
 * @retval 1 8b覆盖读写出错
 * @retval 2 8b单字节读写出错
 * @retval 3 8b单字节流读写出错
 * @retval 4 16b覆盖读写出错
 * @retval 5 16b双字节读写出错
 * @retval 6 16b双字节流读写出错
 */
extern uint8_t sdram_test(void);

#endif
