#include "sys.h"	
#include "delay.h"	
#include "led.h" 
#include "key.h"
#include "usmart.h"
#include "usart.h"
#include "malloc.h"
#include "sram.h"
#include "lwip_comm.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lcd.h"
#include "TskLed.h"
#include "TskLCD.h"
#include "tcp_server_demo.h"

//��LCD����ʾ��ַ��Ϣ����
//�������ȼ�
#define DISPLAY_TASK_PRIO	(tskIDLE_PRIORITY + 2) 
//�����ջ��С
#define DISPLAY_STK_SIZE	128
//�����ջ
StackType_t	DISPLAY_TASK_STK[DISPLAY_STK_SIZE];
//������ƿ�
StaticTask_t DISPLAY_TASK_BUFF;
//��������
const char *DISPLAY_TASK_NAME = "task display";
//������
void display_task(void *pdata);

//START����
//�������ȼ�
#define START_TASK_PRIO		(tskIDLE_PRIORITY + 1) 
//�����ջ��С
#define START_STK_SIZE		128
//�����ջ
StackType_t	START_TASK_STK[DISPLAY_STK_SIZE];
//������ƿ�
StaticTask_t START_TASK_BUFF;
//��������
const char *START_TASK_NAME = "task start";
//������
void start_task(void *pdata);

void show_address(u8 mode)
{
	u8 buf[30];
	if(mode==2)
	{
		sprintf((char*)buf,"DHCP IP :%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);						//��ӡ��̬IP��ַ
		LCD_ShowString(30,260,260,24,24,buf); 
		sprintf((char*)buf,"DHCP GW :%d.%d.%d.%d",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);	//��ӡ���ص�ַ
		LCD_ShowString(30,290,290,24,24,buf); 
		sprintf((char*)buf,"NET MASK:%d.%d.%d.%d",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);	//��ӡ���������ַ
		LCD_ShowString(30,320,320,24,24,buf); 
		LCD_ShowString(30,350,210,24,24,(u8 *)"Port:8088!"); 
	}
	else 
	{
		sprintf((char*)buf,"Static IP:%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);						//��ӡ��̬IP��ַ
		LCD_ShowString(30,260,260,24,24,buf); 
		sprintf((char*)buf,"Static GW:%d.%d.%d.%d",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);	//��ӡ���ص�ַ
		LCD_ShowString(30,290,290,24,24,buf);  
		sprintf((char*)buf,"NET MASK:%d.%d.%d.%d",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);	//��ӡ���������ַ
		LCD_ShowString(30,320,320,24,24,buf); 
		LCD_ShowString(30,350,210,24,24,(u8 *)"Port:8088!"); 
	}	
}

int main(void)
{
	delay_init();	    	 //��ʱ������ʼ��	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);//�����ж����ȼ�����Ϊ��4��0λ��ռ���ȼ���0λ��Ӧ���ȼ�	
	uart_init(115200);	 	//���ڳ�ʼ��Ϊ115200
	LED_Init();		  	 	//��ʼ����LED���ӵ�Ӳ���ӿ�
	LCD_Init();				//LCD��ʼ��
	KEY_Init();	 			//��ʼ������
 	usmart_dev.init(72);	//��ʼ��USMART
	FSMC_SRAM_Init();		//��ʼ���ⲿSRAM
	my_mem_init(SRAMIN);         //��ʼ���ڲ��ڴ��
	my_mem_init(SRAMEX);		//��ʼ���ⲿ�ڴ��
	POINT_COLOR = RED;
	LCD_ShowString(30,30,200,24,24,(u8 *)"WARSHIP STM32F103");
	LCD_ShowString(30,60,200,24,24,(u8 *)"LWIP+FreeRTOS Test");
	LCD_ShowString(30,90,200,24,24,(u8 *)"XAOWANG");
	LCD_ShowString(30,120,200,20,24,(u8 *)"2018/1/28");
//	POINT_COLOR = BLUE; 	//��ɫ����
	while(lwip_comm_init()) //lwip��ʼ��
	{
		LCD_ShowString(30,200,200,20,24,(u8 *)"Lwip Init failed!"); 	//lwip��ʼ��ʧ��
		delay_ms(500);
		LCD_Fill(30,200,230,230,BLACK);
		delay_ms(500);
	}
	LCD_ShowString(30,200,200,20,24,(u8 *)"Lwip Init Success!"); 		//lwip��ʼ���ɹ�
	while(tcp_server_init() != pdPASS) 									//��ʼ��tcp_client(����tcp_client�߳�)
	{
		LCD_ShowString(30,230,200,20,24,(u8 *)"TCP Server failed!!"); //tcp�ͻ��˴���ʧ��
		delay_ms(500);
		LCD_Fill(30,230,230,260,WHITE);
		delay_ms(500);
	}
	LCD_ShowString(30,230,230,20,24,(u8 *)"TCP Server Success!"); 			//TCP�����ɹ�
//	
//	TskLCDCreate();
//	TskLedCreate();			//����Led����
	xTaskCreateStatic(start_task,
					  START_TASK_NAME,
					  (uint32_t)START_STK_SIZE,
					 (void*)NULL,
					  (UBaseType_t)START_TASK_PRIO,
					 (StackType_t *const)START_TASK_STK,
					 (StaticTask_t *const)&START_TASK_BUFF);		 //��ʼ����
	vTaskStartScheduler();          //�����������
	return 0;
}

//start����
void start_task(void *pdata)
{

//	pdata = pdata ;
	TskLedCreate();													//����Led����						
	xTaskCreateStatic(display_task,
					  DISPLAY_TASK_NAME,
					  (uint32_t)DISPLAY_STK_SIZE,
					 (void*)NULL,
					  (UBaseType_t)DISPLAY_TASK_PRIO,
					 (StackType_t *const)DISPLAY_TASK_STK,
					 (StaticTask_t *const)&DISPLAY_TASK_BUFF);		 //��ʾ����
	vTaskSuspend(NULL); 											//����start_task����

}

//��ʾ��ַ����Ϣ
void display_task(void *pdata)
{
	while(1)
	{ 
#if LWIP_DHCP									//������DHCP��ʱ��
		if(lwipdev.dhcpstatus != 0) 			//����DHCP
		{
			show_address(lwipdev.dhcpstatus );	//��ʾ��ַ��Ϣ
			vTaskSuspend(NULL);  		//��ʾ���ַ��Ϣ�������������
		}
#else
		show_address(0); 						//��ʾ��̬��ַ
		vTaskSuspend(NULL);  					//��ʾ���ַ��Ϣ�������������
#endif //LWIP_DHCP
		vTaskDelay(100);
	}
}