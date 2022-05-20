/*
*********************************************************************************************************
*
*	模块名称 : 主程序入口
*	文件名称 : main.c
*	版    本 : V1.0
*	说    明 : 跑马灯例子。使用了systick中断实现精确定时，控制LED指示灯闪烁频率。
*	修改记录 :
*		版本号  日期       作者    说明
*		V1.0    2015-08-30 armfly  首发
*
*	Copyright (C), 2015-2016, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/

#include "bsp.h" /* 底层硬件驱动 */
#include "includes.h"
/*
**********************************************************************************************************
函数声明
**********************************************************************************************************
*/
static void vTaskLED(void *pvParameters);
static void vTaskStart(void *pvParameters);
static void vTaskPrintSysInfo(void *pvParameters);
static void vTaskUartSend(void *pvParameters);
static void AppTaskCreate(void);
/*
**********************************************************************************************************
变量声明
**********************************************************************************************************
*/
static TaskHandle_t xHandleTaskLED = NULL;
static TaskHandle_t xHandleTaskStart = NULL;
static TaskHandle_t xHandleTaskPrintSysInfo = NULL;
static TaskHandle_t xHandleTaskUartSend = NULL;
/*
*********************************************************************************************************
*	函 数 名: main
*	功能说明: c程序入口
*	形    参：无
*	返 回 值: 错误代码(无需处理)
*********************************************************************************************************
*/
int main(void)
{

	bsp_Init(); /* 硬件初始化 */
	//myprintf("Welcome to Solar's experiment\r\n");
	AppTaskCreate();
	/* 启动调度，开始执行任务 */
	vTaskStartScheduler();
	/*
	如果系统正常启动是不会运行到这里的，运行到这里极有可能是用于定时器任务或者空闲任务的
	heap 空间不足造成创建失败，此要加大 FreeRTOSConfig.h 文件中定义的 heap 大小：
	#define configTOTAL_HEAP_SIZE ( ( size_t ) ( 17 * 1024 ) )
   */
	while (1)
		;
}

/*
*********************************************************************************************************
* 函 数 名: AppTaskCreate
* 功能说明: 创建应用任务
* 形 参: 无 * 返 回 值: 无
*********************************************************************************************************
*/
static void AppTaskCreate(void)
{

	xTaskCreate(vTaskLED,		  /* 任务函数 */
				"vTaskLED",		  /* 任务名 */
				500,			  /* 任务栈大小，单位 word，也就是 4 字节 */
				NULL,			  /* 任务参数 */
				2,				  /* 任务优先级*/
				&xHandleTaskLED); /* 任务句柄 */
								  // xTaskCreate(vTaskStart,			 /* 任务函数 */
								  // 			"vTaskStart",		 /* 任务名 */
								  // 			512,				 /* 任务栈大小，单位 word，也就是 4 字节 */
								  // 			NULL,				 /* 任务参数 */
								  // 			4,					 /* 任务优先级*/
								  // 			&xHandleTaskStart);	 /* 任务句柄 */
	
	// xTaskCreate(vTaskPrintSysInfo,		  /* 任务函数 */
	// 			"vTaskPrintSysInfo",		  /* 任务名 */
	// 			512,			  /* 任务栈大小，单位 word，也就是 4 字节 */
	// 			NULL,			  /* 任务参数 */
	// 			2,				  /* 任务优先级*/
	// 			&xHandleTaskPrintSysInfo); /* 任务句柄 */


	xTaskCreate(vTaskUartSend,		  /* 任务函数 */
				"vTaskUartSend",		  /* 任务名 */
				512,			  /* 任务栈大小，单位 word，也就是 4 字节 */
				NULL,			  /* 任务参数 */
				2,				  /* 任务优先级*/
				&xHandleTaskUartSend); /* 任务句柄 */
}

static void vTaskStart(void *pvParameters)
{
	/* 状态机大循环 */
	while (1)
	{
		bsp_LedToggle(1);
		vTaskDelay(500);
	}
}

static void vTaskPrintSysInfo(void *pvParameters)
{
	uint8_t pcWriteBuffer[500];
	/* 状态机大循环 */
	while (1)
	{
		myprintf("=================================================\r\n");
		myprintf("任务名 任务状态 优先级 剩余栈 任务序号\r\n");
		vTaskList((char *)&pcWriteBuffer);
		myprintf("%s\r\n", pcWriteBuffer);
		myprintf("\r\n 任务名 运行计数 使用率\r\n");
		vTaskGetRunTimeStats((char *)&pcWriteBuffer);
		myprintf("%s\r\n", pcWriteBuffer);
		vTaskDelay(1000);
	}
} 

static void vTaskLED(void *pvParameters)
{
	/* 状态机大循环 */
	while (1)
	{
		//Send_Test();
		
		bsp_LedToggle(1);
		vTaskDelay(500);
	}
}

static void vTaskUartSend(void *pvParameters)
{
	while (1)
	{
		do_send();
		vTaskDelay(10);
	}
}