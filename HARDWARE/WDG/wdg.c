#include "wdg.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
TaskHandle_t StartWdgHandler;		//任务句柄

//初始化独立看门狗
//prer:分频数:0~7(只有低3位有效!)
//分频因子=4*2^prer.但最大值只能是256!
//rlr:重装载寄存器值:低11位有效.
//时间计算(大概):Tout=((4*2^prer)*rlr)/40 (ms).
void IWDG_Init(u8 prer,u16 rlr) 
{	
 	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);  //使能对寄存器IWDG_PR和IWDG_RLR的写操作
	
	IWDG_SetPrescaler(prer);  //设置IWDG预分频值:设置IWDG预分频值为64
	
	IWDG_SetReload(rlr);  //设置IWDG重装载值
	
	IWDG_ReloadCounter();  //按照IWDG重装载寄存器的值重装载IWDG计数器
	
	IWDG_Enable();  //使能IWDG
}
//喂独立看门狗
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
	//创建TASK3
	xTaskCreate((	TaskFunction_t) WDG_task,					//任务3函数
						(const char * 	)"WDG_task",			//任务3函数名
						(uint16_t 			)WDG_STK_SIZE,		//任务3堆栈数
						(void * 				)NULL ,			//传入函数参数
						(UBaseType_t 		)WDG_TASK_PRIO,		//任务3优先级
						(TaskHandle_t * )&StartWdgHandler );	//取址

}
