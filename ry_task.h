#ifndef __RY_TASK_H__
	#define	__RY_TASK_H__



#include <stdint.h>
#include "ry_list.h"
#include "ry_def.h"







typedef enum
{
    RY_TASK_MODE_NORMAL,  /* 普通任务,运行后自动挂起 */
    RY_TASK_MODE_SUPER,   /* 特权任务 */
    
    RY_TASK_READY,        /* 就绪态 */
    RY_TASK_TIMER_READY,  /* 定时长就绪,时间片机制，特权任务专用 */
    RY_TASK_SUSPEND,      /* 挂起态，被移至挂起链表 */
    RY_TASK_STBY,         /* 待机态，被移至待机链表，只能等待别人唤醒 */
}ry_task_type_t;



typedef struct
{
    ry_list_t            ready;       /* 就绪链表 */
    ry_list_t            suspend;     /* 挂起链表 */
    ry_list_t            standby;     /* 待机链表 */
}ry_sys_list_t;


/* 任务结构体 */
typedef struct
{
    uint8_t              mode;                /* 模式 */
    uint8_t              status;              /* 就绪、挂起、待机状态 */
    char                *name;                /* 任务名称 */
    void               (*func)(void);         /* 任务主体 */
    uint32_t             remaining_tick;      /* 闹钟时间 */
    uint32_t             init_tick;           /* 时长，配合系统时间生成闹钟时间 */
    ry_list_t            list;                /* 链表节点 */
}ry_task_t;

	
						

extern void ry_task_init(void);
extern void ry_task_reg(ry_task_t *task, void (*func)(void),
                uint8_t mode, uint8_t status, char *name, uint32_t tick);
extern void ry_task_run(void);

extern void ry_task_recover(ry_task_t *task, uint8_t status);
extern void ry_task_suspend(ry_task_t *task, uint32_t tick);
extern void ry_task_standby(ry_task_t *task);



#if RY_USE_CRITICAL_SECTION == 1
void ry_interrupt_on(uint32_t cmd);
extern uint32_t ry_interrupt_off(void);

#define  RY_NEW_IT_VARI       uint32_t cmd
#define  RY_INTERRUPT_OFF     cmd = ry_interrupt_off()
#define  RY_INTERRUPT_ON      ry_interrupt_on(cmd)
#else
#define  RY_NEW_IT_VARI
#define  RY_INTERRUPT_OFF
#define  RY_INTERRUPT_ON
#endif






#endif

