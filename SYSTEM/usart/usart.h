#ifndef __USART_H
#define __USART_H
#include "stdio.h"	


#define pwm_size                "pwm_size="
#define add                     "add"
#define del                     "del"
#define CHEAK_TASK              "cheak_task"
#define SET_RTC_TIME            "set_rtc_time"

#define USART1_TX               GPIO_Pin_9
#define USART1_RX               GPIO_Pin_10
#define USART2_TX               GPIO_Pin_2
#define USART2_RX               GPIO_Pin_3
#define USART_REC_LEN  			200  	//定义最大接收字节数 200
#define EN_USART1_RX 			1		//使能（1）/禁止（0）串口1接收
#define CJSON_BUF_LEN           200
#define CJSON_BUF_DATA_LEN      10  

#define USART2_BUF_LEN           200

#define CJSON_DATA_FLASH_BASE 	0X00	//CJSON数据存放首地址	
#define CJSON_DATA_SIZE         sizeof(Cjson_Buf)

typedef struct{
    

    char timestart[10];        //开始时间字符串
    char timeend[10];          //结束时间字符串
    u8 week_start;          //星期几开始
    u8 week_end;            //星期几结束
    u8 time_start_hour;     //解析出来的开始时间
    u8 time_start_min;
    u8 time_end_hour;       //解析出来的结束时间
    u8 time_end_min;
    u8 interval_time;   //工作间隔 单位：min
    u8 gears;           //工作档位 单位：min
    u16 worktime;       //工作时间 单位：s

}Cjson_Buffer;

typedef struct {
    u8 size;        //工作组计数
    Cjson_Buffer Cjson_Buffer_Data[CJSON_BUF_DATA_LEN];
}Cjson;

extern Cjson Cjson_Buf;

extern u8  USART_RX_BUF[USART_REC_LEN]; //接收缓冲,最大USART_REC_LEN个字节.末字节为换行符 
extern u8  USART2_RX_BUF[USART_REC_LEN]; //接收缓冲,最大USART_REC_LEN个字节.末字节为换行符 

extern u16 USART_RX_STA;         		//接收状态标记	
extern u16 USART2_RX_STA;         		//接收状态标记	


//如果想串口中断接收，请不要注释以下宏定义
void Uart1_Init(u32 bound);
void USART1_SendBit(u8 da);
void USART1_SendStr(u8 *PD);

void Uart2_Init(u32 bound);
void USART2_SendBit(u8 da);
void USART2_SendStr(u8 *PD);

void Uart_Task_Init(void);

void ParseStr(const char *JSON);    //CJSON数据处理函数

#endif


