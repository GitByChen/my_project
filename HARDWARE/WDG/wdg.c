#include "wdg.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
TaskHandle_t StartWdgHandler;		//������

//��ʼ���������Ź�
//prer:��Ƶ��:0~7(ֻ�е�3λ��Ч!)
//��Ƶ����=4*2^prer.�����ֵֻ����256!
//rlr:��װ�ؼĴ���ֵ:��11λ��Ч.
//ʱ�����(���):Tout=((4*2^prer)*rlr)/40 (ms).
void IWDG_Init(u8 prer,u16 rlr) 
{	
 	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);  //ʹ�ܶԼĴ���IWDG_PR��IWDG_RLR��д����
	
	IWDG_SetPrescaler(prer);  //����IWDGԤ��Ƶֵ:����IWDGԤ��ƵֵΪ64
	
	IWDG_SetReload(rlr);  //����IWDG��װ��ֵ
	
	IWDG_ReloadCounter();  //����IWDG��װ�ؼĴ�����ֵ��װ��IWDG������
	
	IWDG_Enable();  //ʹ��IWDG
}
//ι�������Ź�
void IWDG_Feed(void)
{   
 	IWDG_ReloadCounter();//reload										   
}

void WDG_task(void* pvParameters )
{
	while (1)
	{
		IWDG_Feed();		
		vTaskDelay(500);
	}
	
}
void Wdg_Task_Init(void)
{
	//����TASK3
	xTaskCreate((	TaskFunction_t) WDG_task,					//����3����
						(const char * 	)"WDG_task",			//����3������
						(uint16_t 			)WDG_STK_SIZE,		//����3��ջ��
						(void * 				)NULL ,			//���뺯������
						(UBaseType_t 		)WDG_TASK_PRIO,		//����3���ȼ�
						(TaskHandle_t * )&StartWdgHandler );	//ȡַ

}
