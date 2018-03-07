#include "FreeRtOS.h"
#include "task.h"
#include "portmacro.h"
#include "led.h"
#include "TskLed.h"
static const char TskLed0Name[7] = "TskLed0";
static unsigned short TskLed0StackDepth = 128;						//�����ջ��ȣ���λΪ�ֽ�(word)
static UBaseType_t TskLed0Pri = 5;									//�������ȼ�,0Ϊ���
static TaskHandle_t TskLed0Handler;									//������

static const char TskLed1Name[7] = "TskLed1";
static unsigned short TskLed1StackDepth = 128;						//�����ջ��ȣ���λΪ�ֽ�(word)
static UBaseType_t TskLed1Pri = 5;									//�������ȼ�,0Ϊ���
static TaskHandle_t TskLed1Handler;		
//������
void TskLed0(void *arg)
{
	//����LED0
	LED0 = 1;
	while(1)
	{
		vTaskDelay(500);
		LED0 = ~LED0;
	}
}
void TskLed1(void *arg)
{
	//����LED1
	LED1 = 1;
	while(1)
	{
		vTaskDelay(200);
		LED1 = 0;
		vTaskDelay(800);
		LED1 = 1;
	}
}
int TskLedCreate(void)
{
	BaseType_t err = xTaskCreate(TskLed0, TskLed0Name, TskLed0StackDepth, (void *)NULL, 
		TskLed0Pri, (TaskHandle_t *)&TskLed0Handler);
	
	err = xTaskCreate(TskLed1, TskLed1Name, TskLed1StackDepth, (void *)NULL, 
		TskLed1Pri, (TaskHandle_t *)&TskLed1Handler);
	
	return 0;
}
