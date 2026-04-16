#ifndef __WS2812_H
#define __WS2812_H

#include "stm32f4xx.h"
#include <stdint.h>

/* ===================== 用户配置区 ===================== */
#define WS2812_LED_NUM      16       // 灯带 LED 总数量，按需修改

/* ===================== 颜色结构体 ===================== */
typedef struct {
    uint8_t G;   // WS2812 顺序为 G -> R -> B
    uint8_t R;
    uint8_t B;
} WS2812_Color_t;

/* ===================== 常用颜色宏 ===================== */
#define COLOR_RED       (WS2812_Color_t){0,   255, 0  }
#define COLOR_GREEN     (WS2812_Color_t){255, 0,   0  }
#define COLOR_BLUE      (WS2812_Color_t){0,   0,   255}
#define COLOR_WHITE     (WS2812_Color_t){255, 255, 255}
#define COLOR_OFF       (WS2812_Color_t){0,   0,   0  }
#define COLOR_YELLOW    (WS2812_Color_t){255, 255, 0  }
#define COLOR_CYAN      (WS2812_Color_t){255, 0,   255}
#define COLOR_MAGENTA   (WS2812_Color_t){0,   255, 255}

/* ===================== API 函数声明 ===================== */

/**
 * @brief  初始化 WS2812（SPI2 + DMA1，PB15 输出）
 */
void WS2812_Init(void);

/**
 * @brief  设置单颗 LED 颜色（不立即刷新）
 * @param  index: LED 索引，从 0 开始
 * @param  color: 颜色
 */
void WS2812_SetColor(uint16_t index, WS2812_Color_t color);

/**
 * @brief  设置所有 LED 为同一颜色（不立即刷新）
 * @param  color: 颜色
 */
void WS2812_SetAll(WS2812_Color_t color);

/**
 * @brief  将缓冲区数据通过 DMA 发送到灯带（刷新显示）
 */
void WS2812_Show(void);

/**
 * @brief  清除所有 LED（熄灭），并刷新
 */
void WS2812_Clear(void);

#endif /* __WS2812_H */
