#ifndef BSP_UART_DMA_H
#define BSP_UART_DMA_H
/*
	如果需要更改串口对应的管脚，请自行修改 bsp_uart_fifo.c文件中的 static void InitHardUart(void)函数
*/

/* 定义使能的串口, 0 表示不使能（不增加代码大小）， 1表示使能 */
/*
	安富莱STM32-V4 串口分配：
	【串口1】 RS232 芯片第1路。
		PA9/USART1_TX	  --- 打印调试口
		PA10/USART1_RX

	【串口2】GPRS 模块 
		PA2/USART2_TX
		PA3/USART2_RX 

	【串口3】 RS485 通信 - TTL 跳线 和 排针
		PB10/USART3_TX
		PB11/USART3_RX
		PB2/BOOT1/RS485_TX_EN

	【串口4】 --- 不做串口用。
	【串口5】 --- 不做串口用。
*/
#define	UART1_FIFO_EN	1
#define	UART2_FIFO_EN	0
#define	UART3_FIFO_EN	0
#define	UART4_FIFO_EN	0
#define	UART5_FIFO_EN	0

/* RS485芯片发送使能GPIO, PB2 */
#define RCC_RS485_TXEN 	 RCC_APB2Periph_GPIOB
#define PORT_RS485_TXEN  GPIOB
#define PIN_RS485_TXEN	 GPIO_Pin_2

#define RS485_RX_EN()	PORT_RS485_TXEN->BRR = PIN_RS485_TXEN
#define RS485_TX_EN()	PORT_RS485_TXEN->BSRR = PIN_RS485_TXEN


/* 定义端口号 */
typedef enum
{
	COM1 = 0,	/* USART1  PA9, PA10 */
	COM2 = 1,	/* USART2, PA2, PA3 */
	COM3 = 2,	/* USART3, PB10, PB11 */
	COM4 = 3,	/* UART4, PC10, PC11 */
	COM5 = 4,	/* UART5, PC12, PD2 */
}COM_PORT_E;

/* 定义串口波特率和FIFO缓冲区大小，分为发送缓冲区和接收缓冲区, 支持全双工 */
#if UART1_FIFO_EN == 1
	#define UART1_BAUD			230400
	#define UART1_TX_BUF_SIZE	1*1024
	#define UART1_RX_BUF_SIZE	1*1024
#endif

typedef struct 
{
    DMA_Channel_TypeDef *channel;
    int oldCounter;
} DmaObjTypeDef_t;

typedef struct 
{
	uint8_t * addr;
	uint16_t len;
}dataInfo_t;

typedef struct{
	int head;
	int tail;
	int left;
	uint16_t size;
	uint8_t *buf;
}bufMangHandle_t;

typedef struct BufMangNode_t
{
	int next_head;
	
	struct BufMangNode_t *next;
}BufMangNode_t;

typedef struct BufMangList_t
{
	struct BufMangNode_t *tail;
	struct BufMangNode_t *next;
}BufMangList_t;

/* 串口设备结构体 */
typedef struct
{
	USART_TypeDef *uart;		/* STM32内部串口设备指针 */
	uint8_t *pTxBuf;			/* 发送缓冲区 */
	uint8_t *pRxBuf;			/* 接收缓冲区 */
	uint16_t usTxBufSize;		/* 发送缓冲区大小 */
	uint16_t usRxBufSize;		/* 接收缓冲区大小 */
	__IO uint16_t usTxWrite;			/* 发送缓冲区写指针 */
	__IO uint16_t usTxRead;			/* 发送缓冲区读指针 */
	__IO uint16_t usTxCount;			/* 等待发送的数据个数 */

	__IO uint16_t usRxWrite;			/* 接收缓冲区写指针 */
	__IO uint16_t usRxRead;			/* 接收缓冲区读指针 */
	__IO uint16_t usRxCount;			/* 还未读取的新数据个数 */

	void (*SendBefor)(void); 	/* 开始发送之前的回调函数指针（主要用于RS485切换到发送模式） */
	void (*SendOver)(void); 	/* 发送完毕的回调函数指针（主要用于RS485将发送模式切换为接收模式） */
	void (*ReciveNew)(uint8_t _byte);	/* 串口收到数据的回调函数指针 */

    DmaObjTypeDef_t txDmaObj;
    DmaObjTypeDef_t rxDmaObj;
}UART_T;

void bsp_InitUart(void);
void comSendBuf(COM_PORT_E _ucPort, uint8_t *_ucaBuf, uint16_t _usLen);
void comSendChar(COM_PORT_E _ucPort, uint8_t _ucByte);
uint8_t comGetChar(COM_PORT_E _ucPort, uint8_t *_pByte);

void comClearTxFifo(COM_PORT_E _ucPort);
void comClearRxFifo(COM_PORT_E _ucPort);

void RS485_SendBuf(uint8_t *_ucaBuf, uint16_t _usLen);
void RS485_SendStr(char *_pBuf);

void bsp_Set485Baud(uint32_t _baud);

void bsp_SetUart1Baud(uint32_t _baud);
void bsp_SetUart2Baud(uint32_t _baud);
void bsp_UartDmaInit(void);

void Send_Test(void);
void do_send(void);

uint8_t InsertNode(BufMangList_t *list,int next_head);
uint8_t FrontDeleteNode(BufMangList_t *list);

#endif