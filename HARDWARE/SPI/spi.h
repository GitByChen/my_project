#ifndef __SPI_H
#define __SPI_H
#include "sys.h"


#define FLASH_SPI_CS_PIN    GPIO_Pin_4      //CS
#define FLASH_SPI_SCK_PIN	GPIO_Pin_5      //SCK
#define FLASH_SPI_MISO_PIN	GPIO_Pin_6      //MISO
#define FLASH_SPI_MOSI_PIN	GPIO_Pin_7      //MOSI

void SPI1_Init(void);			  //��ʼ��SPI��
void SPI1_SetSpeed(u8 SpeedSet);  //����SPI�ٶ�   
u8 SPI1_ReadWriteByte(u8 TxData);//SPI���߶�дһ���ֽ�
		 
#endif

