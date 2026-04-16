#include "ws2812.h"

/* ===================== 内部宏定义 ===================== */

/*
 * SPI 速率 = APB1(42MHz) / 16 = 2.625MHz ≈ 2.4MHz（误差在容忍范围内）
 * 每 bit 周期 ≈ 381ns
 *   编码 "0": 0b100 → 高 381ns，低 762ns（WS2812 规格：T0H=400ns T0L=850ns）
 *   编码 "1": 0b110 → 高 762ns，低 381ns（WS2812 规格：T1H=800ns T1L=450ns）
 * Reset 信号 > 50μs：发送若干字节 0x00
 */

#define SPI_BYTE_PER_LED    9U                           // 每颗 LED 需要 9 字节 SPI 数据
#define RESET_BYTES         40U                          // 复位码 ≥ 50μs，40字节 × 3bits = 约 45μs

/* DMA 发送缓冲区：LED数据区 + 复位区 */
#define DMA_BUF_SIZE        (WS2812_LED_NUM * SPI_BYTE_PER_LED + RESET_BYTES)

static uint8_t  s_dmaBuf[DMA_BUF_SIZE];                 // DMA 发送缓冲
static WS2812_Color_t s_ledBuf[WS2812_LED_NUM];         // 颜色逻辑缓冲
static volatile uint8_t s_dmaRunning = 0;               // DMA 忙标志

/* ===================== 内部函数：编码单字节颜色数据 ===================== */
/*
 * 将 1 个颜色字节（8bit）编码为 3 个 SPI 字节（24 SPI bits）
 * 每个 WS2812 bit 用 3 个 SPI bit 表示：
 *   bit=0 → 0b100 = 0x4（在 3bit 内）
 *   bit=1 → 0b110 = 0x6（在 3bit 内）
 * 8 × 3 = 24 SPI bits = 3 字节
 *
 * 打包方式（大端，MSB 先行）：
 *   源字节 bit7~bit0 → 目标 [byte0 bit7:5] [byte0 bit4:2] [byte0 bit1 + byte1 bit7:6] ...
 *
 * 编码表（每3bit组 → 查表）：
 *   000 → 100 100 100 → 0x92  0x49  0x24  (但这里逐 bit 处理更直观)
 */
static void EncodeColorByte(uint8_t colorByte, uint8_t *out)
{
    /*
     * 将 8 个 WS2812 bit 打包进 3 个字节（24 SPI bit）
     * 每个 WS2812 bit → 3 SPI bit：0 = 100, 1 = 110
     * 24 SPI bit 存入 3 字节，大端对齐
     */
    uint32_t spiData = 0;

    for (int8_t i = 7; i >= 0; i--)
    {
        spiData <<= 3;
        if (colorByte & (1 << i))
        {
            spiData |= 0x06;   // 0b110
        }
        else
        {
            spiData |= 0x04;   // 0b100
        }
    }

    /* 将 24bit 存入 3 个字节（大端） */
    out[0] = (uint8_t)(spiData >> 16);
    out[1] = (uint8_t)(spiData >> 8);
    out[2] = (uint8_t)(spiData);
}

/* ===================== 内部函数：刷新 DMA 缓冲区 ===================== */
static void RefreshDmaBuffer(void)
{
    uint8_t *pBuf = s_dmaBuf;

    for (uint16_t i = 0; i < WS2812_LED_NUM; i++)
    {
        /* WS2812 发送顺序：G -> R -> B */
        EncodeColorByte(s_ledBuf[i].G, pBuf);  pBuf += 3;
        EncodeColorByte(s_ledBuf[i].R, pBuf);  pBuf += 3;
        EncodeColorByte(s_ledBuf[i].B, pBuf);  pBuf += 3;
    }

    /* 复位区全部填 0 */
    for (uint16_t i = 0; i < RESET_BYTES; i++)
    {
        *pBuf++ = 0x00;
    }
}

/* ===================== WS2812_Init ===================== */
void WS2812_Init(void)
{
    /* ---------- 1. 使能时钟 ---------- */
    RCC->AHB1ENR  |= RCC_AHB1ENR_GPIOBEN;   // GPIOB
    RCC->AHB1ENR  |= RCC_AHB1ENR_DMA1EN;    // DMA1
    RCC->APB1ENR  |= RCC_APB1ENR_SPI2EN;    // SPI2

    /* ---------- 2. 配置 PB15 为 AF5（SPI2_MOSI） ---------- */
    /* 复用功能模式 */
    GPIOB->MODER   &= ~(3U << (15 * 2));
    GPIOB->MODER   |=  (2U << (15 * 2));    // AF 模式

    /* 高速输出 */
    GPIOB->OSPEEDR |=  (3U << (15 * 2));    // Very High Speed

    /* 推挽输出 */
    GPIOB->OTYPER  &= ~(1U << 15);

    /* 无上下拉 */
    GPIOB->PUPDR   &= ~(3U << (15 * 2));

    /* 选择 AF5（SPI2）: AFRH 寄存器，PB15 对应 AFRH[3:0] */
    GPIOB->AFR[1]  &= ~(0xFU << ((15 - 8) * 4));
    GPIOB->AFR[1]  |=  (5U   << ((15 - 8) * 4));  // AF5 = SPI2

    /* ---------- 3. 配置 SPI2 ---------- */
    SPI2->CR1 = 0;
    SPI2->CR2 = 0;

    /*
     * APB1 = 42MHz
     * BR[2:0] = 011 → 分频 /16 → SPI CLK = 42/16 = 2.625 MHz
     * MSTR=1（主模式），SSM=1，SSI=1（软件 NSS），
     * CPOL=0，CPHA=0（模式0），DFF=0（8bit），LSBFIRST=0（MSB先行）
     */
    SPI2->CR1 = SPI_CR1_MSTR
              | SPI_CR1_SSM
              | SPI_CR1_SSI
              | (3U << 3)     // BR = 011 → /16
              | SPI_CR1_SPE;               // 使能 SPI

    /* 使能 SPI TX DMA 请求 */
    SPI2->CR2 |= SPI_CR2_TXDMAEN;

    /* ---------- 4. 配置 DMA1 Stream4 Channel0（SPI2_TX） ---------- */
    DMA1_Stream4->CR = 0;

    /* 等待 DMA Stream 关闭 */
    while (DMA1_Stream4->CR & DMA_SxCR_EN);

    DMA1_Stream4->CR  = (0U << 25)  // Channel 0
                      | DMA_SxCR_MINC                // 内存地址自增
                      | (1U << 6)     // 内存 → 外设
                      | DMA_SxCR_TCIE;               // 传输完成中断

    DMA1_Stream4->PAR  = (uint32_t)&SPI2->DR;        // 外设地址：SPI2 数据寄存器
    DMA1_Stream4->M0AR = (uint32_t)s_dmaBuf;         // 内存地址：DMA 缓冲区
    DMA1_Stream4->NDTR = DMA_BUF_SIZE;               // 传输字节数

    /* ---------- 5. 使能 DMA1 Stream4 传输完成中断 ---------- */
    NVIC_SetPriority(DMA1_Stream4_IRQn, 2);
    NVIC_EnableIRQ(DMA1_Stream4_IRQn);

    /* ---------- 6. 清空 LED 缓冲 ---------- */
    for (uint16_t i = 0; i < WS2812_LED_NUM; i++)
    {
        s_ledBuf[i] = COLOR_OFF;
    }

    s_dmaRunning = 0;
}

/* ===================== WS2812_SetColor ===================== */
void WS2812_SetColor(uint16_t index, WS2812_Color_t color)
{
    if (index < WS2812_LED_NUM)
    {
        s_ledBuf[index] = color;
    }
}

/* ===================== WS2812_SetAll ===================== */
void WS2812_SetAll(WS2812_Color_t color)
{
    for (uint16_t i = 0; i < WS2812_LED_NUM; i++)
    {
        s_ledBuf[i] = color;
    }
}

/* ===================== WS2812_Show ===================== */
void WS2812_Show(void)
{
    /* 等待上一次 DMA 传输完成 */
    while (s_dmaRunning);

    /* 将逻辑颜色缓冲编码进 DMA 缓冲 */
    RefreshDmaBuffer();

    s_dmaRunning = 1;

    /* 清除 DMA1 Stream4 标志位 */
    DMA1->HIFCR = DMA_HIFCR_CTCIF4
                | DMA_HIFCR_CHTIF4
                | DMA_HIFCR_CTEIF4
                | DMA_HIFCR_CDMEIF4
                | DMA_HIFCR_CFEIF4;

    /* 重新配置 DMA 并启动 */
    DMA1_Stream4->CR  &= ~DMA_SxCR_EN;
    while (DMA1_Stream4->CR & DMA_SxCR_EN);

    DMA1_Stream4->M0AR = (uint32_t)s_dmaBuf;
    DMA1_Stream4->NDTR = DMA_BUF_SIZE;
    DMA1_Stream4->CR  |= DMA_SxCR_EN;
}

/* ===================== WS2812_Clear ===================== */
void WS2812_Clear(void)
{
    WS2812_SetAll(COLOR_OFF);
    WS2812_Show();
}

/* ===================== DMA1 Stream4 中断处理函数 ===================== */
void DMA1_Stream4_IRQHandler(void)
{
    if (DMA1->HISR & DMA_HISR_TCIF4)
    {
        /* 清除传输完成标志 */
        DMA1->HIFCR = DMA_HIFCR_CTCIF4;
        s_dmaRunning = 0;
    }
}
