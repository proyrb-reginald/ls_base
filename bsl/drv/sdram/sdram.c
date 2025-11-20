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

    sdram_write_8b_cover(SDRAM_ADDR, 0xFF, SDRAM_SIZE);
}

uint32_t sdram_get_addr_head(void)
{
    return SDRAM_ADDR;
}

uint32_t sdram_get_addr_size(void)
{
    return SDRAM_SIZE;
}

void sdram_write_8b(uint32_t addr, uint8_t data)
{
    assert_param(((addr >= SDRAM_ADDR) && (addr < (SDRAM_ADDR + SDRAM_SIZE))));

    volatile uint8_t *p_sdram = (uint8_t *)addr;
    *p_sdram = data;
}

void sdram_write_8b_cover(uint32_t addr, uint8_t data, uint32_t size)
{
    assert_param(((addr >= SDRAM_ADDR) && (addr < (SDRAM_ADDR + SDRAM_SIZE))));
    assert_param((addr + size) <= (SDRAM_ADDR + SDRAM_SIZE));

    volatile uint8_t *p_sdram = (uint8_t *)addr;
    for (uint32_t i = 0; i < size; i++)
    {
        *p_sdram++ = data;
    }
}

void sdram_write_8b_stream(uint32_t addr, uint8_t *data, uint32_t size)
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

void sdram_write_16b(uint32_t addr, uint16_t data)
{
    assert_param(((addr >= SDRAM_ADDR) && (addr < (SDRAM_ADDR + SDRAM_SIZE))));

    volatile uint8_t *p_sdram = (uint8_t *)addr;
    *p_sdram++ = data & 0x00FF;
    *p_sdram = (data >> 8) & 0x00FF;
}

void sdram_write_16b_cover(uint32_t addr, uint16_t data, uint32_t size)
{
    assert_param(((addr >= SDRAM_ADDR) && (addr < (SDRAM_ADDR + SDRAM_SIZE))));
    assert_param((addr + size) <= (SDRAM_ADDR + SDRAM_SIZE));

    volatile uint8_t *p_sdram = (uint8_t *)addr;
    for (uint32_t i = 0; i < size; i++)
    {
        *p_sdram++ = data & 0x00FF;
        *p_sdram++ = (data >> 8) & 0x00FF;
    }
}

void sdram_write_16b_stream(uint32_t addr, uint16_t *data, uint32_t size)
{
    assert_param(((addr >= SDRAM_ADDR) && (addr < (SDRAM_ADDR + SDRAM_SIZE))));
    assert_param(data != NULL);
    assert_param((addr + size) <= (SDRAM_ADDR + SDRAM_SIZE));

    volatile uint8_t *p_sdram = (uint8_t *)addr;
    for (uint32_t i = 0; i < size; i++)
    {
        *p_sdram++ = data[i] & 0x00FF;
        *p_sdram++ = (data[i] >> 8) & 0x00FF;
    }
}

uint8_t sdram_test(void)
{
    {
#define TEST_SIZE 256
#if TEST_SIZE > 256
#warning "TES_SIZE must be less than or equal to 256"
#endif
        volatile uint8_t *sdram_data = (uint8_t *)SDRAM_ADDR;

        sdram_write_8b_cover(SDRAM_ADDR, 0x00, SDRAM_SIZE);
        SCB_InvalidateDCache_by_Addr((void *)sdram_data, TEST_SIZE);
        for (uint32_t i = 0; i < TEST_SIZE; i++)
        {
            if (sdram_data[i] != (uint8_t)0x00)
            {
                return 1;
            }
        }

        for (uint32_t i = 0; i < TEST_SIZE; i++)
        {
            sdram_write_8b((uint32_t)sdram_data + i, i % TEST_SIZE);
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
        sdram_write_8b_stream((uint32_t)sdram_data, test_data, TEST_SIZE);
        SCB_InvalidateDCache_by_Addr((void *)sdram_data, TEST_SIZE);
        for (uint32_t i = 0; i < TEST_SIZE; i++)
        {
            if (sdram_data[i] != (TEST_SIZE - 1) - (i % TEST_SIZE))
            {
                return 3;
            }
        }
#undef TEST_SIZE
    }

    {
#define TEST_SIZE 128
#if TEST_SIZE > 128
#warning "TES_SIZE must be less than or equal to 128"
#endif
        volatile uint8_t * const sdram_data = (uint8_t *)SDRAM_ADDR;

        sdram_write_16b_cover(SDRAM_ADDR, 0x0000, SDRAM_SIZE / 2);
        SCB_InvalidateDCache_by_Addr((void *)sdram_data, TEST_SIZE * 2);
        for (uint32_t i = 0; i < TEST_SIZE; i++)
        {
            if ((sdram_data[i] != (uint8_t)0x00) || (sdram_data[i + 1] != (uint8_t)0x00))
            {
                return 4;
            }
        }

        for (uint32_t i = 0; i < TEST_SIZE; i++)
        {
            sdram_write_16b((uint32_t)sdram_data + i * 2, i);
        }
        SCB_CleanDCache_by_Addr((uint32_t *)sdram_data, TEST_SIZE * 2);
        SCB_InvalidateDCache_by_Addr((void *)sdram_data, TEST_SIZE * 2);
        for (uint32_t i = 0; i < TEST_SIZE; i++)
        {
            if ((sdram_data[i * 2] != (uint8_t)(i & 0x00FF)) ||
                (sdram_data[i * 2 + 1] != (uint8_t)((i >> 8) & 0x00FF)))
            {
                return 5;
            }
        }

        uint16_t test_data[TEST_SIZE];
        for (uint32_t i = 0; i < TEST_SIZE; i++)
        {
            test_data[i] = (TEST_SIZE - 1) - i;
        }
        SCB_CleanDCache_by_Addr((uint32_t *)test_data, TEST_SIZE);
        sdram_write_16b_stream((uint32_t)sdram_data, test_data, TEST_SIZE);
        SCB_InvalidateDCache_by_Addr((void *)sdram_data, TEST_SIZE);
        for (uint32_t i = 0; i < TEST_SIZE; i++)
        {
            if ((sdram_data[i * 2] != (uint8_t)(((TEST_SIZE - 1) - i) & 0x00FF)) ||
                (sdram_data[i * 2 + 1] != (uint8_t)((((TEST_SIZE - 1) - i) >> 8) & 0x00FF)))
            {
                return 6;
            }
        }
#undef TEST_SIZE
#undef TEST_DEF_DATA
    }

    return 0;
}
