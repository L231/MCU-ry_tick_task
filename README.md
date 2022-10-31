# MCU-ry_tick_task
# 概述  
* 三种任务状态  
  * 就绪态  
  * 挂起态  
  * 待机态  
* 两种任务模式  
  * 普通模式（定时就绪）  
  *特权模式（持续性就绪）  
* 三个任务操作  
  * 恢复任务  
  * 挂起任务  
  * 待机任务  
---
# 如何使用(以STM32F103举例)  
## MCU建立系统时基  
可使用任意一个定时器外设，配置成系统时基，供`ry_tick_task`使用  
举例：  
```
static uint32_t gTick = 0;
void SysTick_Handler(void)
{
	gTick++;
}

void tp_systick_cfg(void)
{
	RCC_ClocksTypeDef  mcu_clk;
	RCC_GetClocksFreq(&mcu_clk);
	SysTick_Config(mcu_clk.SYSCLK_Frequency / 1000000 * 1000);
}

/* 获取当前系统时基 */
uint32_t ry_get_systick(void)
{
	return gTick;
}
```


