#include "bsp.h"
/*
*********************************************************************************************************
* 函 数 名: bsp_SetTIMforInt
* 功能说明: 配置 TIM 和 NVIC，用于简单的定时中断. 开启定时中断。 中断服务程序由应用程序实现。
* 形 参: TIMx : 定时器
* _ulFreq : 定时频率 （Hz）。 0 表示关闭。
* _PreemptionPriority : 中断优先级分组
* _SubPriority : 子优先级
* 返 回 值: 无
*********************************************************************************************************
*/
void bsp_SetTIMforInt(TIM_TypeDef *TIMx, uint32_t _ulFreq, uint8_t _PreemptionPriority, uint8_t _SubPriority)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    uint16_t usPeriod;
    uint16_t usPrescaler;
    uint32_t uiTIMxCLK;
    // /* 使能 TIM 时钟 */
    // if ((TIMx == TIM1) || (TIMx == TIM8))
    // {
    //     RCC_APB2PeriphClockCmd(bsp_GetRCCofTIM(TIMx), ENABLE);
    // }
    // else
    // {
    //     RCC_APB1PeriphClockCmd(bsp_GetRCCofTIM(TIMx), ENABLE);
    // }
    if (_ulFreq == 0)
    {
        TIM_Cmd(TIMx, DISABLE); /* 关闭定时输出 */
        /* 关闭 TIM 定时更新中断 (Update) */
        {
            NVIC_InitTypeDef NVIC_InitStructure; /* 中断结构体在 misc.h 中定义 */
            uint8_t irq = 0;                     /* 中断号, 定义在 stm32f4xx.h */
            if (TIMx == TIM1)
                irq = TIM1_UP_IRQn;
            else if (TIMx == TIM2)
                irq = TIM2_IRQn;
            else if (TIMx == TIM3)
                irq = TIM3_IRQn;
            else if (TIMx == TIM4)
                irq = TIM4_IRQn;
            else if (TIMx == TIM5)
                irq = TIM5_IRQn;
            // else if (TIMx == TIM6)
            //     irq = TIM6_IRQn;
            // else if (TIMx == TIM7)
            //     irq = TIM7_IRQn;
            else if (TIMx == TIM8)
                irq = TIM8_UP_IRQn;
            NVIC_InitStructure.NVIC_IRQChannel = irq;
            NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = _PreemptionPriority;
            NVIC_InitStructure.NVIC_IRQChannelSubPriority = _SubPriority;
            NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
            NVIC_Init(&NVIC_InitStructure);
        }
        return;
    }
    /*-----------------------------------------------------------------------
   system_stm32f4xx.c 文件中 static void SetSysClockToHSE(void) 函数对时钟的配置如下：
   //HCLK = SYSCLK
   RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;

   //PCLK2 = HCLK
   RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE2_DIV1;
   //PCLK1 = HCLK
   RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE1_DIV1;
   APB1 定时器有 TIM2, TIM3 ,TIM4, TIM5, TIM6, TIM7, TIM12, TIM13,TIM14
   APB2 定时器有 TIM1, TIM8 ,TIM9, TIM10, TIM11
   ----------------------------------------------------------------------- */
    if ((TIMx == TIM1) || (TIMx == TIM8) || (TIMx == TIM9) || (TIMx == TIM10) || (TIMx == TIM11))
    {
        /* APB2 定时器 */
        uiTIMxCLK = SystemCoreClock;
    }
    else /* APB1 定时器 . */
    {
        uiTIMxCLK = SystemCoreClock; // SystemCoreClock / 2;
    }
    if (_ulFreq < 100)
    {
        usPrescaler = 10000 - 1;                      /* 分频比 = 1000 */
        usPeriod = (uiTIMxCLK / 10000) / _ulFreq - 1; /* 自动重装的值 */
    }
    else if (_ulFreq < 3000)
    {
        usPrescaler = 100 - 1;                      /* 分频比 = 100 */
        usPeriod = (uiTIMxCLK / 100) / _ulFreq - 1; /* 自动重装的值 */
    }
    else /* 大于 4K 的频率，无需分频 */
    {
        usPrescaler = 0;                    /* 分频比 = 1 */
        usPeriod = uiTIMxCLK / _ulFreq - 1; /* 自动重装的值 */
    }
    /* Time base configuration */
    TIM_TimeBaseStructure.TIM_Period = usPeriod;
    TIM_TimeBaseStructure.TIM_Prescaler = usPrescaler;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIMx, &TIM_TimeBaseStructure);
    TIM_ARRPreloadConfig(TIMx, ENABLE);
    /* TIM Interrupts enable */
    TIM_ITConfig(TIMx, TIM_IT_Update, ENABLE);
    /* TIMx enable counter */
    TIM_Cmd(TIMx, ENABLE);
    /* 配置 TIM 定时更新中断 (Update) */
    {
        NVIC_InitTypeDef NVIC_InitStructure; /* 中断结构体在 misc.h 中定义 */
        uint8_t irq = 0;                     /* 中断号, 定义在 stm32f4xx.h */
        if (TIMx == TIM1)
            irq = TIM1_UP_IRQn;
        else if (TIMx == TIM2)
            irq = TIM2_IRQn;
        else if (TIMx == TIM3)
            irq = TIM3_IRQn;
        else if (TIMx == TIM4)
            irq = TIM4_IRQn;
        else if (TIMx == TIM5)
            irq = TIM5_IRQn;
        // else if (TIMx == TIM6)
        //     irq = TIM6_IRQn;
        // else if (TIMx == TIM7)
        //     irq = TIM7_IRQn;
        else if (TIMx == TIM8)
            irq = TIM8_UP_IRQn;
        NVIC_InitStructure.NVIC_IRQChannel = irq;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = _PreemptionPriority;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = _SubPriority;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);
    }
}