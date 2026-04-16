#ifndef __BSP_XPT2046_LCD_H
#define __BSP_XPT2046_LCD_H

#include "stm32f4xx.h"

/******************************* XPT2046 触摸屏中断引脚定义 ***************************/
/* F407 系列 GPIOF 挂载在 AHB1 总线上 */
#define macXPT2046_EXTI_GPIO_CLK             RCC_AHB1Periph_GPIOF   
#define macXPT2046_EXTI_GPIO_PORT            GPIOF
#define macXPT2046_EXTI_GPIO_PIN             GPIO_Pin_9

/* F4 系列外部中断源配置 */
#define macXPT2046_EXTI_SOURCE_PORT          EXTI_PortSourceGPIOF
#define macXPT2046_EXTI_SOURCE_PIN           EXTI_PinSource9
#define macXPT2046_EXTI_LINE                 EXTI_Line9
#define macXPT2046_EXTI_IRQ                  EXTI9_5_IRQn
#define macXPT2046_EXTI_INT_FUNCTION         EXTI9_5_IRQHandler

#define macXPT2046_EXTI_ActiveLevel          0
#define macXPT2046_EXTI_Read()               GPIO_ReadInputDataBit(macXPT2046_EXTI_GPIO_PORT, macXPT2046_EXTI_GPIO_PIN)

/******************************* XPT2046 触摸屏模拟 SPI 引脚定义 ***************************/
#define macXPT2046_SPI_GPIO_CLK              (RCC_AHB1Periph_GPIOF | RCC_AHB1Periph_GPIOG)

/* 片选 CS */
#define macXPT2046_SPI_CS_PIN                GPIO_Pin_10
#define macXPT2046_SPI_CS_PORT               GPIOF

/* 时钟 CLK */
#define macXPT2046_SPI_CLK_PIN               GPIO_Pin_7
#define macXPT2046_SPI_CLK_PORT              GPIOG

/* 输出 MOSI */
#define macXPT2046_SPI_MOSI_PIN              GPIO_Pin_11
#define macXPT2046_SPI_MOSI_PORT             GPIOF

/* 输入 MISO (已修复原代码中的非法减号和中文字符) */
#define macXPT2046_SPI_MISO_PIN              GPIO_Pin_6
#define macXPT2046_SPI_MISO_PORT             GPIOF

/* 软件模拟 SPI 控制宏 */
#define macXPT2046_CS_ENABLE()               GPIO_ResetBits(macXPT2046_SPI_CS_PORT, macXPT2046_SPI_CS_PIN)
#define macXPT2046_CS_DISABLE()              GPIO_SetBits(macXPT2046_SPI_CS_PORT, macXPT2046_SPI_CS_PIN)    

#define macXPT2046_CLK_HIGH()                GPIO_SetBits(macXPT2046_SPI_CLK_PORT, macXPT2046_SPI_CLK_PIN)    
#define macXPT2046_CLK_LOW()                 GPIO_ResetBits(macXPT2046_SPI_CLK_PORT, macXPT2046_SPI_CLK_PIN) 

#define macXPT2046_MOSI_1()                  GPIO_SetBits(macXPT2046_SPI_MOSI_PORT, macXPT2046_SPI_MOSI_PIN) 
#define macXPT2046_MOSI_0()                  GPIO_ResetBits(macXPT2046_SPI_MOSI_PORT, macXPT2046_SPI_MOSI_PIN)

#define macXPT2046_MISO_READ()               GPIO_ReadInputDataBit(macXPT2046_SPI_MISO_PORT, macXPT2046_SPI_MISO_PIN)

/******************************* XPT2046 触摸屏参数定义 ***************************/
/* 这里的扫描模式应与 ILI9341 驱动中的 LCD_SCAN_MODE 一致 (推荐为 6) */
#define macXPT2046_Coordinate_GramScan       6               
#define macXPT2046_THRESHOLD_CalDiff         2               

#define macXPT2046_CHANNEL_X                 0x90            
#define macXPT2046_CHANNEL_Y                 0xd0 

/******************************* 声明数据类型 ***************************/
typedef struct 
{
   uint16_t x;        
   uint16_t y;
} strType_XPT2046_Coordinate;   

typedef struct 
{
    long double An, Bn, Cn, Dn, En, Fn, Divider;
} strType_XPT2046_Calibration;

typedef struct 
{
    long double dX_X, dX_Y, dX, dY_X, dY_Y, dY;
} strType_XPT2046_TouchPara;

/******************************* 声明外部全局变量 ***************************/
extern volatile uint8_t ucXPT2046_TouchFlag;
extern strType_XPT2046_TouchPara strXPT2046_TouchPara;

/******************************** 触摸屏函数声明 **********************************/
void     XPT2046_Init(void);
uint8_t  XPT2046_Touch_Calibrate(void);
uint8_t  XPT2046_Get_TouchedPoint(strType_XPT2046_Coordinate * displayPtr, strType_XPT2046_TouchPara * para);

#endif /* __BSP_XPT2046_LCD_H */
