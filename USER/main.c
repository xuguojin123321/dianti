#include "led.h"
#include "delay.h"
#include "sys.h"
#include "usart.h"
#include "cJSON.h"
#include "string.h"
#include "stdlib.h"
#include "malloc.h"
#include "key.h"
#include "usart.h"
#include "aes.h"

//前置声明
void u3_send_bytes(uint8_t *buf, uint16_t len);

#define __floor 1 //设置楼层数，1为1楼，防止存在多个出现混乱  
#define msg_req 1 
#define msg_rsp 0 

typedef struct{
	char floor;//几楼 
	char req;
	char rsp;
	int seq;
	int distance;
}pkg_date;


unsigned char AES128key[16] = "shengxundianti";//秘钥
unsigned char AES_IV[16]= "20260116sxdt1234";//向量表


static int seq = 0;

static char need_send = 0;//没收到回复不停的发 
static char msg_cnt = 0;
static pkg_date sg_date = {0};

// 打印缓冲区数据的十六进制和ASCII表示
void dump_hex(const char *tag, uint8_t *buf, int len)
{
	int i;
	printf("\r\n[%s] len=%d: ", tag, len);
	// 打印十六进制
	for(i = 0; i < len; i++)
		printf("%02X ", buf[i]);
	printf("\r\n");
	// 打印ASCII
	printf("[%s] ASCII: ", tag);
	for(i = 0; i < len; i++)
	{
		if(buf[i] >= 32 && buf[i] <= 126)
			printf("%c", buf[i]);
		else
			printf(".");
	}
	printf("\r\n");
}

void hc13_send_date(pkg_date date)
{
	cJSON * usr;
	char *data_buf;
	uint8_t encBuf[200] = {0};
	uint8_t frame_header[3] = {0};  // [0xAA][len_hi][len_lo]
	uint8_t frame_tail = 0x0d;      // 帧尾
	int plain_len = 0;
	int aes_len  = 0;
	usr=cJSON_CreateObject();   //创建根数据对象

	mymemset(encBuf,0,sizeof(encBuf));
	mymemset(frame_header,0,sizeof(frame_header));

	if(seq > 10000)
	{
		seq = 0;//太大了，归零  
	}
	
	cJSON_AddItemToObject(usr, "seq", cJSON_CreateNumber(seq++));  //根节点下添加数字
	cJSON_AddNumberToObject(usr, "req", date.req);
	cJSON_AddNumberToObject(usr, "rsp", date.rsp);  //根节点下添加数字
	cJSON_AddNumberToObject(usr, "floor", date.floor);  //根节点下添加数字
	cJSON_AddNumberToObject(usr, "distance", date.distance);  //根节点下添加数字
	data_buf = cJSON_Print(usr);   //将json形式打印成正常字符串形式(带有\r\n)
	printf("\r\n您发送的消息为:%s\r\n",data_buf);

	
	plain_len = strlen((char*)data_buf);
	aes_len = ((plain_len + 15) / 16) * 16;

	aes_encrypt((const unsigned char*)data_buf, encBuf, aes_len, AES_IV);
	
	// 组织协议帧: [0xAA][len_hi][len_lo][encrypted_data][0x0d]
	frame_header[0] = 0xAA;
	frame_header[1] = (uint8_t)((aes_len >> 8) & 0xFF);
	frame_header[2] = (uint8_t)(aes_len & 0xFF);
	
	u3_send_bytes(frame_header, 3);
	u3_send_bytes(encBuf, aes_len);
	dump_hex("send Encrypted Data", encBuf, aes_len);
	u3_send_bytes(&frame_tail, 1);

	cJSON_Delete(usr);
	myfree(data_buf);
}



int parse_distance(const char *str)
{
    int value = 0;
    sscanf(str, "distance:%d", &value);
    return value;
}

static int have_people_count  = 0;
static int no_people_count  = 0;
static int sleep_count  = 0;

void hc13_recv_process(u8* buf,int len)//发送数据端需要处理的任务
{
	cJSON *json,*json_vlaue;
	pkg_date date;
	uint8_t decBuf[200] = {0};
	int dec_len = 0;
	int json_len = 0;
	int idx = 0;
	memset(&date,0,sizeof(date));
	mymemset(decBuf,0,sizeof(decBuf));

	dump_hex("Received Encrypted Data", buf, len);
	// 解密接收到的数据
	dec_len = ((len + 15) / 16) * 16;  // 对齐到16字节
	aes_decrypt(buf, decBuf, dec_len, AES_IV);
	
	// 查找JSON结束标志 }
	json_len = 0;
	for(idx = 0; idx < dec_len; idx++)
	{
		if(decBuf[idx] == '}')
		{
			json_len = idx + 1;
			break;
		}
	}
	
	if(json_len == 0)
	{
		printf("JSON end marker not found\r\n");
		return;
	}
	
	decBuf[json_len] = '\0';  // 在}后添加字符串结束符
	
	printf("\r\nrecv msg(encrypted):");
	//dump_hex("encrypted data", buf, len);
	printf("\r\nrecv msg(decrypted):%s\r\n",(char*)decBuf);
	printf("%s\r\n",(char*)decBuf);

	json = cJSON_Parse((const char*)decBuf); 
	if(json == NULL)
	{
		printf("JSON parse failed\r\n");
		return;
	}
	json_vlaue = cJSON_GetObjectItem( json , "seq" );  //从json获取键值内容
	if(json_vlaue != NULL)
	{
		//USART_SendData(USART3,'a');  调试打印
		date.seq = json_vlaue->valueint;
	}
		json_vlaue = cJSON_GetObjectItem( json , "distance" );  //从json获取键值内容
	if(json_vlaue != NULL)
	{
		//USART_SendData(USART3,'a');  调试打印
		date.distance  = json_vlaue->valueint;
	}

			json_vlaue = cJSON_GetObjectItem( json , "floor" );  //从json获取键值内容
	if(json_vlaue != NULL)
	{
		//USART_SendData(USART3,'a');  调试打印
		date.floor  = json_vlaue->valueint;
	}

	json_vlaue = cJSON_GetObjectItem( json , "req" );  //从json获取键值内容
	if(json_vlaue != NULL)
	{
		if(json_vlaue->valueint == 1 && date.floor == __floor)
		{
			//接通按键 
			HUTI = 1;
			date.rsp = 1;
			date.req = 0;
			hc13_send_date(date);//回复消息
			delay_ms(100);
			HUTI = 0;
		}
	}

	json_vlaue = cJSON_GetObjectItem( json , "rsp" );  //从json获取键值内容
	if(json_vlaue != NULL)
	{
		if(json_vlaue->valueint == 1)
		{
			need_send = 0;
			msg_cnt = 0;
			if(date.floor == __floor)
			{
				sleep_count = 190;//160*190=28800 29s
			}
		}
	}

	cJSON_Delete(json);
	//print_value(&date);
}

void ld2402_recv_process(u8* buf,int len)//发送数据端需要处理的任务
{
	int distance = 0;
	memset(&sg_date,0,sizeof(sg_date));

	printf("%s", (char*)buf);


	distance = 100;//parse_distance((const char*)buf);
	
	if(sleep_count >= 1)
	{
		sleep_count --;
		printf("sleeping sleep_count %d\r\n",sleep_count);
	}
	
	if(strcmp((const char*)buf,"OFF") == 0 || distance > 400)
	{
		no_people_count ++;
		printf("no people count %d time %d ms\r\n",no_people_count,no_people_count*160);
		LED1 = 0;
		if(no_people_count > 60)//160ms cnt 9600ms
		{
			TOUYINGDENG = 0;
			printf("no people clear count\r\n");//人消失 
			have_people_count = 0;
			no_people_count = 0;
		}
		return;
	}


	if(distance < 300)
	{
		have_people_count ++;
		printf("have people count %d time %d dis %d\r\n",have_people_count, have_people_count*160, distance);//有人出现 
		if(have_people_count > 25)//160ms cnt = 4000ms
		{
			TOUYINGDENG = 1;
			if(sleep_count <= 1)
			{
				need_send = 1;
			}
			LED1 =1;
			have_people_count = 0;
			no_people_count = 0;


			sg_date.floor = __floor;
			sg_date.req = 1;
			sg_date.rsp = 0;
			sg_date.distance = distance;
		}
	}

	
}

 int main(void)
 {	
	u8 len;	
	u16 times=0; 
	u8 work_mode;//工作模式 0控制端 1干接点
	aes_init(AES128key);//AES初始化

	delay_init();	    	 //延时函数初始化	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);// 设置中断优先级分组2
	LED_Init();		  	 //初始化与LED连接的硬件接口 
	KEY_Init();

	work_mode = WORKMODE;
	
	switch (work_mode)
	{
		case 0: //总控制端 
			TOUYINGDENG = 0;
			uart_init(9600);	 //串口初始化为9600  调试使用 
			usart2_init(115200);	 //9600 人体传感器使用
			usart3_init(9600);	 //9600  无线接收发射使用 
			break;
		case 1:
			HUTI = 0;
			uart_init(9600);	 //串口初始化为9600  zigbee使用
			usart3_init(9600);	 //9600  无线接收发射使用 
			break;
	}
	printf("system start %d\r\n",work_mode);
	while(1)
	{
		times++;
		//if(times%30==0)LED0=!LED0;//闪烁LED,提示系统正在运行.
		delay_ms(10); 
		switch (work_mode)
		{
			case 0:
				if(need_send == 0 && USART2_RX_STA&0x8000)//正在发送消息时不处理人体传感器数据 
				{
					len = USART2_RX_STA & 0x3fff;//得到此次接收到的数据长度
					ld2402_recv_process((u8*)&USART2_RX_BUF,len);//处理人体传感器数据 
					mymemset(USART2_RX_BUF,0,USART2_MAX_RECV_LEN);
					USART2_RX_STA=0;
				}else{
					LED0= 1;
				}
				if(USART3_RX_STA&0x8000)
				{
					LED0 = 0;
					len = USART3_RX_STA & 0x3fff;//得到此次接收到的数据长度
					hc13_recv_process((u8*)&USART3_RX_BUF,len);//处理人体传感器数据 
					mymemset(USART3_RX_BUF,0,USART3_MAX_RECV_LEN);
					USART3_RX_STA=0;
				}else{
					LED0= 1;
				}

				if(need_send == 1 && times%10==0)//100ms发送一次
				{
					msg_cnt ++;
					if(msg_cnt >= 10)
					{
						msg_cnt = 0;
						need_send = 0;
					}
					times = 0;
					hc13_send_date(sg_date);
					//delay_ms(100);
				}
				//if(times%30==0)LED0=!LED0;//闪烁LED,提示系统正在运行.
			break;
			case 1:
				if(times%30==0){LED0=!LED0; times = 0;}//闪烁LED,提示系统正在运行.
				if(USART3_RX_STA&0x8000)
				{
					LED0 = 0;
					len = USART3_RX_STA & 0x3fff;//得到此次接收到的数据长度
					hc13_recv_process((u8*)&USART3_RX_BUF,len);//处理人体传感器数据 
					mymemset(USART3_RX_BUF,0,USART3_MAX_RECV_LEN);
					USART3_RX_STA=0;
				}else{
					//LED0= 1;
				}
			break;
		}

	}	 
}


