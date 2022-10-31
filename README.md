# MCU-ry_tick_task
# 概述  
* **三类任务状态**  
  * 就绪态  
  * 挂起态  
  * 待机态  
* **两种任务模式**  
  * 普通模式（定时就绪）  
  * 特权模式（持续性就绪）  
* **三个任务操作**  
  * 恢复任务  
  * 挂起任务  
  * 待机任务  
---
# 如何使用(以STM32F103举例)  
## MCU建立系统时基  
可使用任意一个定时器外设，配置成系统时基，供`ry_tick_task`使用  
**举例：**  
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
## 运行ry_tick_task  
```
int main(void)
{
	ry_task_init();
	while(1)
	{
		ry_task_run();
	}
}
```

## 注册任务  
**注册三个任务：**  
* **任务1**  
  * 特权模式、常待机，**唤醒后就绪2000ms自动待机**。  
* **任务2**  
  * 普通模式、**100ms运行一次**；  
  * 运行100次后，使Task1以“RY_TASK_TIMER_READY”就绪。  
* **任务3**  
  * 特权模式、**持续就绪**。
```
static ry_task_t Task1;
static ry_task_t Task2;
static ry_task_t Task3;
/* 任务1处理函数 */
static void Task1_func(void)
{
}
/* 任务2处理函数 */
static void Task2_func(void)
{
    /* 运行100次后，使Task1以“RY_TASK_TIMER_READY”就绪 */
}
/* 任务3处理函数 */
static void Task2_func(void)
{
}

void ry_board_init(void)
{
    /* 系统时基初始化，如配置成1ms的定时中断，提供给系统使用 */
    tp_systick_cfg();
	
    /* 任务1，特权、常待机、唤醒后就绪2000ms自动待机 */
    ry_task_reg(&Task1, Task1_func, RY_TASK_MODE_SUPER, RY_TASK_STBY, "Task1", 2000);
    
    /* 任务2，普通、100ms运行一次 */
    ry_task_reg(&Task2, Task2_func, RY_TASK_MODE_NORMAL, RY_TASK_READY, "Task2", 100);
    
    /* 任务3，特权、持续就绪（时间参数无效） */
    ry_task_reg(&Task3, Task3_func, RY_TASK_MODE_SUPER, RY_TASK_READY, "Task3", 0);
}
```

