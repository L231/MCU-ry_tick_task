
#include "ry_task.h"



/* 系统链表 */
static ry_sys_list_t SysList;
static uint32_t OldTick = 0;


/* 获取当前系统时基 */
extern uint32_t get_sys_tick(void);
#define  GET_TICK             get_sys_tick


static void ry_task_update(void);
void ry_task_suspend(ry_task_t *task, uint32_t tick);





void ry_sys_list_init(void)
{
    ry_list_init(&SysList.ready);
    ry_list_init(&SysList.suspend);
    ry_list_init(&SysList.standby);
}


/* 任务注册 */
void ry_task_reg(ry_task_t *task, void (*func)(void),
                uint8_t mode, uint8_t status, char *name, uint32_t tick)
{
    task->name           = name;
    task->mode           = mode;
    task->status         = status;
    task->func           = func;
    task->init_tick      = tick;
    task->remaining_tick = tick + GET_TICK();
    ry_list_init(&task->list);
    if(status == RY_TASK_STBY)
        ry_list_insert_after(SysList.standby.prev, &task->list);
    else
        ry_list_insert_after(SysList.ready.prev, &task->list);
}

void ry_task_status_cfg(uint8_t status)
{
    TASK_CONTAINER_OF(SysList.ready.next, ry_task_t, list)->status = status;
}

/* 更新就绪任务，执行任务并转移状态 */
void ry_task_run(void)
{
    ry_list_t *List = 0;
    
    /* 刷新挂起链表，有就绪任务则依次更新至就绪链表 */
    if(OldTick != GET_TICK())
    {
        ry_task_update();
		OldTick = GET_TICK();
    }
    
    List = SysList.ready.next;
    if(List != &SysList.ready) 
    {
        ry_task_t *task = TASK_CONTAINER_OF(List, ry_task_t, list);
        task->func();
        
        /* 任务自身已主动进待机态 */
        if(task->status == RY_TASK_STBY)
            return;
        switch(task->mode)
        {
            case RY_TASK_MODE_SUPER :
                /* 从当前节点脱离 */
                ry_list_remove(&task->list);
                if(task->status == RY_TASK_TIMER_READY)
                {
                    /* 就绪时间用完，转入待机 */
                    if(GET_TICK() - task->remaining_tick < RY_TICK_MAX_DIV_2)
                    {
                        ry_list_insert_after(SysList.standby.prev, &task->list);
                        task->status = RY_TASK_STBY;
						break;
                    }
                }
				ry_list_insert_after(SysList.ready.prev, &task->list);
                break;
            case RY_TASK_MODE_NORMAL :
                /* 普通任务，运行完后挂起 */
                ry_task_suspend(task, task->init_tick);
                break;
        }
    }
}

/* 检索挂起链表，所有延时超时的任务，转移到就绪链表 */
static void ry_task_update(void)
{
    ry_task_t *task;
    ry_list_t *List = SysList.suspend.next;
    while(List != &SysList.suspend) 
    {
        ry_list_t *l = List;
        List = List->next;
        task = TASK_CONTAINER_OF(l, ry_task_t, list);
        /* 延时时间到了，添加到就绪链表 */
        if(GET_TICK() - task->remaining_tick < RY_TICK_MAX_DIV_2)
        {
            ry_list_remove(&task->list);
            ry_list_insert_after(SysList.ready.prev, &task->list);
            task->status = RY_TASK_READY;
        }
        else
            break;
    }
}

/* 使任务进入就绪链表 */
void ry_task_recover(ry_task_t *task, uint8_t status)
{
    ry_list_remove(&task->list);
    ry_list_insert_after(SysList.ready.prev, &task->list);
    task->remaining_tick = task->init_tick + GET_TICK();
    task->status = status;
}

/* 以延时时间排队，挂起任务 */
void ry_task_suspend(ry_task_t *task, uint32_t tick)
{
    ry_task_t *t;
    ry_list_t *List;
    
    if(task->mode == RY_TASK_MODE_SUPER || task->status == RY_TASK_SUSPEND)
        return;
    
    ry_list_remove(&task->list);
    task->remaining_tick = tick + GET_TICK();
    /* 延时时间从小到大排序 */
    for(List = SysList.suspend.next; List != &SysList.suspend; List = List->next)
    {
        t = TASK_CONTAINER_OF(List, ry_task_t, list);
        /* 时间比当前节点的小，插在当前节点前面 */
        if(task->remaining_tick < t->remaining_tick)
            break;
    }
    ry_list_insert_after(List->prev, &task->list);
    task->status = RY_TASK_SUSPEND;
}

/* 使任务进入待机态，被动等待别人唤醒 */
void ry_task_standby(ry_task_t *task)
{
    if(task->status == RY_TASK_STBY)
        return;
    ry_list_remove(&task->list);
    ry_list_insert_after(SysList.standby.prev, &task->list);
    task->status = RY_TASK_STBY;
}
