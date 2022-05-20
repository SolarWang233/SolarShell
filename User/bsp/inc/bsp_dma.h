#ifndef BSP_DMA_H
#define BSP_DMA_H

void bsp_dmaConfig(DMA_Channel_TypeDef* DMA_CHx,u32 cpar,u32 cmar,u16 cndtr,u8 mode,u8 dir);
void bsp_dmaEnable(DMA_Channel_TypeDef*DMA_CHx,u32 bufLen);
void bsp_dmaSetAddrLen(DMA_Channel_TypeDef* DMA_CHx,u32 addr,u32 len);
#endif
