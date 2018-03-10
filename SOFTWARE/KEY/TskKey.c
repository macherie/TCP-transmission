#include "FreeRtOS.h"
#include "task.h"
#include "portmacro.h"
#include "key.h"
#include "lcd.h"
#include "stmflash.h"
#include "TskKey.h"
static const char TskKeyName[7] = "TskKey";
static unsigned short TskKeyStackDepth = 128;						//�����ջ��ȣ���λΪ�ֽ�(word)
static UBaseType_t TskKeyPri = 5;									//�������ȼ�,0Ϊ���
static TaskHandle_t TskKeyHandler;									//������
	
	
//Ҫд�뵽STM32 FLASH���ַ�������
const u8 TEXT_Buffer[]={"STM32F103 FLASH TEST"};
#define SIZE sizeof(TEXT_Buffer)		//���鳤��
#define FLASH_SAVE_ADDR  0X08014000		//����FLASH �����ַ80K(����Ϊż��������ֵҪ���ڱ�������ռ��FLASH�Ĵ�С+0X08000000)

//������
void TskKey(void *arg)
{
	u8 key;
	u8 datatemp[SIZE];
	while(1)
	{
		key=KEY_Scan(0);
		if(key==KEY1_PRES)	//KEY1����,д��STM32 FLASH
		{
//			LCD_Fill(30,400,300,500,BLACK);//������� 	
			LCD_ShowStringColor(30,430,240,24,24,datatemp, BLACK);		//�����������ַ���
			
 			LCD_ShowString(30,400,360,24,24,(u8 *)"Start Write FLASH....");
			STMFLASH_Write(FLASH_SAVE_ADDR,(u16*)TEXT_Buffer,SIZE);
			LCD_ShowString(30,400,270,24,24,(u8 *)"FLASH Write Finished!");//��ʾ�������
		}
		if(key==KEY0_PRES)	//KEY0����,��ȡ�ַ�������ʾ
		{		
 			LCD_ShowString(30,400,240,24,24,(u8 *)"Start Read FLASH.... ");
			STMFLASH_Read(FLASH_SAVE_ADDR,(u16*)datatemp,SIZE);
			LCD_ShowString(30,400,270,24,24,(u8 *)"The Data Readed Is:  ");//��ʾ�������
			LCD_ShowString(30,430,240,24,24,datatemp);//��ʾ�������ַ���
		}
		vTaskDelay(10);		   
	} 
}

int TskKeyCreate(void)
{
	BaseType_t err = xTaskCreate(TskKey, TskKeyName, TskKeyStackDepth, (void *)NULL, 
		TskKeyPri, (TaskHandle_t *)&TskKeyHandler);
	
	return 0;
}
