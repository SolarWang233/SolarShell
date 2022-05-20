#include "bsp.h"
/* 定时器频率，50us 一次中断 */
#define timerINTERRUPT_FREQUENCY 20000
/* 中断优先级 */
#define timerHIGHEST_PRIORITY 1
/* 被系统调用 */
volatile uint32_t ulHighFrequencyTimerTicks = 0UL;

/*
*********************************************************************************************************
* 函 数 名: TIM6_IRQHandler
* 功能说明: TIM6 中断服务程序。
* 形 参: 无 * 返 回 值: 无
*********************************************************************************************************
*/
void TIM4_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)
    {
        ulHighFrequencyTimerTicks++;
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
    }
}

void bsp_InitSysInfoTimer(void)
{
    bsp_SetTIMforInt(TIM4, timerINTERRUPT_FREQUENCY, timerHIGHEST_PRIORITY, 0);
}