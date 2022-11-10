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


TaskHandle_t StartTaskHandler;		//任务句柄
int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);//设置系统中断优先级分组4
    W25QXX_Init();   
	delay_init();	    				//延时函数初始化  
    RTC_Init();	  			//RTC初始化	
	Uart1_Init(9600);					//初始化串口
    Uart2_Init(9600);					//初始化串口
	//LED_Init();		  					//初始化LED	
     Flash_Data_Init();	
    TIM3_PWM_Init(999,0);	//不分频，pwm频率为72000000/1000=72KHZ
    IWDG_Init(4,625);    //与分频数为64,重载值为625,溢出时间为1s	 Tout=((4×2^prer) ×rlr) /40
	Start_Task_Init();
	
}
void Start_Task_Init(void)
{
	xTaskCreate((	TaskFunction_t) start_task,				//任务函数
							(const char * 	)"start_task",			//任务函数名
							( uint16_t 			)START_STK_SIZE,		//任务堆栈数
							(void * 				)NULL ,							//传入函数参数
							(UBaseType_t 		)START_TASK_PRIO,		//任务优先级
							(TaskHandle_t * )&StartTaskHandler );//取址
								
    vTaskStartScheduler();          //开启任务调度

}
void start_task(void* pvParameters )
{
	
	taskENTER_CRITICAL();           //进入临界区
	Rtc_Task_Init();
	Uart_Task_Init();
	Wdg_Task_Init();								
	vTaskDelete(StartTaskHandler);//删除任务	
	taskEXIT_CRITICAL();            //退出临界区							
}







