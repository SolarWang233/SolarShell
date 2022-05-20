/*
*********************************************************************************************************
*
*	ģ������ : ���������
*	�ļ����� : main.c
*	��    �� : V1.0
*	˵    �� : ��������ӡ�ʹ����systick�ж�ʵ�־�ȷ��ʱ������LEDָʾ����˸Ƶ�ʡ�
*	�޸ļ�¼ :
*		�汾��  ����       ����    ˵��
*		V1.0    2015-08-30 armfly  �׷�
*
*	Copyright (C), 2015-2016, ���������� www.armfly.com
*
*********************************************************************************************************
*/

#include "bsp.h" /* �ײ�Ӳ������ */
#include "includes.h"
/*
**********************************************************************************************************
��������
**********************************************************************************************************
*/
static void vTaskLED(void *pvParameters);
static void vTaskStart(void *pvParameters);
static void vTaskPrintSysInfo(void *pvParameters);
static void vTaskUartSend(void *pvParameters);
static void AppTaskCreate(void);
/*
**********************************************************************************************************
��������
**********************************************************************************************************
*/
static TaskHandle_t xHandleTaskLED = NULL;
static TaskHandle_t xHandleTaskStart = NULL;
static TaskHandle_t xHandleTaskPrintSysInfo = NULL;
static TaskHandle_t xHandleTaskUartSend = NULL;
/*
*********************************************************************************************************
*	�� �� ��: main
*	����˵��: c�������
*	��    �Σ���
*	�� �� ֵ: �������(���账��)
*********************************************************************************************************
*/
int main(void)
{

	bsp_Init(); /* Ӳ����ʼ�� */
	//myprintf("Welcome to Solar's experiment\r\n");
	AppTaskCreate();
	/* �������ȣ���ʼִ������ */
	vTaskStartScheduler();
	/*
	���ϵͳ���������ǲ������е�����ģ����е����Ｋ�п��������ڶ�ʱ��������߿��������
	heap �ռ䲻����ɴ���ʧ�ܣ���Ҫ�Ӵ� FreeRTOSConfig.h �ļ��ж���� heap ��С��
	#define configTOTAL_HEAP_SIZE ( ( size_t ) ( 17 * 1024 ) )
   */
	while (1)
		;
}

/*
*********************************************************************************************************
* �� �� ��: AppTaskCreate
* ����˵��: ����Ӧ������
* �� ��: �� * �� �� ֵ: ��
*********************************************************************************************************
*/
static void AppTaskCreate(void)
{

	xTaskCreate(vTaskLED,		  /* ������ */
				"vTaskLED",		  /* ������ */
				500,			  /* ����ջ��С����λ word��Ҳ���� 4 �ֽ� */
				NULL,			  /* ������� */
				2,				  /* �������ȼ�*/
				&xHandleTaskLED); /* ������ */
								  // xTaskCreate(vTaskStart,			 /* ������ */
								  // 			"vTaskStart",		 /* ������ */
								  // 			512,				 /* ����ջ��С����λ word��Ҳ���� 4 �ֽ� */
								  // 			NULL,				 /* ������� */
								  // 			4,					 /* �������ȼ�*/
								  // 			&xHandleTaskStart);	 /* ������ */
	
	// xTaskCreate(vTaskPrintSysInfo,		  /* ������ */
	// 			"vTaskPrintSysInfo",		  /* ������ */
	// 			512,			  /* ����ջ��С����λ word��Ҳ���� 4 �ֽ� */
	// 			NULL,			  /* ������� */
	// 			2,				  /* �������ȼ�*/
	// 			&xHandleTaskPrintSysInfo); /* ������ */


	xTaskCreate(vTaskUartSend,		  /* ������ */
				"vTaskUartSend",		  /* ������ */
				512,			  /* ����ջ��С����λ word��Ҳ���� 4 �ֽ� */
				NULL,			  /* ������� */
				2,				  /* �������ȼ�*/
				&xHandleTaskUartSend); /* ������ */
}

static void vTaskStart(void *pvParameters)
{
	/* ״̬����ѭ�� */
	while (1)
	{
		bsp_LedToggle(1);
		vTaskDelay(500);
	}
}

static void vTaskPrintSysInfo(void *pvParameters)
{
	uint8_t pcWriteBuffer[500];
	/* ״̬����ѭ�� */
	while (1)
	{
		myprintf("=================================================\r\n");
		myprintf("������ ����״̬ ���ȼ� ʣ��ջ �������\r\n");
		vTaskList((char *)&pcWriteBuffer);
		myprintf("%s\r\n", pcWriteBuffer);
		myprintf("\r\n ������ ���м��� ʹ����\r\n");
		vTaskGetRunTimeStats((char *)&pcWriteBuffer);
		myprintf("%s\r\n", pcWriteBuffer);
		vTaskDelay(1000);
	}
} 

static void vTaskLED(void *pvParameters)
{
	/* ״̬����ѭ�� */
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