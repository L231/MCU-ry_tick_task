/**
 *
**/

#include "ry_task.h"




/* 串口通信处理任务 */
static ry_task_t UartCommandHandle;
/* M2相关指令的处理任务 */
static ry_task_t UartToM2CommandHandle;
/* M2无响应时进行的处理 */
static ry_task_t M2NoResponse;
/* xxx缓启动任务 */
static ry_task_t xxxTask;




/* 接收并处理来自PC的指令*/
static void UartCommandHandle_task(void)
{
	/* 任务逻辑处理 */
	
	
	/* (演示)唤醒“xxxTask” */
    ry_task_recover(&xxxTask, RY_TASK_TIMER_READY);
}

/* 接收M2的响应 */
static void UartToM2CommandHandle_task(void)
{
	/* 任务逻辑处理 */
	
	
	/* (演示)以初始配置的时间挂起任务“M2NoResponse” */
    ry_task_suspend(&M2NoResponse, M2NoResponse.init_tick);
}

/* 等待时间已过，M2无响应，则立马报错 */
static void M2NoResponse_task(void)
{
	/* 任务逻辑处理 */
	
	
	/* 主动进入待机 */
    ry_task_standby(&M2NoResponse);
}

/* xxx缓启动处理，等待别人唤醒 */
static void xxxTask_task(void)
{
	/* 缓启动处理 */
}



void uart_task_reg(void)
{
	/* 接收并处理来自PC的指令，普通、1ms执行一次 */
    ry_task_reg(&UartCommandHandle, UartCommandHandle_task,
                RY_TASK_MODE_NORMAL, RY_TASK_READY, "UCH", 1);
				
	/* 接收M2的响应，特权、永久就绪（时间参数配置无效果） */
    ry_task_reg(&UartToM2CommandHandle, UartToM2CommandHandle_task,
                RY_TASK_MODE_SUPER, RY_TASK_READY, "UTM2CH", 0);
				
	/* M2响应超时的处理，普通、常待机、挂起后等待500ms运行 */
    ry_task_reg(&M2NoResponse, M2NoResponse_task,
                RY_TASK_MODE_NORMAL, RY_TASK_STBY, "M2NR", 500);
				
	/* xxx缓启动处理，特权、常待机、唤醒后就绪2000ms自动待机 */
    ry_task_reg(&xxxTask, xxxTask_task,
                RY_TASK_MODE_SUPER, RY_TASK_STBY, "xxxT", 2000);
}



int main(void)
{
	/* 系统时基初始化，如配置成1ms的定时中断，提供给系统使用 */
	
	ry_sys_list_init();
	uart_task_reg();
	while(1)
	{
		ry_task_run();
	}
}






