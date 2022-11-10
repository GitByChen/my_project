#ifndef __RTC_H
#define __RTC_H	    
#include	"stm32f10x_rtc.h"

//时间结构体
typedef struct 
{
	vu8 hour;
	vu8 min;
	vu8 sec;			
	//公历日月年周
	vu16 w_year;
	vu8  w_month;
	vu8  w_date;
	vu8  week;		 
}_calendar_obj;					 
extern _calendar_obj calendar;	//日历结构体

typedef struct 
{	
	char *week;
	char *time;
	u8 work_time_flag;	//是否在工作时间段内
	u8 working_flag;	//是否正在工作	0:没有工作；1：正在工作；2：工作完成
	u8 working_time;	//工作时间计数
	u8 which_working_time;  //记录是哪个工作组在工作
	u16 working_interval_time;	//工作间隔时间计数
}_work_time;
extern _work_time work_time; 

extern u8 g_salable_sroduct_flag;	//药瓶正品标志位
	
extern u8 const mon_table[12];	//月份日期数据表

void Disp_Time(u8 x,u8 y,u8 size);//在制定位置开始显示时间
void Disp_Week(u8 x,u8 y,u8 size,u8 lang);//在指定位置显示星期
u8 RTC_Init(void);        //初始化RTC,返回0,失败;1,成功;
u8 Is_Leap_Year(u16 year);//平年,闰年判断
u8 RTC_Alarm_Set(u16 syear,u8 smon,u8 sday,u8 hour,u8 min,u8 sec);
u8 RTC_Get(void);         //更新时间   
u8 RTC_Get_Week(u16 year,u8 month,u8 day);
u8 RTC_Set(u16 syear,u8 smon,u8 sday,u8 hour,u8 min,u8 sec);//设置时间
void Rtc_Task_Init(void);
#endif


