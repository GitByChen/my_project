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
//���ʹ��ucos,����������ͷ�ļ�����.
#if SYSTEM_SUPPORT_OS
#include "FreeRTOS.h"					//FreeRTOSʹ��
#include "task.h"
#include "queue.h"
#endif
 
TaskHandle_t StartUartHandler;		//������
#define MESSAGE_Q_NUM   4   	//�������ݵ���Ϣ���е����� 
QueueHandle_t Message_Queue;		//����1��Ϣ���о��
QueueHandle_t Message_Queue_Uart2;	//����2��Ϣ���о��

u8 USART_RX_BUF[USART_REC_LEN];     //���ջ���,���USART_REC_LEN���ֽ�.
u8 USART2_RX_BUF[USART_REC_LEN];	//����2���ջ���,���USART_REC_LEN���ֽ�.	

Cjson Cjson_Buf={0};

u8 g_salable_sroduct_flag=0;	//ҩƿ��Ʒ��־λ	
//����״̬
//bit15��	������ɱ�־
//bit14��	���յ�0x0d
//bit13~0��	���յ�����Ч�ֽ���Ŀ
u16 USART_RX_STA=0;       //����״̬���
u16 USART2_RX_STA=0;	  //����2����״̬���	
//////////////////////////////////////////////////////////////////
//�������´���,֧��printf����,������Ҫѡ��use MicroLIB	  
#if 1
#pragma import(__use_no_semihosting)             
//��׼����Ҫ��֧�ֺ���                 
struct __FILE 
{ 
	int handle; 

}; 

FILE __stdout;       
//����_sys_exit()�Ա���ʹ�ð�����ģʽ    
void _sys_exit(int x) 
{ 
	x = x; 
} 
//__use_no_semihosting was requested, but _ttywrch was 
_ttywrch(int ch)
{
        ch = ch;
}
//�ض���fputc���� 
int fputc(int ch, FILE *f)
{      
	while((USART1->SR&0X40)==0);//ѭ������,ֱ���������   
    USART1->DR = (u8) ch;      
	return ch;
}
#endif 

/*ʹ��microLib�ķ���*/
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

#if EN_USART1_RX   //���ʹ���˽���
//����1�жϷ������
//ע��,��ȡUSARTx->SR�ܱ���Ī������Ĵ���   		  
  void Uart1_Init(u32 bound){
	//GPIO�˿�����
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);	//ʹ��USART1��GPIOAʱ��
  
	//USART1_TX   GPIOA.9
	GPIO_InitStructure.GPIO_Pin = USART1_TX; //PA.9
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//�����������
	GPIO_Init(GPIOA, &GPIO_InitStructure);//��ʼ��GPIOA.9
   
	//USART1_RX	  GPIOA.10��ʼ��
	GPIO_InitStructure.GPIO_Pin = USART1_RX;//PA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//��������
	GPIO_Init(GPIOA, &GPIO_InitStructure);//��ʼ��GPIOA.10  

	//Usart1 NVIC ����
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=7 ;//��ռ���ȼ�7
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���
  
	//USART ��ʼ������

	USART_InitStructure.USART_BaudRate = bound;//���ڲ�����
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ

	USART_Init(USART1, &USART_InitStructure); //��ʼ������1
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//�������ڽ����ж�
	USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);//���������ж�
	USART_Cmd(USART1, ENABLE);                    //ʹ�ܴ���1 
}

void Uart2_Init(u32 bound)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//ʹ��USART1��GPIOAʱ��
  
	//USART2_TX   GPIOA.2
	GPIO_InitStructure.GPIO_Pin = USART2_TX; //PA.2
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//�����������
	GPIO_Init(GPIOA, &GPIO_InitStructure);//��ʼ��GPIOA.2
   
	//USART2_RX	  GPIOA.3��ʼ��
	GPIO_InitStructure.GPIO_Pin = USART2_RX;//PA3
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//��������
	GPIO_Init(GPIOA, &GPIO_InitStructure);//��ʼ��GPIOA.3  

	//Usart1 NVIC ����
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=8 ;//��ռ���ȼ�7
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���
  
	//USART ��ʼ������

	USART_InitStructure.USART_BaudRate = bound;//���ڲ�����
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ

	USART_Init(USART2, &USART_InitStructure); //��ʼ������1
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//�������ڽ����ж�
	USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);//���������ж�
	USART_Cmd(USART2, ENABLE);                    //ʹ�ܴ���1 

}
void USART1_IRQHandler(void)                	//����1�жϷ������
{
	u8 Res;
	BaseType_t xHigherPriorityTaskWoken;

	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)  //�����ж�(���յ������ݱ�����0x0d 0x0a��β)
	{
		Res =USART_ReceiveData(USART1);	//��ȡ���յ�������
		USART_RX_BUF[USART_RX_STA++]=Res ;
		USART_ClearITPendingBit(USART1,USART_IT_RXNE);			  		 
    }
	//������з��ͽ��յ�������
	if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET &&(Message_Queue!=NULL))//�����ж�
	{
		xQueueSendFromISR(Message_Queue,USART_RX_BUF,&xHigherPriorityTaskWoken);//������з�������		
		USART_RX_STA=0;	
		memset(USART_RX_BUF,0,USART_REC_LEN);//������ݽ��ջ�����USART_RX_BUF,������һ�����ݽ���
		Res = USART1->SR;//���ڿ����жϵ��жϱ�־ֻ��ͨ���ȶ�SR�Ĵ������ٶ�DR�Ĵ��������
        Res = USART1->DR;
		//USART_ClearITPendingBit(USART1,USART_IT_RXNE);	//�رտ����ж�
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);//�����Ҫ�Ļ�����һ�������л�

	} 
} 

void USART2_IRQHandler(void)                	//����1�жϷ������
{
	u8 Res;
	BaseType_t xHigherPriorityTaskWoken_Uart2;

	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)  //�����ж�(���յ������ݱ�����0x0d 0x0a��β)
	{
		Res =USART_ReceiveData(USART2);	//��ȡ���յ�������
		USART2_RX_BUF[USART2_RX_STA++]=Res ;
		USART_ClearITPendingBit(USART2,USART_IT_RXNE);			  		 
    }
	//������з��ͽ��յ�������
	if(USART_GetITStatus(USART2, USART_IT_IDLE) != RESET &&(Message_Queue_Uart2!=NULL))//�����ж�
	{
		xQueueSendFromISR(Message_Queue_Uart2,USART2_RX_BUF,&xHigherPriorityTaskWoken_Uart2);//������з�������		
		USART2_RX_STA=0;	
		memset(USART2_RX_BUF,0,USART_REC_LEN);//������ݽ��ջ�����USART2_RX_BUF,������һ�����ݽ���
		Res = USART2->SR;//���ڿ����жϵ��жϱ�־ֻ��ͨ���ȶ�SR�Ĵ������ٶ�DR�Ĵ��������
        Res = USART2->DR;
		//USART_ClearITPendingBit(USART1,USART_IT_RXNE);	//�رտ����ж�
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken_Uart2);//�����Ҫ�Ļ�����һ�������л�

	} 
}
#endif	
void USART1_SendBit(u8 da)
{
	USART_SendData(USART1,da);
	while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//�ȴ����ݷ������ 
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
	while(USART_GetFlagStatus(USART2,USART_FLAG_TC)!=SET);//�ȴ����ݷ������ 
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
			memset(buffer,0,USART_REC_LEN);	//���������
			err=xQueueReceiveFromISR(Message_Queue,buffer,&xTaskWokenByReceive);//������ϢMessage_Queue
			if(err == pdPASS)			//���յ���Ϣ
			{
				//USART1_SendStr(buffer);				
				if(strstr((const char*)buffer,CHEAK_TASK)!=0)
				{		
					for(ret=0;ret<Cjson_Buf.size;ret++)
					{
						printf("���ڼ���ʼ:%d,���ڼ�����:%d,��ʼʱ��:%s,����ʱ��:%s,���������%d,������λ��%d,����ʱ��:%d\r\n",Cjson_Buf.Cjson_Buffer_Data[ret].week_start,
										Cjson_Buf.Cjson_Buffer_Data[ret].week_end,
										Cjson_Buf.Cjson_Buffer_Data[ret].timestart,
										Cjson_Buf.Cjson_Buffer_Data[ret].timeend,
										Cjson_Buf.Cjson_Buffer_Data[ret].interval_time,
										Cjson_Buf.Cjson_Buffer_Data[ret].gears,  
										Cjson_Buf.Cjson_Buffer_Data[ret].worktime);
					}
					printf("�ڵ�%d�������鹤��\r\n",work_time.which_working_time);
					
				}
				else
				{
					ParseStr((const char *)buffer);
				}
			}
			
			free(buffer);		//�ͷ��ڴ�
		}
		portYIELD_FROM_ISR(xTaskWokenByReceive);//�����Ҫ�Ļ�����һ�������л�

		if(Message_Queue_Uart2!=NULL)
		{
			buffer=malloc(USART_REC_LEN);
			memset(buffer,0,USART_REC_LEN);	//���������
			err=xQueueReceiveFromISR(Message_Queue_Uart2,buffer,&xTaskWokenByReceive_usart2);//������ϢMessage_Queue
			if(err == pdPASS)			//���յ���Ϣ
			{
				USART1_SendStr(buffer);				
			}
			free(buffer);		//�ͷ��ڴ�
		}
		portYIELD_FROM_ISR(xTaskWokenByReceive_usart2);//�����Ҫ�Ļ�����һ�������л�
		vTaskDelay(200);
	}	
}

//CJSON���ݴ�����
void ParseStr(const char *JSON)
{
    //JSON = {"arder":"add","weekstart":1,"weekend":2,"starttime":"8:00","endtime":"12:30","interval_time":"30","gears":"3","worktime":10};
    int start_hour,start_min,end_hour,end_min;
    cJSON *str_json, *str_arder;
    str_json = cJSON_Parse(JSON);   //����JSON�������󣬷���JSON��ʽ�Ƿ���ȷ
    if (!str_json)
    {
        printf("JSON��ʽ����:%s \r\n", cJSON_GetErrorPtr()); //���json��ʽ������Ϣ
    }
    else
    {
        //printf("JSON��ʽ��ȷ:%s \r\n",cJSON_Print(str_json) );
        str_arder = cJSON_GetObjectItem(str_json, "arder"); //��ȡarder����Ӧ��ֵ����Ϣ
        if (str_arder->type == cJSON_String)
        {
            printf("����:%s \r\n", str_arder->valuestring);
           if(strstr(str_arder->valuestring,add)!=0)    //�ж���ɾ���������
            {    
                cJSON *str_weekstart,*str_weekend,*str_timestart,*str_timeend,*str_intervaltime,*str_gears,*str_worktime;
				char *time_end,*time_start;
                str_weekstart = cJSON_GetObjectItem(str_json, "weekstart");   //��ȡCjson���ݶ�Ӧ��ֵ����Ϣ
                str_weekend = cJSON_GetObjectItem(str_json, "weekend");   
                str_timestart = cJSON_GetObjectItem(str_json, "starttime");   
                str_timeend = cJSON_GetObjectItem(str_json, "endtime");   
                str_intervaltime = cJSON_GetObjectItem(str_json, "interval_time");   
                str_gears = cJSON_GetObjectItem(str_json, "gears");   
                str_worktime = cJSON_GetObjectItem(str_json, "worktime");
				time_start= str_timestart->valuestring;
				time_end =str_timeend->valuestring;
                sscanf(time_start,  "%d:%d",&start_hour,&start_min);	//����ʱ��
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
                    printf("���ڼ���ʼ:%d,���ڼ�����:%d,��ʼʱ��:%s,����ʱ��:%s,���������%d,������λ��%d,����ʱ��:%d��\r\n",Cjson_Buf.Cjson_Buffer_Data[Cjson_Buf.size].week_start,
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
                    printf("�������ˣ������������"); 
                }
            }
            else if(strstr(str_arder->valuestring,del)!=0) 
            {
                cJSON *str_serial;
                u8 num;
                str_serial = cJSON_GetObjectItem(str_json, "serial");   //��ȡserial����Ӧ��ֵ����Ϣ
                if(str_serial->type==cJSON_Number)
                {
                    printf("���:%d \r\n", str_serial->valueint);                   
                }        
                for(num=str_serial->valueint;num<Cjson_Buf.size;num++)		//ɾ����Ӧ��������������
                {                   
                    Cjson_Buf.Cjson_Buffer_Data[num]= Cjson_Buf.Cjson_Buffer_Data[num+1];
                    memset(&Cjson_Buf.Cjson_Buffer_Data[num+1],0,sizeof(Cjson_Buf.Cjson_Buffer_Data[num+1]));
                }
                if(Cjson_Buf.size>0)
                    Cjson_Buf.size--; 
                W25QXX_Write((u8*)&Cjson_Buf,CJSON_DATA_FLASH_BASE,CJSON_DATA_SIZE);
                memset(&work_time,0,sizeof(work_time));//ÿ��ɾ��������Ҫ��λ�����ж�
            }
            else if(strstr(str_arder->valuestring,SET_RTC_TIME)!=0)		//����RTCʱ��
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
				RTC_Set(year,month,date,hour,min,sec);  //����ʱ��	
            }
        }
         cJSON_Delete(str_json);    //�ͷ��ڴ�
    }
}

void Uart_Task_Init(void)
{
 	//������Ϣ����
    Message_Queue=xQueueCreate(MESSAGE_Q_NUM,USART_REC_LEN); //������ϢMessage_Queue,��������Ǵ��ڽ��ջ���������
	Message_Queue_Uart2=xQueueCreate(MESSAGE_Q_NUM,USART_REC_LEN); //������ϢMessage_Queue,��������Ǵ��ڽ��ջ���������
	//����TASK2
	xTaskCreate((	TaskFunction_t) UART_task,					//����2����
						(const char * 	)"UART_task",			//����2������
						(uint16_t 			)UART_STK_SIZE,		//����2��ջ��
						(void * 				)NULL ,			//���뺯������
						(UBaseType_t 		)UART_TASK_PRIO,	//����2���ȼ�
						(TaskHandle_t * )&StartUartHandler );	//ȡַ

}
