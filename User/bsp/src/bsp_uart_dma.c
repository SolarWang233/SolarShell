/*
*********************************************************************************************************
*
*	模块名称 : 串口中断+FIFO驱动模块
*	文件名称 : bsp_uart_fifo.c
*	版    本 : V1.0
*	说    明 : 采用串口中断+FIFO模式实现多个串口的同时访问
*	修改记录 :
*		版本号  日期       作者    说明
*		V1.0    2013-02-01 armfly  正式发布
*		V1.1    2013-06-09 armfly  FiFo结构增加TxCount成员变量，方便判断缓冲区满; 增加 清FiFo的函数
*		V1.2	2014-09-29 armfly  增加RS485 MODBUS接口。接收到新字节后，直接执行回调函数。
*
*	修改补充 :
*		版本号    日期       作者                  说明
*		V1.0    2015-08-19  Eric2013       临界区处理采用FreeRTOS方案
*
*	Copyright (C), 2013-2014, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/

#include "bsp.h"
#include "stdarg.h"

/* 定义每个串口结构体变量 */
#if UART1_FIFO_EN == 1
static UART_T g_tUart1;
static uint8_t g_TxBuf1[UART1_TX_BUF_SIZE]; /* 发送缓冲区 */
static uint8_t g_RxBuf1[UART1_RX_BUF_SIZE]; /* 接收缓冲区 */
#endif
dataInfo_t dataInfo;
bufMangHandle_t bufMangHandle;
BufMangList_t BufMangList = {NULL,NULL} ;
u8 ok = 1;

static void UartVarInit(void);

static void InitHardUart(void);
static void UartSend(UART_T *_pUart, uint8_t *_ucaBuf, uint16_t _usLen);
static uint8_t UartGetChar(UART_T *_pUart, uint8_t *_pByte);
static void UartIRQ(UART_T *_pUart);
static void ConfigUartNVIC(void);
static void UartSendDma(UART_T *_pUart, uint8_t *_ucaBuf, uint16_t _usLen);
void do_send(void);
void bufMangInit(bufMangHandle_t *q,u8* buf,u16 len);
void RS485_InitTXE(void);
static void do_irqHandle(bufMangHandle_t *q);

/*
*********************************************************************************************************
*	函 数 名: bsp_InitUart
*	功能说明: 初始化串口硬件，并对全局变量赋初值.
*	形    参:  无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_InitUart(void)
{
	// UartVarInit();		/* 必须先初始化全局变量,再配置硬件 */

	// InitHardUart();		/* 配置串口的硬件参数(波特率等) */

	// RS485_InitTXE();	/* 配置RS485芯片的发送使能硬件，配置为推挽输出 */

	bufMangInit(&bufMangHandle,g_TxBuf1,UART1_TX_BUF_SIZE);
	bsp_UartDmaInit();

	ConfigUartNVIC(); /* 配置串口中断 */
}

/*
*********************************************************************************************************
*	函 数 名: ComToUart
*	功能说明: 将COM端口号转换为UART指针
*	形    参: _ucPort: 端口号(COM1 - COM6)
*	返 回 值: uart指针
*********************************************************************************************************
*/
UART_T *ComToUart(COM_PORT_E _ucPort)
{
	if (_ucPort == COM1)
	{
#if UART1_FIFO_EN == 1
		return &g_tUart1;
#else
		return 0;
#endif
	}
	else
	{
		/* 不做任何处理 */
		return 0;
	}
}

/*
*********************************************************************************************************
*	函 数 名: comSendBuf
*	功能说明: 向串口发送一组数据。数据放到发送缓冲区后立即返回，由中断服务程序在后台完成发送
*	形    参: _ucPort: 端口号(COM1 - COM6)
*			  _ucaBuf: 待发送的数据缓冲区
*			  _usLen : 数据长度
*	返 回 值: 无
*********************************************************************************************************
*/
void comSendBuf(COM_PORT_E _ucPort, uint8_t *_ucaBuf, uint16_t _usLen)
{
	UART_T *pUart;

	pUart = ComToUart(_ucPort);
	if (pUart == 0)
	{
		return;
	}

	if (pUart->SendBefor != 0)
	{
		pUart->SendBefor(); /* 如果是RS485通信，可以在这个函数中将RS485设置为发送模式 */
	}

	UartSend(pUart, _ucaBuf, _usLen);
}

/*
*********************************************************************************************************
*	函 数 名: comSendChar
*	功能说明: 向串口发送1个字节。数据放到发送缓冲区后立即返回，由中断服务程序在后台完成发送
*	形    参: _ucPort: 端口号(COM1 - COM6)
*			  _ucByte: 待发送的数据
*	返 回 值: 无
*********************************************************************************************************
*/
void comSendChar(COM_PORT_E _ucPort, uint8_t _ucByte)
{
	comSendBuf(_ucPort, &_ucByte, 1);
}

/*
*********************************************************************************************************
*	函 数 名: comGetChar
*	功能说明: 从串口缓冲区读取1字节，非阻塞。无论有无数据均立即返回
*	形    参: _ucPort: 端口号(COM1 - COM6)
*			  _pByte: 接收到的数据存放在这个地址
*	返 回 值: 0 表示无数据, 1 表示读取到有效字节
*********************************************************************************************************
*/
uint8_t comGetChar(COM_PORT_E _ucPort, uint8_t *_pByte)
{
	UART_T *pUart;

	pUart = ComToUart(_ucPort);
	if (pUart == 0)
	{
		return 0;
	}

	return UartGetChar(pUart, _pByte);
}

/*
*********************************************************************************************************
*	函 数 名: comClearTxFifo
*	功能说明: 清零串口发送缓冲区
*	形    参: _ucPort: 端口号(COM1 - COM6)
*	返 回 值: 无
*********************************************************************************************************
*/
void comClearTxFifo(COM_PORT_E _ucPort)
{
	UART_T *pUart;

	pUart = ComToUart(_ucPort);
	if (pUart == 0)
	{
		return;
	}

	pUart->usTxWrite = 0;
	pUart->usTxRead = 0;
	pUart->usTxCount = 0;
}

/*
*********************************************************************************************************
*	函 数 名: comClearRxFifo
*	功能说明: 清零串口接收缓冲区
*	形    参: _ucPort: 端口号(COM1 - COM6)
*	返 回 值: 无
*********************************************************************************************************
*/
void comClearRxFifo(COM_PORT_E _ucPort)
{
	UART_T *pUart;

	pUart = ComToUart(_ucPort);
	if (pUart == 0)
	{
		return;
	}

	pUart->usRxWrite = 0;
	pUart->usRxRead = 0;
	pUart->usRxCount = 0;
}

/*
*********************************************************************************************************
*	函 数 名: bsp_SetUart1Baud
*	功能说明: 修改UART1波特率
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_SetUart1Baud(uint32_t _baud)
{
	USART_InitTypeDef USART_InitStructure;

	/* 第2步： 配置串口硬件参数 */
	USART_InitStructure.USART_BaudRate = _baud; /* 波特率 */
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStructure);
}

/*
*********************************************************************************************************
*	函 数 名: UartSend
*	功能说明: 填写数据到UART发送缓冲区,并启动发送中断。中断处理函数发送完毕后，自动关闭发送中断
*	形    参:  无
*	返 回 值: 无
*********************************************************************************************************
*/
static void UartSend(UART_T *_pUart, uint8_t *_ucaBuf, uint16_t _usLen)
{
	uint16_t i;

	for (i = 0; i < _usLen; i++)
	{
		/* 如果发送缓冲区已经满了，则等待缓冲区空 */
	#if 0
		/*
			在调试GPRS例程时，下面的代码出现死机，while 死循环
			原因： 发送第1个字节时 _pUart->usTxWrite = 1；_pUart->usTxRead = 0;
			将导致while(1) 无法退出
		*/
		while (1)
		{
			uint16_t usRead;

			DISABLE_INT();
			usRead = _pUart->usTxRead;
			ENABLE_INT();

			if (++usRead >= _pUart->usTxBufSize)
			{
				usRead = 0;
			}

			if (usRead != _pUart->usTxWrite)
			{
				break;
			}
		}
	#else
		/* 当 _pUart->usTxBufSize == 1 时, 下面的函数会死掉(待完善) */
		while (1)
		{
			__IO uint16_t usCount;

			DISABLE_INT();
			usCount = _pUart->usTxCount;
			ENABLE_INT();

			if (usCount < _pUart->usTxBufSize)
			{
				break;
			}
		}
	#endif

		/* 将新数据填入发送缓冲区 */
		_pUart->pTxBuf[_pUart->usTxWrite] = _ucaBuf[i];

		DISABLE_INT();
		if (++_pUart->usTxWrite >= _pUart->usTxBufSize)
		{
			_pUart->usTxWrite = 0;
		}
		_pUart->usTxCount++;
		ENABLE_INT();
	}

	USART_ITConfig(_pUart->uart, USART_IT_TXE, ENABLE);
}

/*
*********************************************************************************************************
*	函 数 名: bsp_SetUart2Baud
*	功能说明: 修改UART2波特率
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_SetUart2Baud(uint32_t _baud)
{
	USART_InitTypeDef USART_InitStructure;

	/* 第2步： 配置串口硬件参数 */
	USART_InitStructure.USART_BaudRate = _baud; /* 波特率 */
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStructure);
}

/* 如果是RS485通信，请按如下格式编写函数， 我们仅举了 USART3作为RS485的例子 */

/*
*********************************************************************************************************
*	函 数 名: RS485_InitTXE
*	功能说明: 配置RS485发送使能口线 TXE
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void RS485_InitTXE(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_RS485_TXEN, ENABLE); /* 打开GPIO时钟 */

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; /* 推挽输出模式 */
	GPIO_InitStructure.GPIO_Pin = PIN_RS485_TXEN;
	GPIO_Init(PORT_RS485_TXEN, &GPIO_InitStructure);
}

/*
*********************************************************************************************************
*	函 数 名: bsp_Set485Baud
*	功能说明: 修改UART3波特率
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_Set485Baud(uint32_t _baud)
{
	USART_InitTypeDef USART_InitStructure;

	/* 第2步： 配置串口硬件参数 */
	USART_InitStructure.USART_BaudRate = _baud; /* 波特率 */
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART3, &USART_InitStructure);
}

/*
*********************************************************************************************************
*	函 数 名: RS485_SendBefor
*	功能说明: 发送数据前的准备工作。对于RS485通信，请设置RS485芯片为发送状态，
*			  并修改 UartVarInit()中的函数指针等于本函数名，比如 g_tUart2.SendBefor = RS485_SendBefor
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void RS485_SendBefor(void)
{
	RS485_TX_EN(); /* 切换RS485收发芯片为发送模式 */
}

/*
*********************************************************************************************************
*	函 数 名: RS485_SendOver
*	功能说明: 发送一串数据结束后的善后处理。对于RS485通信，请设置RS485芯片为接收状态，
*			  并修改 UartVarInit()中的函数指针等于本函数名，比如 g_tUart2.SendOver = RS485_SendOver
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void RS485_SendOver(void)
{
	RS485_RX_EN(); /* 切换RS485收发芯片为接收模式 */
}

/*
*********************************************************************************************************
*	函 数 名: RS485_SendBuf
*	功能说明: 通过RS485芯片发送一串数据。注意，本函数不等待发送完毕。
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void RS485_SendBuf(uint8_t *_ucaBuf, uint16_t _usLen)
{
	comSendBuf(COM3, _ucaBuf, _usLen);
}

/*
*********************************************************************************************************
*	函 数 名: RS485_SendStr
*	功能说明: 向485总线发送一个字符串
*	形    参: _pBuf 数据缓冲区
*			 _ucLen 数据长度
*	返 回 值: 无
*********************************************************************************************************
*/
void RS485_SendStr(char *_pBuf)
{
	RS485_SendBuf((uint8_t *)_pBuf, strlen(_pBuf));
}

/*
*********************************************************************************************************
*	函 数 名: RS485_ReciveNew
*	功能说明: 接收到新的数据
*	形    参: _byte 接收到的新数据
*	返 回 值: 无
*********************************************************************************************************
*/
// extern void MODBUS_ReciveNew(uint8_t _byte);
void RS485_ReciveNew(uint8_t _byte)
{
	//	MODBUS_ReciveNew(_byte);
}

/*
*********************************************************************************************************
*	函 数 名: UartVarInit
*	功能说明: 初始化串口相关的变量
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void UartVarInit(void)
{
#if UART1_FIFO_EN == 1
	g_tUart1.uart = USART1;					  /* STM32 串口设备 */
	g_tUart1.pTxBuf = g_TxBuf1;				  /* 发送缓冲区指针 */
	g_tUart1.pRxBuf = g_RxBuf1;				  /* 接收缓冲区指针 */
	g_tUart1.usTxBufSize = UART1_TX_BUF_SIZE; /* 发送缓冲区大小 */
	g_tUart1.usRxBufSize = UART1_RX_BUF_SIZE; /* 接收缓冲区大小 */
	g_tUart1.usTxWrite = 0;					  /* 发送FIFO写索引 */
	g_tUart1.usTxRead = 0;					  /* 发送FIFO读索引 */
	g_tUart1.usRxWrite = 0;					  /* 接收FIFO写索引 */
	g_tUart1.usRxRead = 0;					  /* 接收FIFO读索引 */
	g_tUart1.usRxCount = 0;					  /* 接收到的新数据个数 */
	g_tUart1.usTxCount = 0;					  /* 待发送的数据个数 */
	g_tUart1.SendBefor = 0;					  /* 发送数据前的回调函数 */
	g_tUart1.SendOver = 0;					  /* 发送完毕后的回调函数 */
	g_tUart1.ReciveNew = 0;					  /* 接收到新数据后的回调函数 */
	g_tUart1.txDmaObj.channel = DMA1_Channel4;
	g_tUart1.txDmaObj.oldCounter = 0;
	g_tUart1.rxDmaObj.channel = DMA1_Channel5;
	g_tUart1.rxDmaObj.oldCounter = 0;
#endif

}

/*
*********************************************************************************************************
*	函 数 名: InitHardUart
*	功能说明: 配置串口的硬件参数（波特率，数据位，停止位，起始位，校验位，中断使能）适合于STM32-F4开发板
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void InitHardUart(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;

#if UART1_FIFO_EN == 1 /* 串口1 TX = PA9   RX = PA10 或 TX = PB6   RX = PB7*/

	/* 第1步：打开GPIO和USART部件的时钟 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	/* 第2步：将USART Tx的GPIO配置为推挽复用模式 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* 第3步：将USART Rx的GPIO配置为浮空输入模式
		由于CPU复位后，GPIO缺省都是浮空输入模式，因此下面这个步骤不是必须的
		但是，我还是建议加上便于阅读，并且防止其它地方修改了这个口线的设置参数
	*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* 第4步： 配置串口硬件参数 */
	USART_InitStructure.USART_BaudRate = UART1_BAUD; /* 波特率 */
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);

	// USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);	/* 使能接收中断 */
	USART_ITConfig(USART1, USART_IT_IDLE, ENABLE); /* 使能接收中断 */
	/*
		USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
		注意: 不要在此处打开发送中断
		发送中断使能在SendUart()函数打开
	*/
	USART_Cmd(USART1, ENABLE);					   /* 使能串口 */
	USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE); // RX使能DMA
	USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE); // RX使能DMA
	/* CPU的小缺陷：串口配置好，如果直接Send，则第1个字节发送不出去
		如下语句解决第1个字节无法正确发送出去的问题 */
	USART_ClearFlag(USART1, USART_FLAG_TC); /* 清发送完成标志，Transmission Complete flag */
#endif
}

void bsp_UartDmaInit(void)
{
	UartVarInit();
	InitHardUart();
	bsp_dmaConfig(DMA1_Channel4, (u32)&g_tUart1.uart->DR, (u32)g_tUart1.pTxBuf, g_tUart1.usTxBufSize, 0, 0);
	// bsp_dmaEnable(DMA1_Channel4,g_tUart1.usTxBufSize);
	bsp_dmaConfig(DMA1_Channel5, (u32)&g_tUart1.uart->DR, (u32)g_tUart1.pRxBuf, g_tUart1.usRxBufSize, 1, 1);
	bsp_dmaEnable(DMA1_Channel5, g_tUart1.usRxBufSize);
	DMA_ITConfig(DMA1_Channel4,DMA_IT_TC,ENABLE);
	
	NVIC_InitTypeDef NVIC_InitStructure;


	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

}

void DMA1_Channel4_IRQHandler(void)
{
	if (DMA_GetITStatus(DMA1_IT_TC4) != RESET)
	{
		DMA_ClearFlag(DMA1_IT_TC4);//清除通道4传输完成标志

		do_irqHandle(&bufMangHandle);
	}
}
/*
*********************************************************************************************************
*	函 数 名: ConfigUartNVIC
*	功能说明: 配置串口硬件中断.
*	形    参:  无
*	返 回 值: 无
*********************************************************************************************************
*/
static void ConfigUartNVIC(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	/* Configure the NVIC Preemption Priority Bits */
	/*	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);  --- 在 bsp.c 中 bsp_Init() 中配置中断优先级组 */

#if UART1_FIFO_EN == 1
	/* 使能串口1中断 */
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
#endif

}

/*
*************************************************************************
buffer 管理
*************************************************************************
*/

void bufMangInit(bufMangHandle_t *q,u8* buf,u16 len)
{
	q->size = len;
	q->head = 0;
	q->tail = 0;
	q->left = len;
	q->buf = buf;
}	

uint8_t isEmpty(bufMangHandle_t *q)
{
	if(q->head == q->tail)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

uint8_t isMatchMe(bufMangHandle_t *q,dataInfo_t *dataInfo)
{
	if(q -> left > dataInfo->len)
	{
		return 1;
	}
	else 
	{
		return 0;
	}
}


uint8_t putDataInBuf(bufMangHandle_t *q,dataInfo_t *dataInfo)
{
	if(isMatchMe(q,dataInfo))
	{
		if(q->tail+dataInfo->len >= q->size)
		{
			
			//memcpy((q->buf+q->tail),dataInfo->addr,q->size - q->tail);
			for(int i = 0; i < q->size - q->tail;i++)
			{
				*(q->buf+q->tail + i) = *(dataInfo->addr + i);
			}
			
			//memcpy((q->buf),dataInfo->addr+q->size - q->tail,dataInfo->len- (q->size - q->tail));
			for(int i = 0;i < dataInfo->len- (q->size - q->tail);i++)
			{
				 *(q->buf + i) = *(dataInfo->addr+q->size - q->tail + i);
			}

		}
		else
		{
			//memcpy((q->buf+q->tail),dataInfo->addr,dataInfo->len);
			for(int i = 0;i<dataInfo->len;i++)
			{
				*(q->buf+q->tail + i) = *(dataInfo->addr + i);
			}
		}
//			if(data[0]!='o'||data[1]!='j'||data[2]!='b'||data[3]!='k'||data[4]!=0x0d||data[5]!=0x0a)
//			{
//				myprintf("bad boy");
//			}
		
		
		q-> tail = (q->tail + dataInfo->len) % q->size;
		q->left = q->size - (q->tail - q->head + q->size)% q->size;
		InsertNode( &BufMangList,q-> tail);
	}
	else 
	{
		return 0;
	}
	return 1;
}

uint8_t getDataFromBuf(UART_T *_pUart,bufMangHandle_t *q)
{
	u32 addr;
	u32 len;
	u8 *_addr;
	int head = q->head;
	int tail = q->tail;
	if(isEmpty(q) == 0)
	{

		//到工作区
		addr = (u32)q->buf + head;
		_addr = (u8 *)addr;
		if(tail > head)
		{
			len = (tail - head + q->size) % q->size;  
			UartSendDma(_pUart,(u8 *)addr,len);
		}
		else
		{
			UartSendDma(_pUart,(u8 *)addr,q->size-head);
			UartSendDma(_pUart,(u8 *)q->buf ,tail);
		}

		
		
		//if(_addr[0]!='o'||_addr[1]!='j'||_addr[2]!='b'||_addr[3]!='k'||_addr[4]!=0x0d||_addr[5]!=0x0a)
		{
			
		}
	}
	else 
	{
		return 0; 
	}

	
	return 1;
}

static void do_irqHandle(bufMangHandle_t *q)
{
	
	q->head = BufMangList.next->next_head; 
	FrontDeleteNode(&BufMangList);
	//q->head = q->tail; 
	q->left = q->size - (q->tail - q->head + q->size)% q->size;
	ok = 1;
}

void do_send(void)
{
	getDataFromBuf(&g_tUart1,&bufMangHandle);
}







uint16_t overtime;

static void UartSendDma(UART_T *_pUart, uint8_t *_ucaBuf, uint16_t _usLen)
{
	overtime = 0;
	if(ok ==1)
	{
			bsp_dmaSetAddrLen(_pUart->txDmaObj.channel,(u32)_ucaBuf,_usLen);
		//memcpy(_pUart->pTxBuf,(const void *)_ucaBuf,_usLen);
		bsp_dmaEnable(_pUart->txDmaObj.channel,_usLen);
		ok = 0;
	}

//	while(1)
//	{
//		if(overtime++>5000)
//		{
//			break;
//		}
//		if(DMA_GetITStatus(DMA1_IT_TC4)!=RESET)	//判断通道4传输完成
//		{
//			DMA_ClearFlag(DMA1_IT_TC4);//清除通道4传输完成标志
//			break;
//		}
//	}

}




/*
*********************************************************************************************************
*	函 数 名: UartGetChar
*	功能说明: 从串口接收缓冲区读取1字节数据 （用于主程序调用）
*	形    参: _pUart : 串口设备
*			  _pByte : 存放读取数据的指针
*	返 回 值: 0 表示无数据  1表示读取到数据
*********************************************************************************************************
*/
static uint8_t UartGetChar(UART_T *_pUart, uint8_t *_pByte)
{
	uint16_t usCount;

	/* usRxWrite 变量在中断函数中被改写，主程序读取该变量时，必须进行临界区保护 */
	DISABLE_INT();
	usCount = _pUart->usRxCount;
	ENABLE_INT();

	/* 如果读和写索引相同，则返回0 */
	// if (_pUart->usRxRead == usRxWrite)
	if (usCount == 0) /* 已经没有数据 */
	{
		return 0;
	}
	else
	{
		*_pByte = _pUart->pRxBuf[_pUart->usRxRead]; /* 从串口接收FIFO取1个数据 */

		/* 改写FIFO读索引 */
		DISABLE_INT();
		if (++_pUart->usRxRead >= _pUart->usRxBufSize)
		{
			_pUart->usRxRead = 0;
		}
		_pUart->usRxCount--;
		ENABLE_INT();
		return 1;
	}
}

uint16_t nowCounter;
uint16_t oldCounter;
uint16_t bufLen;
void bsp_getInputFromBuffer(UART_T *_pUart,dataInfo_t *dataInfo)
{
	nowCounter = _pUart->usRxBufSize - DMA_GetCurrDataCounter(_pUart->rxDmaObj.channel);
	oldCounter = _pUart->rxDmaObj.oldCounter;
	
//	dataInfo_t *pOutput;
	if(nowCounter > oldCounter)
	{
		bufLen =  nowCounter - oldCounter;
		dataInfo->addr = (u8 *)malloc(bufLen);
		dataInfo->len = bufLen;
		if(dataInfo->addr!=NULL)
		{
			memcpy(dataInfo->addr,&_pUart->pRxBuf[oldCounter],bufLen);
		}
	}
	else
	{
		bufLen = _pUart->usRxBufSize - oldCounter  + nowCounter;
		dataInfo->addr = (u8 *)malloc(bufLen);
		dataInfo->len = bufLen;
		if(dataInfo->addr!=NULL)
		{
			memcpy(dataInfo->addr,&_pUart->pRxBuf[oldCounter],_pUart->usRxBufSize - oldCounter);
			memcpy(dataInfo->addr+_pUart->usRxBufSize - oldCounter,_pUart->pRxBuf,nowCounter);
		}
	}
	_pUart->rxDmaObj.oldCounter = nowCounter;
//	if(nowCounter >240)
//	{
//		myprintf("i got you\r\n");
//	}
	//myprintf("%s",dataInfo->addr);

	{
//		myprintf("%s,len = %d\r\n",dataInfo->addr,bufLen);
		
		putDataInBuf(&bufMangHandle,dataInfo);
	}
	
	//UartSendDma(_pUart,dataInfo->addr,bufLen);
	free(dataInfo->addr);
}






uint8_t InsertNode(BufMangList_t *list,int next_head)
{
	BufMangNode_t *newNode;
	newNode = (BufMangNode_t *)malloc(sizeof(BufMangNode_t));
	if(newNode == NULL)
	{
		return 0;
	}
	newNode->next = NULL;
	newNode->next_head = next_head;
	if(list->tail!= NULL)
	{
		list->tail->next = newNode;
	}
	else
	{
		list->next = newNode;
	}
	list->tail = newNode;
	
	

	return 1;
}

uint8_t FrontDeleteNode(BufMangList_t *list)
{
	BufMangNode_t *p;
	p = list->next;
	if(p)
	{
		list->next = p->next;
		if(p->next == NULL)
		{
			list->tail = NULL;
		}
		free(p);
		return 1;
	}
	else
	{
		list->next = NULL;
		list->tail = NULL;
	}
	
	return 0;
	
}

//uint8_t InitList(BufMangList_t *list)
//{
//	if(CreatNode(list,0,0))
//	{
//		return 1;
//	}
//	else
//	{
//		return 0;
//	}
//}



/*
*********************************************************************************************************
*	函 数 名: UartIRQ
*	功能说明: 供中断服务程序调用，通用串口中断处理函数
*	形    参: _pUart : 串口设备
*	返 回 值: 无
*********************************************************************************************************
*/
static void UartIRQ(UART_T *_pUart)
{
	/* 处理接收中断  */
	// if (USART_GetITStatus(_pUart->uart, USART_IT_RXNE) != RESET)
	// {
	// 	/* 从串口接收数据寄存器读取数据存放到接收FIFO */
	// 	uint8_t ch;

	// 	ch = USART_ReceiveData(_pUart->uart);
	// 	_pUart->pRxBuf[_pUart->usRxWrite] = ch;
	// 	if (++_pUart->usRxWrite >= _pUart->usRxBufSize)
	// 	{
	// 		_pUart->usRxWrite = 0;
	// 	}
	// 	if (_pUart->usRxCount < _pUart->usRxBufSize)
	// 	{
	// 		_pUart->usRxCount++;
	// 	}

	// 	/* 回调函数,通知应用程序收到新数据,一般是发送1个消息或者设置一个标记 */
	// 	//if (_pUart->usRxWrite == _pUart->usRxRead)
	// 	//if (_pUart->usRxCount == 1)
	// 	{
	// 		if (_pUart->ReciveNew)
	// 		{
	// 			_pUart->ReciveNew(ch);
	// 		}
	// 	}
	// }

	//空闲中断
	if (USART_GetITStatus(_pUart->uart, USART_IT_IDLE) != RESET)
	{
		u8 clear;
		clear = _pUart->uart->SR;
		clear = _pUart->uart->DR; //读这两个寄存器来清除空闲中断标志位

		bsp_getInputFromBuffer(_pUart,&dataInfo);
	}

	/* 处理发送缓冲区空中断 */
	if (USART_GetITStatus(_pUart->uart, USART_IT_TXE) != RESET)
	{
		// if (_pUart->usTxRead == _pUart->usTxWrite)
		if (_pUart->usTxCount == 0)
		{
			/* 发送缓冲区的数据已取完时， 禁止发送缓冲区空中断 （注意：此时最后1个数据还未真正发送完毕）*/
			USART_ITConfig(_pUart->uart, USART_IT_TXE, DISABLE);

			/* 使能数据发送完毕中断 */
			USART_ITConfig(_pUart->uart, USART_IT_TC, ENABLE);
		}
		else
		{
			/* 从发送FIFO取1个字节写入串口发送数据寄存器 */
			USART_SendData(_pUart->uart, _pUart->pTxBuf[_pUart->usTxRead]);
			if (++_pUart->usTxRead >= _pUart->usTxBufSize)
			{
				_pUart->usTxRead = 0;
			}
			_pUart->usTxCount--;
		}
	}

	/* 数据bit位全部发送完毕的中断 */
	else if (USART_GetITStatus(_pUart->uart, USART_IT_TC) != RESET)
	{
		// if (_pUart->usTxRead == _pUart->usTxWrite)
		if (_pUart->usTxCount == 0)
		{
			/* 如果发送FIFO的数据全部发送完毕，禁止数据发送完毕中断 */
			USART_ITConfig(_pUart->uart, USART_IT_TC, DISABLE);

			/* 回调函数, 一般用来处理RS485通信，将RS485芯片设置为接收模式，避免抢占总线 */
			if (_pUart->SendOver)
			{
				_pUart->SendOver();
			}
		}
		else
		{
			/* 正常情况下，不会进入此分支 */

			/* 如果发送FIFO的数据还未完毕，则从发送FIFO取1个数据写入发送数据寄存器 */
			USART_SendData(_pUart->uart, _pUart->pTxBuf[_pUart->usTxRead]);
			if (++_pUart->usTxRead >= _pUart->usTxBufSize)
			{
				_pUart->usTxRead = 0;
			}
			_pUart->usTxCount--;
		}
	}
}





/*
*********************************************************************************************************
*	函 数 名: USART1_IRQHandler  USART2_IRQHandler USART3_IRQHandler UART4_IRQHandler UART5_IRQHandler
*	功能说明: USART中断服务程序
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/

void USART1_IRQHandler(void)
{
	UartIRQ(&g_tUart1);
}

/*
*********************************************************************************************************
*	函 数 名: fputc
*	功能说明: 重定义putc函数，这样可以使用printf函数从串口1打印输出
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
int fputc(int ch, FILE *f)
{
#if 1 /* 将需要printf的字符通过串口中断FIFO发送出去，printf函数会立即返回 */
	comSendChar(COM1, ch);

	return ch;
#else /* 采用阻塞方式发送每个字符,等待数据发送完毕 */
	/* 写一个字节到USART1 */
	USART_SendData(USART1, (uint8_t)ch);

	/* 等待发送结束 */
	while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
	{
	}

	return ch;
#endif
}
void myprintf(const char *format, ...)
{
	uint32_t length;
	va_list args;
	static u8 buf[512];
	u8 len = g_tUart1.usTxBufSize;

	va_start(args, format);
	length = vsnprintf((char *)buf, len, (char *)format, args);
	va_end(args);
	UartSendDma(&g_tUart1, buf, length);
}

/*
*********************************************************************************************************
*	函 数 名: fgetc
*	功能说明: 重定义getc函数，这样可以使用getchar函数从串口1输入数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
int fgetc(FILE *f)
{

#if 1 /* 从串口接收FIFO中取1个数据, 只有取到数据才返回 */
	uint8_t ucData;

	while (comGetChar(COM1, &ucData) == 0)
		;

	return ucData;
#else
	/* 等待串口1输入数据 */
	while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET)
		;

	return (int)USART_ReceiveData(USART1);
#endif
}

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
