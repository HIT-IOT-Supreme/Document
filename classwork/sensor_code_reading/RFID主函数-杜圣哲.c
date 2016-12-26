#include "include.h"


/**
	郑重声明：
	RFID的东西太多了，n多变量都没有注释，而且都没有被用到？
	所以全程靠猜，而且私以为这个代码不是很干净，删没删利索
	反正我没有咋认真读，凑合看吧
*/


/**
	include.h的内容：

	#include <ioCC2530.h>
	#include <string.h>

	#include "rc522.h"	--	本RFID模块采用MF522芯片，该头文件定义了MF522的一些命令字、寄存器等
	#include "ctrlprocess.h"	--	控制程序，定义了InitAll()、Warn()、Pass()、ctrl_uart()、ctrlprocess()等函数
	#include "timer.h"	--	计时器相关
	#include "uart.h"	--	异步收发传输相关设置
	#include "I2C.h"	--	I2C协议相关操作函数定义
	#include "key.h"	--	
*/


/**
	rc522.h头文件中有定义：
	uchar 为 unsigned char
	uint 为 unsigned int  
*/

uchar PassWd[6] = {0x00};				//定义存储密码的字符数组，初始化为空串	
uchar Read_Data[16] = {0x00};				//定义读数据的字符串	
uchar WriteData[16] = {0x00};				//定义写数据的字符串
uchar RechargeData[4] = {0x00,0x00,0x00,0x00};				//定义充值的字符串
uchar ConsumeData[4] = {0x00,0x00,0x00,0x00};				//定义消费的字符串
uchar RevBuffer[35] = {0x00};				//定义接收数据缓冲
uchar SendBuffer[35] = {0x00,DevType,0x00,0x00,0x00,0x00,0x00,0x00};				//定义发送数据缓冲，第二个字节为设备类型
uchar MLastSelectedSnr[4] = {0x00};				//

uchar uart_count = 0;				//定义异步收发传输字符位计数变量并初始化
uchar uart_comp = 0;				//定义异步收发传输是否开始标识符
uint KeyNum = 0;				//定义卡号变量并初始化
uint KuaiN = 0; 				//定义块号变量并初始化
uchar bWarn = 0;				//定义Warn标识并初始化
uchar bPass = 0;				//定义Pass标识并初始化

uchar KeyTime = 0;				//
uchar WaitTimes = 0;				//

long int SysTime = 0;				//定义一个系统时间变量

uchar oprationcard = 0;				//定义卡片操作类型变量并初始化
uchar bSendID = 0;				//
uchar temp = 0;				//
extern uchar result1;				//

void main(void)				//程序主函数入口
{
	InitAll();				//主要任务是做该模块的初始化
	/**
	void InitAll(void)
	{
	  Init_I2C();				//初始化I2C协议：GPIO口方向、SDA、SCl等
	  Init_IO_AND_LED();				//初始化连接板载的蜂鸣器、LED指示灯等的GPIO口方向
	  InitRc522();				//初始化MF522芯片
	  EA=0;				//关中断
	  init_time3();				//初始化定时器3
	  initUART();				//初始化异步收发传输设置
	  EA=1; //re-enable interrupts  
	  bWarn=0;
	  bPass=0;
	  SysTime=0;
	  KeyTime=0;
	  WaitTimes=0;
	  KeyNum=0;
	  KuaiN=0;
	  oprationcard=0;
	  uart_count=0;
	  uart_comp=0;
	  bSendID=0;
	  Pass();				//Pass()
	}
	*/
	while(1)				//主循环
	{  
		if(bWarn == 1)				//当warn标识为1时
		{
			bWarn = 0;				//清除warn标识
			Warn();				//警告：蜂鸣器响三声
			/**
			void Warn(void)
			{
			  uchar ii;
			  for(ii=0;ii<3;ii++)
			  {
			    SET_BEEP;				//蜂鸣器引脚高电平
			    delay_ms(120);				//延时120毫秒
			    CLR_BEEP;				//蜂鸣器引脚低电平
			    delay_ms(120);
			  }
			}
			*/
		}
		if(bPass == 1)				//如果pass标识为1
		{
			bPass = 0;				//清除pass标识
			Pass();				//延时700毫秒的蜂鸣
			/**
			void Pass(void)
			{
			  SET_BEEP;
			  delay_ms(700);
			  CLR_BEEP;
			}
			*/
		}
		if(uart_comp == 1)
		{
			ctrl_uart();
			/**
			void ctrl_uart(void)
			{
				uchar ii = 0;
				//当收到的消息数据长度为0时（可能是规定好的一个类似握手的连接过程）
				if((RevBuffer[0] == 0x00) && (RevBuffer[2] == 0x00) && (RevBuffer[3] == 0x01))
				{
					//构建一个发送字符串
					SendBuffer[A_MsgLen] = 0x00;				//设置消息长度为0
					SendBuffer[A_DevType] = 0x01;				//设备类型为RFID
					SendBuffer[A_MsgID_0] = 0x00;				//消息ID是0x0010
					SendBuffer[A_MsgID_1] = 0x10;
					SendBuffer[8] = XOR_verify(SendBuffer,8);				//位验证
				
					UartTX_Send_String(SendBuffer, 9);				//发送数据
				}
				//当数据长度不为零时，消息ID后一个字节应与数据部分第一个字节相同
				if((RevBuffer[0] != 0x00) && (RevBuffer[3] == RevBuffer[8]))
				{
					switch(RevBuffer[8])				//判断是哪种消息
					{
						case 0xa0:				//读卡号
							oprationcard=SENDID;				//赋值卡片操作类型变量
							break;
						case 0xa1:				//读数据
							oprationcard=READCARD;
							for(ii=0;ii<6;ii++)
							{
								PassWd[ii]=RevBuffer[ii+9];				//取出信息体中的密码
							} 
					  		KuaiN=RevBuffer[15];				//取出消息体中的块号
					  		break;
						case 0xa2:				//写数据
							oprationcard=WRITECARD;
							for(ii=0;ii<6;ii++)
							{
								PassWd[ii]=RevBuffer[ii+9];
							} 
					  		KuaiN=RevBuffer[15];
							for(ii=0;ii<16;ii++)
							{
								WriteData[ii]=RevBuffer[ii+17];				//取出消息体中的写入数据
							}
							break;  
						case 0xa3:          //消费
							oprationcard=CONSUME;
							for(ii=0;ii<6;ii++)
							{
								PassWd[ii]=RevBuffer[ii+9];
							} 
							KuaiN=RevBuffer[15];
							for(ii=0;ii<4;ii++)
							{
								ConsumeData[ii]=RevBuffer[ii+17];				//取出消费数据
							} 		
							break;
						case 0xa4:          //充值
							oprationcard=ADDMONEY;
							for(ii=0;ii<6;ii++)
							{
								PassWd[ii]=RevBuffer[ii+9];
							} 
							KuaiN=RevBuffer[15];
							for(ii=0;ii<4;ii++)
							{
								RechargeData[ii]=RevBuffer[ii+17];				//取出充值数据
							}
							break;
						default:
							break;                     
					}
				}
				for(ii = 0; ii < 35; ii++)
				{
					RevBuffer[ii] = 0;				//清空缓冲
				}
				uart_comp=0;
				uart_count=0;
			}

			*/
			uart_comp = 0;
		}
		if(SysTime >= 40)				//每过40秒，将执行该函数体内容
		{   
			SysTime = 0;
			YLED = !YLED;
			ctrlprocess();
			/**
			void ctrlprocess(void)
			{
				unsigned char ii;
				char status;
				
				PcdReset();
				
				status=PcdRequest(PICC_REQALL,&RevBuffer[0]);//寻天线区内未进入休眠状态的卡，返回卡片类型 2字节
				
				if(status!=MI_OK)
				{
					return;
				}
				status=PcdAnticoll(&RevBuffer[2]);//防冲撞，返回卡的序列号 4字节
				if(status!=MI_OK)
				{
					return;
				}
				memcpy(MLastSelectedSnr,&RevBuffer[2],4); 
				status=PcdSelect(MLastSelectedSnr);//选卡
				if(status!=MI_OK)
				{
					return;
				}
			  if(oprationcard != 0)
			  {
			   if(oprationcard==READCARD)//读卡片数据
			  {
				oprationcard=0;	
			    status=PcdAuthState(PICC_AUTHENT1A,KuaiN,PassWd,MLastSelectedSnr);//
			    if(status!=MI_OK)
			    {
			      bWarn=1;
			      return;
			    }
			    status=PcdRead(KuaiN,Read_Data);
			    if(status!=MI_OK)
			    {
			      bWarn=1;
			      return;
			    }
					SendBuffer[A_MsgLen] = 0x13;
					SendBuffer[A_MsgID_0] = 0xA1;
					SendBuffer[A_MsgID_1] = 0x00;
					SendBuffer[A_MsgType] = 0xA1;
				
					SendBuffer[9] = KuaiN;
					SendBuffer[10] = 0x10;
					for(ii=0;ii<16;ii++)
				    {
				      SendBuffer[ii + 11] = Read_Data[ii];
				    }
					SendBuffer[27] = XOR_verify(SendBuffer,27);

					UartTX_Send_String(SendBuffer, 28);
			    bPass=1;
			    PcdHalt();
			  }
			  else if(oprationcard==WRITECARD)//写卡
			  {
			     oprationcard=0;
			    status=PcdAuthState(PICC_AUTHENT1A,KuaiN,PassWd,MLastSelectedSnr);//
			    if(status!=MI_OK)
			    {
			      bWarn=1;
			      return;
			    }
			    status=PcdWrite(KuaiN,&WriteData[0]);
			    if(status!=MI_OK)
			    {
			      bWarn=1;
			      return;
			    }	
					SendBuffer[A_MsgLen] = 0x03;
					SendBuffer[A_MsgID_0] = 0xA2;
					SendBuffer[A_MsgID_1] = 0x00;
					SendBuffer[A_MsgType] = 0xA2;
				
					SendBuffer[9] = KuaiN;
					SendBuffer[10] = 0x00;
					SendBuffer[11] = XOR_verify(SendBuffer,11);

					UartTX_Send_String(SendBuffer, 12);
			    bPass=1;
			    PcdHalt();	
			  } 
			  else if(oprationcard==SENDID)//发送卡号
			  {
			     oprationcard=0;
					//发送卡号
					SendBuffer[A_MsgLen] = 0x06;
					SendBuffer[A_MsgID_0] = 0xA0;
					SendBuffer[A_MsgID_1] = 0x00;
					SendBuffer[A_MsgType] = 0xA0;
				
					SendBuffer[9] = 0x04;
					for(ii=0;ii<4;ii++)
				    {
				      SendBuffer[ii + 10] = MLastSelectedSnr[ii];
				    }
					SendBuffer[14] = XOR_verify(SendBuffer,14);

					UartTX_Send_String(SendBuffer, 15);
			    bPass=1;

			  }
			  else if(oprationcard==CONSUME)//消费
			  {
			   	oprationcard=0;
			    status=PcdAuthState(PICC_AUTHENT1A,KuaiN,PassWd,MLastSelectedSnr);
			    if(status!=MI_OK)
			    {
				      bWarn=1;
				      return;
				}	 
				status = PcdValue(PICC_DECREMENT, KuaiN, ConsumeData);
			    if(status!=MI_OK)
			    {
			      bWarn=1;
			      return;
			    }

				status=PcdRead(KuaiN,Read_Data);
			    if(status!=MI_OK)
			    {
			      bWarn=1;
			      return;
			    }
					 
				SendBuffer[A_MsgLen] = 0x07;
				SendBuffer[A_MsgID_0] = 0xA3;
				SendBuffer[A_MsgID_1] = 0x00;
				SendBuffer[A_MsgType] = 0xA3;

				SendBuffer[9] = KuaiN;
				SendBuffer[10] = 0x04;
				for(ii=0;ii<4;ii++)
			    {
			      SendBuffer[ii + 11] = Read_Data[ii];
			    }
				SendBuffer[15] = XOR_verify(SendBuffer,15);		

				UartTX_Send_String(SendBuffer, 16);
				bPass=1;
			    PcdHalt();
				
			  }
				  else if(oprationcard==ADDMONEY)//充值
				  {
					oprationcard=0;	
				    status=PcdAuthState(PICC_AUTHENT1A,KuaiN,PassWd,MLastSelectedSnr);
				    if(status!=MI_OK)
				    {
				      return;
				    }
					status = PcdValue(PICC_INCREMENT, KuaiN, RechargeData);
				    if(status!=MI_OK)
				    {
				      bWarn=1;
			          return;
			        }
					status=PcdRead(KuaiN,Read_Data);
					if(status!=MI_OK)
					{
					  bWarn=1;
					  return;
					}
					SendBuffer[A_MsgLen] = 0x07;
					SendBuffer[A_MsgID_0] = 0xA4;
					SendBuffer[A_MsgID_1] = 0x00;
					SendBuffer[A_MsgType] = 0xA4;
				
					SendBuffer[9] = KuaiN;
					SendBuffer[10] = 0x04;
					for(ii=0;ii<4;ii++)
				    {
				      SendBuffer[ii + 11] = Read_Data[ii];
				    }
					SendBuffer[15] = XOR_verify(SendBuffer,15);		

					UartTX_Send_String(SendBuffer, 16);		
				    bPass=1;
				    PcdHalt();

				  }
				  	//发送跟踪解析帧
			#ifdef DEBUG
					//寻卡 向RC522的FIFO写入数据，该数据将通过RC522发送给卡片
					SendBuffer[A_MsgLen] = 0x05;
					SendBuffer[A_MsgID_0] = 0xA6;
					SendBuffer[A_MsgID_1] = 0x00;
					SendBuffer[A_MsgType] = 0xA6;
				
					SendBuffer[9] = 0x02;
					SendBuffer[10] = 0x09;
					SendBuffer[11] = 0x01;
					SendBuffer[12] = 0x52;
					SendBuffer[13] = XOR_verify(SendBuffer,13);
				
					UartTX_Send_String(SendBuffer, 14);
					//寻卡 向RC522命令寄存器写命令
					SendBuffer[A_MsgLen] = 0x04;
					SendBuffer[A_MsgID_0] = 0xA6;
					SendBuffer[A_MsgID_1] = 0x00;
					SendBuffer[A_MsgType] = 0xA6;
					SendBuffer[9] = 0x01;
					SendBuffer[10] = 0x01;
					SendBuffer[11] = PCD_TRANSCEIVE;
					SendBuffer[12] = XOR_verify(SendBuffer,12);

					UartTX_Send_String(SendBuffer, 13);
					//寻卡 从RC522的FIFO读数据，该数据为卡片发送给RC522的数据
					SendBuffer[A_MsgLen] = 0x06;
					SendBuffer[A_MsgID_0] =  0xA6;
					SendBuffer[A_MsgID_1] =  0x00;
					SendBuffer[A_MsgType] =  0xA6;
				
					SendBuffer[9] = 0x03;
					SendBuffer[10] = 0x09;
					SendBuffer[11] = 0x02;
					SendBuffer[12] = RevBuffer[0];
					SendBuffer[13] = RevBuffer[1];
					SendBuffer[14] = XOR_verify(SendBuffer,14); 

					UartTX_Send_String(SendBuffer, 15);
					//防冲撞 向RC522的FIFO写入数据，该数据将通过RC522发送给卡片
					SendBuffer[A_MsgLen] = 0x06;
					SendBuffer[A_MsgID_0] =  0xA7;
					SendBuffer[A_MsgID_1] =  0x00;
					SendBuffer[A_MsgType] =  0xA7;
				
					SendBuffer[9] = 0x02;
					SendBuffer[10] = 0x09;
					SendBuffer[11] = 0x02;
					SendBuffer[12] = PICC_ANTICOLL1;
					SendBuffer[13] = 0x20;
					SendBuffer[14] = XOR_verify(SendBuffer,14);

					UartTX_Send_String(SendBuffer, 15);
					//防冲撞 向RC522命令寄存器写命令
					SendBuffer[A_MsgLen] = 0x04;
					SendBuffer[A_MsgID_0] = 0xA7;
					SendBuffer[A_MsgID_1] = 0x00;
					SendBuffer[A_MsgType] = 0xA7;
					SendBuffer[9] = 0x01;
					SendBuffer[10] = 0x01;
					SendBuffer[11] = PCD_TRANSCEIVE;
					SendBuffer[12] = XOR_verify(SendBuffer,12);

					UartTX_Send_String(SendBuffer, 13);
					//防冲撞 从RC522的FIFO读数据，该数据为卡片发送给RC522的数据
					SendBuffer[A_MsgLen] = 0x08;
					SendBuffer[A_MsgID_0] =  0xA7;
					SendBuffer[A_MsgID_1] =  0x00;
					SendBuffer[A_MsgType] =  0xA7;
				
					SendBuffer[9] = 0x03;
					SendBuffer[10] = 0x09;
					SendBuffer[11] = 0x04;
					for(ii=0;ii<4;ii++)
				    {
				      SendBuffer[ii + 12] = MLastSelectedSnr[ii];
				    }
					SendBuffer[16] = XOR_verify(SendBuffer,16);

					UartTX_Send_String(SendBuffer, 17);
					//选卡 向RC522的FIFO写入数据，该数据将通过RC522发送给卡片
					SendBuffer[A_MsgLen] = 0x0D;
					SendBuffer[A_MsgID_0] =  0xA8;
					SendBuffer[A_MsgID_1] =  0x00;
					SendBuffer[A_MsgType] =  0xA8;
				
					SendBuffer[9] = 0x02;
					SendBuffer[10] = 0x09;
					SendBuffer[11] = 0x09;
					SendBuffer[12] = PICC_ANTICOLL1;
					SendBuffer[13] = 0x70;
					SendBuffer[18] = 0;
				    for (ii=0; ii<4; ii++)
				    {
				    	SendBuffer[ii+14] = *(MLastSelectedSnr+ii);
				    	SendBuffer[18]  ^= *(MLastSelectedSnr+ii);
				    }
					
				//    CalulateCRC(&SendBuffer[12],7,&SendBuffer[19]);
					SendBuffer[21] = XOR_verify(SendBuffer,21);
				
					UartTX_Send_String(SendBuffer, 22);
					//选卡 向RC522命令寄存器写命令
					SendBuffer[A_MsgLen] = 0x04;
					SendBuffer[A_MsgID_0] = 0xA8;
					SendBuffer[A_MsgID_1] = 0x00;
					SendBuffer[A_MsgType] = 0xA8;
					SendBuffer[9] = 0x01;
					SendBuffer[10] = 0x01;
					SendBuffer[11] = PCD_TRANSCEIVE;
					SendBuffer[12] = XOR_verify(SendBuffer,12);

					UartTX_Send_String(SendBuffer, 13);
			#endif
			    }   					
			}
			*/
		}
	}  
}

