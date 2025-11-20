#include <drv.h>

/* 定义SDRAM设备信息 */
#define SDRAM_ADDR ((uint32_t)0xC0000000)
#define SDRAM_SIZE ((uint32_t)(32 * 1024 * 1024))

/* 定义SDRAM可选参数 */
#define SDRAM_BURST_LEN_1                ((uint16_t)0x0000)
#define SDRAM_BURST_LEN_2                ((uint16_t)0x0001)
#define SDRAM_BURST_LEN_4                ((uint16_t)0x0002)
#define SDRAM_BURST_LEN_8                ((uint16_t)0x0004)
#define SDRAM_BURST_TYPE_SEQUENTIAL      ((uint16_t)0x0000)
#define SDRAM_BURST_TYPE_INTERLEAVED     ((uint16_t)0x0008)
#define SDRAM_CAS_LATENCY_2              ((uint16_t)0x0020)
#define SDRAM_CAS_LATENCY_3              ((uint16_t)0x0030)
#define SDRAM_OPERATING_MODE_STANDARD    ((uint16_t)0x0000)
#define SDRAM_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000)
#define SDRAM_WRITEBURST_MODE_SINGLE     ((uint16_t)0x0200)

static void sdram_send_cmd(uint32_t cmd_mode, uint32_t bank, uint32_t refresh_cnt, uint32_t reg_val)
{
    FMC_SDRAM_CommandTypeDef cmd;
    cmd.CommandMode = cmd_mode;
    if (bank == 1)
    {
        cmd.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
    }
    else if (bank == 2)
    {
        cmd.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK2;
    }
    cmd.AutoRefreshNumber = refresh_cnt;
    cmd.ModeRegisterDefinition = reg_val;
    HAL_SDRAM_SendCommand(&hsdram1, &cmd, 10);
}

void sdram_init(void)
{
    /* 时钟使能命令 */
    sdram_send_cmd(FMC_SDRAM_CMD_CLK_ENABLE, 1, 1, 0);
    LL_mDelay(1);

    /* SDRAM全部预充电命令 */
    sdram_send_cmd(FMC_SDRAM_CMD_PALL, 1, 1, 0);

    /* 自动刷新命令 */
    sdram_send_cmd(FMC_SDRAM_CMD_AUTOREFRESH_MODE, 1, 8, 0);

    /* 配置SDRAM模式寄存器 */
    uint32_t temp;
    temp = (uint32_t)SDRAM_BURST_LEN_1 |   // 设置突发长度：1
           SDRAM_BURST_TYPE_SEQUENTIAL |   // 设置突发类型：连续
           SDRAM_CAS_LATENCY_3 |           // 设置CL值：3
           SDRAM_OPERATING_MODE_STANDARD | // 设置操作模式：标准
           SDRAM_WRITEBURST_MODE_SINGLE;   // 设置突发写模式：单点访问
    sdram_send_cmd(FMC_SDRAM_CMD_LOAD_MODE, 1, 1, temp);

    /* 设置自刷新频率 */
    /* SDRAM refresh period / Number of rows）*SDRAM时钟速度 – 20
     * = 64000(64ms) / 8192 * 120MHz - 20
     */
    HAL_SDRAM_ProgramRefreshRate(&hsdram1, 918);
    LL_mDelay(1);

    sdram_write_cover(SDRAM_ADDR, 0xFF, SDRAM_SIZE);
}

uint32_t sdram_get_addr_head(void)
{
    return SDRAM_ADDR;
}

uint32_t sdram_get_addr_size(void)
{
    return SDRAM_SIZE;
}

void sdram_write_byte(uint32_t addr, uint8_t byte)
{
    assert_param(((addr >= SDRAM_ADDR) && (addr < (SDRAM_ADDR + SDRAM_SIZE))));

    volatile uint8_t *p_sdram = (uint8_t *)addr;
    *p_sdram = byte;
}

void sdram_write_cover(uint32_t addr, uint8_t byte, uint32_t size)
{
    assert_param(((addr >= SDRAM_ADDR) && (addr < (SDRAM_ADDR + SDRAM_SIZE))));
    assert_param((addr + size) <= (SDRAM_ADDR + SDRAM_SIZE));

    volatile uint8_t *p_sdram = (uint8_t *)addr;
    for (uint32_t i = 0; i < size; i++)
    {
        *p_sdram++ = byte;
    }
}

void sdram_write_byte_stream(uint32_t addr, uint8_t *data, uint32_t size)
{
    assert_param(((addr >= SDRAM_ADDR) && (addr < (SDRAM_ADDR + SDRAM_SIZE))));
    assert_param(data != NULL);
    assert_param((addr + size) <= (SDRAM_ADDR + SDRAM_SIZE));

    volatile uint8_t *p_sdram = (uint8_t *)addr;
    for (uint32_t i = 0; i < size; i++)
    {
        *p_sdram++ = data[i];
    }
}

uint8_t sdram_test(void)
{
#define TEST_SIZE 256
#if TEST_SIZE > 256
#warning "TES_SIZE must be less than or equal to 256"
#endif
    volatile uint8_t *sdram_data = (uint8_t *)SDRAM_ADDR;

    sdram_write_cover(SDRAM_ADDR, 0xFF, SDRAM_SIZE);
    SCB_InvalidateDCache_by_Addr((void *)sdram_data, TEST_SIZE);
    for (uint32_t i = 0; i < TEST_SIZE; i++)
    {
        if (sdram_data[i] != 0xFF)
        {
            return 1;
        }
    }

    for (uint32_t i = 0; i < TEST_SIZE; i++)
    {
        sdram_write_byte((uint32_t)sdram_data + i, i % TEST_SIZE);
    }
    SCB_CleanDCache_by_Addr((uint32_t *)sdram_data, TEST_SIZE);
    SCB_InvalidateDCache_by_Addr((void *)sdram_data, TEST_SIZE);
    for (uint32_t i = 0; i < TEST_SIZE; i++)
    {
        if (sdram_data[i] != (i % TEST_SIZE))
        {
            return 2;
        }
    }

    uint8_t test_data[TEST_SIZE];
    for (uint32_t i = 0; i < TEST_SIZE; i++)
    {
        test_data[i] = (TEST_SIZE - 1) - (i % TEST_SIZE);
    }
    SCB_CleanDCache_by_Addr((uint32_t *)test_data, TEST_SIZE);
    sdram_write_byte_stream((uint32_t)sdram_data, test_data, TEST_SIZE);
    SCB_InvalidateDCache_by_Addr((void *)sdram_data, TEST_SIZE);
    for (uint32_t i = 0; i < TEST_SIZE; i++)
    {
        if (sdram_data[i] != (TEST_SIZE - 1) - (i % TEST_SIZE))
        {
            return 3;
        }
    }

    return 0;
}
