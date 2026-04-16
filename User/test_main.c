#include "stm32f4xx.h"
#include "./lcd/bsp_ili9341_lcd.h"
#include "./lcd/bsp_xpt2046_lcd.h"
#include "./led/bsp_led.h"
#include "./usart/bsp_debug_usart.h"
#include "./deng/ws2812.h"
#include <stdio.h>

#include "delay.h"
#include "sys.h"


static void LCD_Test(void);	
static void Delay ( __IO uint32_t nCount );
void Printf_Charater(void)   ;

float Motor_RPM = 0.0f; // ȫ �� �� ���� �� �� �� �� �� ʵ ת ��(RPM)

//����������Ժ���
void TIM1_PWM_Init(u32 arr, u32 psc)    
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    TIM_OCInitTypeDef  TIM_OCInitStructure;

    // 1. ʹ�� TIM1 �� GPIOA ��ʱ��
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);      
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);   

    // 2. ���� PA8~PA11 �ĸ��ù���Ϊ TIM1
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_TIM1);     //ʹ�����
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_TIM1); 
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_TIM1); 
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource11, GPIO_AF_TIM1); 

    // 3. ��ʼ�� GPIOA
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11;   
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;        // ���ù���
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;  // �ٶ�100MHz
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      // ���츴�����
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;        // ����
    GPIO_Init(GPIOA, &GPIO_InitStructure);              // ��ʼ��PA��

    // 4. ��ʼ�� TIM1 ����ʱ��
    TIM_TimeBaseStructure.TIM_Period = arr; 
    TIM_TimeBaseStructure.TIM_Prescaler = psc; 
    TIM_TimeBaseStructure.TIM_ClockDivision = 1; 
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure); 

    // 5. ��ʼ�� TIM1 PWM ģʽ
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; 
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; 
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;     
    TIM_OCInitStructure.TIM_Pulse = arr / 2;
    
    TIM_OC1Init(TIM1, &TIM_OCInitStructure); 
    TIM_OC2Init(TIM1, &TIM_OCInitStructure); 
    TIM_OC3Init(TIM1, &TIM_OCInitStructure); 
    TIM_OC4Init(TIM1, &TIM_OCInitStructure); 

    // 6. �߼���ʱ���������ʹ��
    TIM_CtrlPWMOutputs(TIM1, ENABLE);

    // 7. Ԥװ��ʹ��
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable); 
    TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);  
    TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);  
    TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Enable);  
    TIM_ARRPreloadConfig(TIM1, ENABLE); 

    // 8. ʹ��TIM1
    TIM_Cmd(TIM1, ENABLE); 
}

// TB6612 �������������ų�ʼ�� (AIN1 -> PB1, AIN2 -> PB2)
void Motor_Direction_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    // 1. ʹ�� GPIOB ʱ�� (PB1 �� PB2 ���� GPIOB)
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    // 2. ���� PB1 �� PB2 Ϊ�������ģʽ
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;       // ��ͨ���ģʽ
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      // �������
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;  // 100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;        // ����
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // 3. �趨�����ʼ����Ϊ��ת (AIN1=1, AIN2=0)
    // ������ֵ��ת���ˣ��� SetBits �� ResetBits ��������
    GPIO_SetBits(GPIOB, GPIO_Pin_1);      // PB1 ����ߵ�ƽ
    GPIO_ResetBits(GPIOB, GPIO_Pin_2);    // PB2 ����͵�ƽ
}

//��������������
//�� �� �� �� ʱ �� �� ʼ �� (TIM3 - PA6, PA7)
void Encoder_TIM3_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_ICInitTypeDef TIM_ICInitStructure;

	// 1. ʹ �� ʱ ��
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA , ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3 , ENABLE);

	// 2. �� �� PA6, PA7 Ϊ �� �� �� ��
	GPIO_PinAFConfig(GPIOA , GPIO_PinSource6 , GPIO_AF_TIM3);
	GPIO_PinAFConfig(GPIOA , GPIO_PinSource7 , GPIO_AF_TIM3);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //�� �� �� �� Ҫ �� ��
	GPIO_Init(GPIOA , &GPIO_InitStructure);

	// 3. �� ʱ �� �� �� �� ��
	TIM_TimeBaseStructure.TIM_Prescaler = 0; // ������ģʽ����Ƶ
	TIM_TimeBaseStructure.TIM_Period = 65535; // �� �� �� װ �� ֵ(16λ)
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	// 4. �� �� Ϊ �� �� �� ģ ʽ (TI1��TI2˫ �� �� ���� ʵ ��4�� Ƶ)
	TIM_EncoderInterfaceConfig(TIM3,
	TIM_EncoderMode_TI12 ,
	TIM_ICPolarity_Rising ,
	TIM_ICPolarity_Rising);

	// 5. �� �� �� �� �� �� �� ���� ֹ ë �� �� �ţ�
	TIM_ICStructInit(&TIM_ICInitStructure);
	TIM_ICInitStructure.TIM_ICFilter = 10;
	TIM_ICInit(TIM3, &TIM_ICInitStructure);

	// 6. �� �� �� �� �� �� �� ��
	TIM_SetCounter(TIM3, 0);
	TIM_Cmd(TIM3, ENABLE);
}

//�� ʱ �� ȡ �� �� �� ʼ �� (TIM6 - 10ms)
void Timer6_Init(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6 , ENABLE);

	// �� ʱ �� ʱ �� Ϊ 84MHz
	// �� Ƶ �� Ϊ 10kHz (0.1ms/��)�� �� ��100�� = 10ms
	TIM_TimeBaseStructure.TIM_Prescaler = 8400 - 1;
	TIM_TimeBaseStructure.TIM_Period = 100 - 1;
	TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);

	// �� �� �� �� �� ��
	TIM_ITConfig(TIM6, TIM_IT_Update , ENABLE);

	// �� �� NVIC �� �� �� �� ��
	NVIC_InitStructure.NVIC_IRQChannel =
	TIM6_DAC_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	TIM_Cmd(TIM6, ENABLE);
}

// TIM6 �� �� �� �� �� ���� ÿ 10ms ִ �� һ ��
void TIM6_DAC_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM6, TIM_IT_Update) != RESET)
		{
			TIM_ClearITPendingBit(TIM6, TIM_IT_Update); // �� �� �� �� �� ־ λ
			// 1. �� ȡ 10ms �� �� �� �� �� �� ��
			// ǿ ת Ϊ short (int16_t) �� �� ����
			// �� �� �� ת ʱ �� �� �� �� �� �� 0xFFFF��
			// ת �� �� �� �� �� Ϊ -1�� �� �� �� Ϣ �� �� �� ��
			short encoder_count = (short)TIM_GetCounter(TIM3);

			// 2. �� ȡ �� �� �� �� �㣬 Ϊ �� �� 10ms �� ׼ ��
			TIM_SetCounter(TIM3, 0);

			// 3. �� �� ת �� (RPM)
			// �� ʽ: (N / 1040) Ϊ 10ms �� ת �� �� Ȧ ��
			// �� 100 -> 1�� �� Ȧ ���� �� 60 -> 1�� �� Ȧ ��
			// 100 * 60 = 6000
			Motor_RPM = (float)encoder_count * 6000.0f / 1040.0f;
		}
}


/**
  * @brief  ������
  * @param  ��  
  * @retval ��
  */
int main ( void )
{
    //Debug_USART_Config();		 //���봮�ڳ�ʼ����LCD��ʼ��֮ǰ
	ILI9341_Init ();             //LCD ��ʼ��
	WS2812_Init();

    /* ====== 示例1：点亮所有 LED 为红色 ====== */
    WS2812_SetAll(COLOR_RED);
    WS2812_Show();
	// 初始测试：循环显示红/绿/蓝/灭
	// RGB_RED(1);                  //点亮第一个像素为红色，输出在 PB15
	// ϵͳʱ��Ϊ168_0000_00Hz������ arr=999, psc=83��Ƶ�� = 168,000,000 / (1000 * 84) = 2000Hz (2kHz)
    TIM1_PWM_Init(999, 83);		 //�����ʼ��
	Motor_Direction_Init();		 //������ƽӿڳ�ʼ��
	//�� ʼ �� �� �� �� �� �� ��TIM3 + TIM6��
	Encoder_TIM3_Init();
	Timer6_Init();
	
	//printf("\r\n ********** Һ������ʾ����*********** \r\n"); 
	//printf("\r\n ������֧�����ģ���ʾ���ĵĳ�����ѧϰ��һ�� \r\n"); 
	
	//����0��3��5��6 ģʽ�ʺϴ���������ʾ���֣�
	//���Ƽ�ʹ������ģʽ��ʾ����	����ģʽ��ʾ���ֻ��о���Ч��			
	//���� 6 ģʽΪ�󲿷�Һ�����̵�Ĭ����ʾ����  
    ILI9341_GramScan ( 6 );   //��ʾģʽ6
	
	//ws�ƴ����Դ���
	while ( 1 )
	{
		LCD_Test();
		delay_ms(500);
		for (uint16_t i = 0; i < WS2812_LED_NUM; i++)
        {
            WS2812_Clear();                       // 全灭
            WS2812_SetColor(i, COLOR_GREEN);      // 点亮第 i 颗
            WS2812_Show();                        // 刷新
            for (volatile uint32_t d = 0; d < 500000; d++); // 简单延时
        }
    }
		
	}



extern uint16_t lcdid;

/*���ڲ��Ը���Һ���ĺ���*/
void LCD_Test(void)
{
	/*��ʾ��ʾ����*/
	static uint8_t testCNT = 0;	
	char dispBuff[100];
	
	testCNT++;	
	
	LCD_SetFont(&Font8x16);   //��С������
	LCD_SetColors(RED,BLACK); //������ɫ��ɫ��������ɫ��ɫ

  ILI9341_Clear(0,0,LCD_X_LENGTH,LCD_Y_LENGTH);	/* ��������ʾȫ�� */
	/********��ʾ�ַ���ʾ��*******/
  ILI9341_DispStringLine_EN(LINE(0),"BH 3.2 inch LCD para:");
  ILI9341_DispStringLine_EN(LINE(1),"Image resolution:240x320 px");
  if(lcdid == LCDID_ILI9341)
  {
	  printf("LCD ID: 0x%X\r\n", lcdid);
    ILI9341_DispStringLine_EN(LINE(2),"ILI9341 LCD driver");
  }
  else if(lcdid == LCDID_ST7789V)
  {
	  printf("LCD ID: 0x%X\r\n", lcdid);
    ILI9341_DispStringLine_EN(LINE(2),"ST7789V LCD driver");
  }
  ILI9341_DispStringLine_EN(LINE(3),"XPT2046 Touch Pad driver");
  
	/********��ʾ����ʾ��*******/
	LCD_SetFont(&Font16x24);
	LCD_SetTextColor(GREEN);

	/*ʹ��c��׼��ѱ���ת�����ַ���*/
	sprintf(dispBuff,"Count : %d ",testCNT);
  LCD_ClearLine(LINE(4));	/* ����������� */
	
	/*Ȼ����ʾ���ַ������ɣ���������Ҳ����������*/
	ILI9341_DispStringLine_EN(LINE(4),dispBuff);

	/*******��ʾͼ��ʾ��******/
	LCD_SetFont(&Font24x32);
  /* ��ֱ�� */
  
  LCD_ClearLine(LINE(4));/* ����������� */
	LCD_SetTextColor(BLUE);

  ILI9341_DispStringLine_EN(LINE(4),"Draw line:");
  
	LCD_SetTextColor(RED);
  ILI9341_DrawLine(50,170,210,230);  
  ILI9341_DrawLine(50,200,210,240);
  
	LCD_SetTextColor(GREEN);
  ILI9341_DrawLine(100,170,200,230);  
  ILI9341_DrawLine(200,200,220,240);
	
	LCD_SetTextColor(BLUE);
  ILI9341_DrawLine(110,170,110,230);  
  ILI9341_DrawLine(130,200,220,240);
  
  Delay(0xFFFFFF);
  
  ILI9341_Clear(0,16*8,LCD_X_LENGTH,LCD_Y_LENGTH-16*8);	/* ��������ʾȫ�� */
  
  
  /*������*/

  LCD_ClearLine(LINE(4));	/* ����������� */
	LCD_SetTextColor(BLUE);

  ILI9341_DispStringLine_EN(LINE(4),"Draw Rect:");

	LCD_SetTextColor(RED);
  ILI9341_DrawRectangle(50,200,100,30,1);
	
	LCD_SetTextColor(GREEN);
  ILI9341_DrawRectangle(160,200,20,40,0);
	
	LCD_SetTextColor(BLUE);
  ILI9341_DrawRectangle(170,200,50,20,1);
  
  
  Delay(0xFFFFFF);
	
	ILI9341_Clear(0,16*8,LCD_X_LENGTH,LCD_Y_LENGTH-16*8);	/* ��������ʾȫ�� */

  /* ��Բ */
  LCD_ClearLine(LINE(4));	/* ����������� */
	LCD_SetTextColor(BLUE);
	
  ILI9341_DispStringLine_EN(LINE(4),"Draw Cir:");

	LCD_SetTextColor(RED);
  ILI9341_DrawCircle(100,200,20,0);
	
	LCD_SetTextColor(GREEN);
  ILI9341_DrawCircle(100,200,10,1);
	
	LCD_SetTextColor(BLUE);
	ILI9341_DrawCircle(140,200,20,0);

  Delay(0xFFFFFF);
  
  ILI9341_Clear(0,16*8,LCD_X_LENGTH,LCD_Y_LENGTH-16*8);	/* ��������ʾȫ�� */

}


/**
  * @brief  ����ʱ����
  * @param  nCount ����ʱ����ֵ
  * @retval ��
  */	
static void Delay ( __IO uint32_t nCount )
{
  for ( ; nCount != 0; nCount -- );
	
}

