#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys.h"
#include "rtc.h"
#include "usart.h"
#include "pwm.h"
#include "flash.h"	
#include "w25q64.h"
#include "cJSON.h"
////////////////////////////////////////////////////////////////////////////////// 	 
//如果使用ucos,则包括下面的头文件即可.
#if SYSTEM_SUPPORT_OS
#include "FreeRTOS.h"					//FreeRTOS使用
#include "task.h"
#include "queue.h"
#endif
 
TaskHandle_t StartUartHandler;		//任务句柄
#define MESSAGE_Q_NUM   4   	//发送数据的消息队列的数量 
QueueHandle_t Message_Queue;		//串口1信息队列句柄
QueueHandle_t Message_Queue_Uart2;	//串口2信息队列句柄

u8 USART_RX_BUF[USART_REC_LEN];     //接收缓冲,最大USART_REC_LEN个字节.
u8 USART2_RX_BUF[USART_REC_LEN];	//串口2接收缓冲,最大USART_REC_LEN个字节.	

Cjson Cjson_Buf={0};

u8 g_salable_sroduct_flag=0;	//药瓶正品标志位	
//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
u16 USART_RX_STA=0;       //接收状态标记
u16 USART2_RX_STA=0;	  //串口2接收状态标记	
//////////////////////////////////////////////////////////////////
//加入以下代码,支持printf函数,而不需要选择use MicroLIB	  
#if 1
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 

}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
void _sys_exit(int x) 
{ 
	x = x; 
} 
//__use_no_semihosting was requested, but _ttywrch was 
_ttywrch(int ch)
{
        ch = ch;
}
//重定义fputc函数 
int fputc(int ch, FILE *f)
{      
	while((USART1->SR&0X40)==0);//循环发送,直到发送完毕   
    USART1->DR = (u8) ch;      
	return ch;
}
#endif 

/*使用microLib的方法*/
 /* 
int fputc(int ch, FILE *f)
{
	USART_SendData(USART1, (uint8_t) ch);

	while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {}	
   
    return ch;
}
int GetKey (void)  { 

    while (!(USART1->SR & USART_FLAG_RXNE));

    return ((int)(USART1->DR & 0x1FF));
}
*/

#if EN_USART1_RX   //如果使能了接收
//串口1中断服务程序
//注意,读取USARTx->SR能避免莫名其妙的错误   		  
  void Uart1_Init(u32 bound){
	//GPIO端口设置
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);	//使能USART1，GPIOA时钟
  
	//USART1_TX   GPIOA.9
	GPIO_InitStructure.GPIO_Pin = USART1_TX; //PA.9
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIOA.9
   
	//USART1_RX	  GPIOA.10初始化
	GPIO_InitStructure.GPIO_Pin = USART1_RX;//PA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIOA.10  

	//Usart1 NVIC 配置
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=7 ;//抢占优先级7
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
  
	//USART 初始化设置

	USART_InitStructure.USART_BaudRate = bound;//串口波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式

	USART_Init(USART1, &USART_InitStructure); //初始化串口1
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//开启串口接受中断
	USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);//开启空闲中断
	USART_Cmd(USART1, ENABLE);                    //使能串口1 
}

void Uart2_Init(u32 bound)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//使能USART1，GPIOA时钟
  
	//USART2_TX   GPIOA.2
	GPIO_InitStructure.GPIO_Pin = USART2_TX; //PA.2
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIOA.2
   
	//USART2_RX	  GPIOA.3初始化
	GPIO_InitStructure.GPIO_Pin = USART2_RX;//PA3
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIOA.3  

	//Usart1 NVIC 配置
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=8 ;//抢占优先级7
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
  
	//USART 初始化设置

	USART_InitStructure.USART_BaudRate = bound;//串口波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式

	USART_Init(USART2, &USART_InitStructure); //初始化串口1
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//开启串口接受中断
	USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);//开启空闲中断
	USART_Cmd(USART2, ENABLE);                    //使能串口1 

}
void USART1_IRQHandler(void)                	//串口1中断服务程序
{
	u8 Res;
	BaseType_t xHigherPriorityTaskWoken;

	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)  //接收中断(接收到的数据必须是0x0d 0x0a结尾)
	{
		Res =USART_ReceiveData(USART1);	//读取接收到的数据
		USART_RX_BUF[USART_RX_STA++]=Res ;
		USART_ClearITPendingBit(USART1,USART_IT_RXNE);			  		 
    }
	//就向队列发送接收到的数据
	if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET &&(Message_Queue!=NULL))//空闲中断
	{
		xQueueSendFromISR(Message_Queue,USART_RX_BUF,&xHigherPriorityTaskWoken);//向队列中发送数据		
		USART_RX_STA=0;	
		memset(USART_RX_BUF,0,USART_REC_LEN);//清除数据接收缓冲区USART_RX_BUF,用于下一次数据接收
		Res = USART1->SR;//串口空闲中断的中断标志只能通过先读SR寄存器，再读DR寄存器清除！
        Res = USART1->DR;
		//USART_ClearITPendingBit(USART1,USART_IT_RXNE);	//关闭空闲中断
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);//如果需要的话进行一次任务切换

	} 
} 

void USART2_IRQHandler(void)                	//串口1中断服务程序
{
	u8 Res;
	BaseType_t xHigherPriorityTaskWoken_Uart2;

	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)  //接收中断(接收到的数据必须是0x0d 0x0a结尾)
	{
		Res =USART_ReceiveData(USART2);	//读取接收到的数据
		USART2_RX_BUF[USART2_RX_STA++]=Res ;
		USART_ClearITPendingBit(USART2,USART_IT_RXNE);			  		 
    }
	//就向队列发送接收到的数据
	if(USART_GetITStatus(USART2, USART_IT_IDLE) != RESET &&(Message_Queue_Uart2!=NULL))//空闲中断
	{
		xQueueSendFromISR(Message_Queue_Uart2,USART2_RX_BUF,&xHigherPriorityTaskWoken_Uart2);//向队列中发送数据		
		USART2_RX_STA=0;	
		memset(USART2_RX_BUF,0,USART_REC_LEN);//清除数据接收缓冲区USART2_RX_BUF,用于下一次数据接收
		Res = USART2->SR;//串口空闲中断的中断标志只能通过先读SR寄存器，再读DR寄存器清除！
        Res = USART2->DR;
		//USART_ClearITPendingBit(USART1,USART_IT_RXNE);	//关闭空闲中断
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken_Uart2);//如果需要的话进行一次任务切换

	} 
}
#endif	
void USART1_SendBit(u8 da)
{
	USART_SendData(USART1,da);
	while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待数据发送完毕 
}


void USART1_SendStr(u8 *PD)
{
    while(1)
	{
	    USART1_SendBit(*(PD++));
		if(*PD=='\0')
		{
		    break;
		}
	} 
	USART1_SendBit(13);
	USART1_SendBit(10); 
}
void USART2_SendBit(u8 da)
{
	USART_SendData(USART2,da);
	while(USART_GetFlagStatus(USART2,USART_FLAG_TC)!=SET);//等待数据发送完毕 
}


void USART2_SendStr(u8 *PD)
{
    while(1)
	{
	    USART2_SendBit(*(PD++));
		if(*PD=='\0')
		{
		    break;
		}
	} 
	USART2_SendBit(13);
	USART2_SendBit(10); 
}
void UART_task(void* pvParameters )
{
	u8 *buffer;
	BaseType_t xTaskWokenByReceive=pdFALSE;
	BaseType_t xTaskWokenByReceive_usart2=pdFALSE;
	BaseType_t err;
 	u8 ret;
	while (1)
	{
		if(Message_Queue!=NULL)
		{
			buffer=malloc(USART_REC_LEN);
			memset(buffer,0,USART_REC_LEN);	//清除缓冲区
			err=xQueueReceiveFromISR(Message_Queue,buffer,&xTaskWokenByReceive);//请求消息Message_Queue
			if(err == pdPASS)			//接收到消息
			{
				//USART1_SendStr(buffer);				
				if(strstr((const char*)buffer,CHEAK_TASK)!=0)
				{		
					for(ret=0;ret<Cjson_Buf.size;ret++)
					{
						printf("星期几开始:%d,星期几结束:%d,开始时间:%s,结束时间:%s,工作间隔：%d,工作档位：%d,工作时间:%d\r\n",Cjson_Buf.Cjson_Buffer_Data[ret].week_start,
										Cjson_Buf.Cjson_Buffer_Data[ret].week_end,
										Cjson_Buf.Cjson_Buffer_Data[ret].timestart,
										Cjson_Buf.Cjson_Buffer_Data[ret].timeend,
										Cjson_Buf.Cjson_Buffer_Data[ret].interval_time,
										Cjson_Buf.Cjson_Buffer_Data[ret].gears,  
										Cjson_Buf.Cjson_Buffer_Data[ret].worktime);
					}
					printf("在第%d个工作组工作\r\n",work_time.which_working_time);
					
				}
				else
				{
					ParseStr((const char *)buffer);
				}
			}
			
			free(buffer);		//释放内存
		}
		portYIELD_FROM_ISR(xTaskWokenByReceive);//如果需要的话进行一次任务切换

		if(Message_Queue_Uart2!=NULL)
		{
			buffer=malloc(USART_REC_LEN);
			memset(buffer,0,USART_REC_LEN);	//清除缓冲区
			err=xQueueReceiveFromISR(Message_Queue_Uart2,buffer,&xTaskWokenByReceive_usart2);//请求消息Message_Queue
			if(err == pdPASS)			//接收到消息
			{
				USART1_SendStr(buffer);				
			}
			free(buffer);		//释放内存
		}
		portYIELD_FROM_ISR(xTaskWokenByReceive_usart2);//如果需要的话进行一次任务切换
		vTaskDelay(200);
	}	
}

//CJSON数据处理函数
void ParseStr(const char *JSON)
{
    //JSON = {"arder":"add","weekstart":1,"weekend":2,"starttime":"8:00","endtime":"12:30","interval_time":"30","gears":"3","worktime":10};
    int start_hour,start_min,end_hour,end_min;
    cJSON *str_json, *str_arder;
    str_json = cJSON_Parse(JSON);   //创建JSON解析对象，返回JSON格式是否正确
    if (!str_json)
    {
        printf("JSON格式错误:%s \r\n", cJSON_GetErrorPtr()); //输出json格式错误信息
    }
    else
    {
        //printf("JSON格式正确:%s \r\n",cJSON_Print(str_json) );
        str_arder = cJSON_GetObjectItem(str_json, "arder"); //获取arder键对应的值的信息
        if (str_arder->type == cJSON_String)
        {
            printf("类型:%s \r\n", str_arder->valuestring);
           if(strstr(str_arder->valuestring,add)!=0)    //判断是删除还是添加
            {    
                cJSON *str_weekstart,*str_weekend,*str_timestart,*str_timeend,*str_intervaltime,*str_gears,*str_worktime;
				char *time_end,*time_start;
                str_weekstart = cJSON_GetObjectItem(str_json, "weekstart");   //获取Cjson数据对应的值的信息
                str_weekend = cJSON_GetObjectItem(str_json, "weekend");   
                str_timestart = cJSON_GetObjectItem(str_json, "starttime");   
                str_timeend = cJSON_GetObjectItem(str_json, "endtime");   
                str_intervaltime = cJSON_GetObjectItem(str_json, "interval_time");   
                str_gears = cJSON_GetObjectItem(str_json, "gears");   
                str_worktime = cJSON_GetObjectItem(str_json, "worktime");
				time_start= str_timestart->valuestring;
				time_end =str_timeend->valuestring;
                sscanf(time_start,  "%d:%d",&start_hour,&start_min);	//解析时间
                sscanf(time_end,  "%d:%d",&end_hour,&end_min);
                if(Cjson_Buf.size<CJSON_BUF_DATA_LEN)
                {
                    Cjson_Buf.Cjson_Buffer_Data[Cjson_Buf.size].week_start=          str_weekstart->valueint; 
                    Cjson_Buf.Cjson_Buffer_Data[Cjson_Buf.size].week_end=            str_weekend->valueint;
                    strcpy(Cjson_Buf.Cjson_Buffer_Data[Cjson_Buf.size].timestart,time_start);
                    strcpy(Cjson_Buf.Cjson_Buffer_Data[Cjson_Buf.size].timeend,time_end);
                    Cjson_Buf.Cjson_Buffer_Data[Cjson_Buf.size].interval_time=     str_intervaltime->valueint;
                    Cjson_Buf.Cjson_Buffer_Data[Cjson_Buf.size].gears=              str_gears->valueint;                   
                    Cjson_Buf.Cjson_Buffer_Data[Cjson_Buf.size].worktime=           str_worktime->valueint;
                    Cjson_Buf.Cjson_Buffer_Data[Cjson_Buf.size].time_start_hour=    start_hour;    
                    Cjson_Buf.Cjson_Buffer_Data[Cjson_Buf.size].time_start_min=     start_min;
                    Cjson_Buf.Cjson_Buffer_Data[Cjson_Buf.size].time_end_hour=      end_hour;   
                    Cjson_Buf.Cjson_Buffer_Data[Cjson_Buf.size].time_end_min=       end_min;                   
                    printf("星期几开始:%d,星期几结束:%d,开始时间:%s,结束时间:%s,工作间隔：%d,工作档位：%d,工作时间:%d秒\r\n",Cjson_Buf.Cjson_Buffer_Data[Cjson_Buf.size].week_start,
                                                Cjson_Buf.Cjson_Buffer_Data[Cjson_Buf.size].week_end,
                                                Cjson_Buf.Cjson_Buffer_Data[Cjson_Buf.size].timestart,
                                                Cjson_Buf.Cjson_Buffer_Data[Cjson_Buf.size].timeend,
                                                Cjson_Buf.Cjson_Buffer_Data[Cjson_Buf.size].interval_time,
                                                Cjson_Buf.Cjson_Buffer_Data[Cjson_Buf.size].gears,  
                                                Cjson_Buf.Cjson_Buffer_Data[Cjson_Buf.size].worktime);
                Cjson_Buf.size++; 
                W25QXX_Write((u8*)&Cjson_Buf,CJSON_DATA_FLASH_BASE,CJSON_DATA_SIZE);
                }
                else
                {
                    printf("设置满了，请清除旧设置"); 
                }
            }
            else if(strstr(str_arder->valuestring,del)!=0) 
            {
                cJSON *str_serial;
                u8 num;
                str_serial = cJSON_GetObjectItem(str_json, "serial");   //获取serial键对应的值的信息
                if(str_serial->type==cJSON_Number)
                {
                    printf("序号:%d \r\n", str_serial->valueint);                   
                }        
                for(num=str_serial->valueint;num<Cjson_Buf.size;num++)		//删除对应工作组数据内容
                {                   
                    Cjson_Buf.Cjson_Buffer_Data[num]= Cjson_Buf.Cjson_Buffer_Data[num+1];
                    memset(&Cjson_Buf.Cjson_Buffer_Data[num+1],0,sizeof(Cjson_Buf.Cjson_Buffer_Data[num+1]));
                }
                if(Cjson_Buf.size>0)
                    Cjson_Buf.size--; 
                W25QXX_Write((u8*)&Cjson_Buf,CJSON_DATA_FLASH_BASE,CJSON_DATA_SIZE);
                memset(&work_time,0,sizeof(work_time));//每次删除操作后都要复位重新判断
            }
            else if(strstr(str_arder->valuestring,SET_RTC_TIME)!=0)		//设置RTC时间
            {
				int  year,month,date,hour,min,sec;
				cJSON *str_date,*str_time;
				char *data_date,*data_time;				
				str_date = cJSON_GetObjectItem(str_json,"set_date");
				data_date = str_date->valuestring;
				str_time = cJSON_GetObjectItem(str_json,"set_time");
				data_time = str_time->valuestring;
				sscanf(data_date, "%d-%d-%d", &year, &month, &date);
				sscanf(data_time, "%d:%d:%d", &hour, &min, &sec);
				RTC_Set(year,month,date,hour,min,sec);  //设置时间	
            }
        }
         cJSON_Delete(str_json);    //释放内存
    }
}

void Uart_Task_Init(void)
{
 	//创建消息队列
    Message_Queue=xQueueCreate(MESSAGE_Q_NUM,USART_REC_LEN); //创建消息Message_Queue,队列项长度是串口接收缓冲区长度
	Message_Queue_Uart2=xQueueCreate(MESSAGE_Q_NUM,USART_REC_LEN); //创建消息Message_Queue,队列项长度是串口接收缓冲区长度
	//创建TASK2
	xTaskCreate((	TaskFunction_t) UART_task,					//任务2函数
						(const char * 	)"UART_task",			//任务2函数名
						(uint16_t 			)UART_STK_SIZE,		//任务2堆栈数
						(void * 				)NULL ,			//传入函数参数
						(UBaseType_t 		)UART_TASK_PRIO,	//任务2优先级
						(TaskHandle_t * )&StartUartHandler );	//取址

}
