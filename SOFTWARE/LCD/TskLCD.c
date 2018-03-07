#include "FreeRtOS.h"
#include "task.h"
#include "portmacro.h"
#include "TskLCD.h"
#include "lcd.h"

static const char TskLCDName[7] = "TskLCD";
static unsigned short TskLCDStackDepth = 512;						//�����ջ��ȣ���λΪ�ֽ�(word)
static UBaseType_t TskLCDPri = 10;									//�������ȼ�,0Ϊ���
static TaskHandle_t TskLCDHandler;									//������


void TskLCD(void *arg)
{
	//LCD��ʼ��
	LCD_Init();
	POINT_COLOR = RED;
//	LCD_Display_Dir(1);								//������Ļ��ʾ����Ϊ����
//	LCD_Scan_Dir(R2L_D2U);									//������ɨ�跽��
	LCD_Clear(BLACK);
	vTaskDelay(1000);
	while(1)
	{
		LCD_ShowString(100, 200, 12*16, 24, 24, (u8 *)"Hello World! ^-^");		//��ʾһ���ַ���
		LCD_ShowString(100, 230, 12*9, 24, 24, (u8 *)"LCD id : ");
		LCD_ShowNum(100+12*9, 230, lcddev.id, 5, 24);
		vTaskDelay(1000);													//����1s
	}
	
}
int TskLCDCreate(void)
{
	taskENTER_CRITICAL();           //�����ٽ���
	xTaskCreate(TskLCD, TskLCDName, TskLCDStackDepth, (void *)NULL, 
		TskLCDPri, (TaskHandle_t *)&TskLCDHandler);
	taskEXIT_CRITICAL();            //�˳��ٽ���
	return 0;
	
}
