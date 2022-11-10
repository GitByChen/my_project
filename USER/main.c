#include "FreeRTOS.h"
#include "task.h"
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "rtc.h"
#include "pwm.h"
#include "wdg.h"
#include "w25q64.h"


TaskHandle_t StartTaskHandler;		//������
int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);//����ϵͳ�ж����ȼ�����4
    W25QXX_Init();   
	delay_init();	    				//��ʱ������ʼ��  
    RTC_Init();	  			//RTC��ʼ��	
	Uart1_Init(9600);					//��ʼ������
    Uart2_Init(9600);					//��ʼ������
	//LED_Init();		  					//��ʼ��LED	
     Flash_Data_Init();	
    TIM3_PWM_Init(999,0);	//����Ƶ��pwmƵ��Ϊ72000000/1000=72KHZ
    IWDG_Init(4,625);    //���Ƶ��Ϊ64,����ֵΪ625,���ʱ��Ϊ1s	 Tout=((4��2^prer) ��rlr) /40
	Start_Task_Init();
	
}
void Start_Task_Init(void)
{
	xTaskCreate((	TaskFunction_t) start_task,				//������
							(const char * 	)"start_task",			//��������
							( uint16_t 			)START_STK_SIZE,		//�����ջ��
							(void * 				)NULL ,							//���뺯������
							(UBaseType_t 		)START_TASK_PRIO,		//�������ȼ�
							(TaskHandle_t * )&StartTaskHandler );//ȡַ
								
    vTaskStartScheduler();          //�����������

}
void start_task(void* pvParameters )
{
	
	taskENTER_CRITICAL();           //�����ٽ���
	Rtc_Task_Init();
	Uart_Task_Init();
	Wdg_Task_Init();								
	vTaskDelete(StartTaskHandler);//ɾ������	
	taskEXIT_CRITICAL();            //�˳��ٽ���							
}







