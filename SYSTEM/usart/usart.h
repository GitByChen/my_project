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
#define USART_REC_LEN  			200  	//�����������ֽ��� 200
#define EN_USART1_RX 			1		//ʹ�ܣ�1��/��ֹ��0������1����
#define CJSON_BUF_LEN           200
#define CJSON_BUF_DATA_LEN      10  

#define USART2_BUF_LEN           200

#define CJSON_DATA_FLASH_BASE 	0X00	//CJSON���ݴ���׵�ַ	
#define CJSON_DATA_SIZE         sizeof(Cjson_Buf)

typedef struct{
    

    char timestart[10];        //��ʼʱ���ַ���
    char timeend[10];          //����ʱ���ַ���
    u8 week_start;          //���ڼ���ʼ
    u8 week_end;            //���ڼ�����
    u8 time_start_hour;     //���������Ŀ�ʼʱ��
    u8 time_start_min;
    u8 time_end_hour;       //���������Ľ���ʱ��
    u8 time_end_min;
    u8 interval_time;   //������� ��λ��min
    u8 gears;           //������λ ��λ��min
    u16 worktime;       //����ʱ�� ��λ��s

}Cjson_Buffer;

typedef struct {
    u8 size;        //���������
    Cjson_Buffer Cjson_Buffer_Data[CJSON_BUF_DATA_LEN];
}Cjson;

extern Cjson Cjson_Buf;

extern u8  USART_RX_BUF[USART_REC_LEN]; //���ջ���,���USART_REC_LEN���ֽ�.ĩ�ֽ�Ϊ���з� 
extern u8  USART2_RX_BUF[USART_REC_LEN]; //���ջ���,���USART_REC_LEN���ֽ�.ĩ�ֽ�Ϊ���з� 

extern u16 USART_RX_STA;         		//����״̬���	
extern u16 USART2_RX_STA;         		//����״̬���	


//����봮���жϽ��գ��벻Ҫע�����º궨��
void Uart1_Init(u32 bound);
void USART1_SendBit(u8 da);
void USART1_SendStr(u8 *PD);

void Uart2_Init(u32 bound);
void USART2_SendBit(u8 da);
void USART2_SendStr(u8 *PD);

void Uart_Task_Init(void);

void ParseStr(const char *JSON);    //CJSON���ݴ�����

#endif


