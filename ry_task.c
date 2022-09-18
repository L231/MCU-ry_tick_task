
#include "ry_task.h"



/* ϵͳ���� */
static ry_sys_list_t SysList;
static uint32_t OldTick = 0;


/* ��ȡ��ǰϵͳʱ�� */
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


/* ����ע�� */
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

/* ���¾�������ִ������ת��״̬ */
void ry_task_run(void)
{
    ry_list_t *List = 0;
    
    /* ˢ�¹��������о������������θ������������� */
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
        
        /* ��������������������̬ */
        if(task->status == RY_TASK_STBY)
            return;
        switch(task->mode)
        {
            case RY_TASK_MODE_SUPER :
                /* �ӵ�ǰ�ڵ����� */
                ry_list_remove(&task->list);
                if(task->status == RY_TASK_TIMER_READY)
                {
                    /* ����ʱ�����꣬ת����� */
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
                /* ��ͨ�������������� */
                ry_task_suspend(task, task->init_tick);
                break;
        }
    }
}

/* ������������������ʱ��ʱ������ת�Ƶ��������� */
static void ry_task_update(void)
{
    ry_task_t *task;
    ry_list_t *List = SysList.suspend.next;
    while(List != &SysList.suspend) 
    {
        ry_list_t *l = List;
        List = List->next;
        task = TASK_CONTAINER_OF(l, ry_task_t, list);
        /* ��ʱʱ�䵽�ˣ���ӵ��������� */
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

/* ʹ�������������� */
void ry_task_recover(ry_task_t *task, uint8_t status)
{
    ry_list_remove(&task->list);
    ry_list_insert_after(SysList.ready.prev, &task->list);
    task->remaining_tick = task->init_tick + GET_TICK();
    task->status = status;
}

/* ����ʱʱ���Ŷӣ��������� */
void ry_task_suspend(ry_task_t *task, uint32_t tick)
{
    ry_task_t *t;
    ry_list_t *List;
    
    if(task->mode == RY_TASK_MODE_SUPER || task->status == RY_TASK_SUSPEND)
        return;
    
    ry_list_remove(&task->list);
    task->remaining_tick = tick + GET_TICK();
    /* ��ʱʱ���С�������� */
    for(List = SysList.suspend.next; List != &SysList.suspend; List = List->next)
    {
        t = TASK_CONTAINER_OF(List, ry_task_t, list);
        /* ʱ��ȵ�ǰ�ڵ��С�����ڵ�ǰ�ڵ�ǰ�� */
        if(task->remaining_tick < t->remaining_tick)
            break;
    }
    ry_list_insert_after(List->prev, &task->list);
    task->status = RY_TASK_SUSPEND;
}

/* ʹ����������̬�������ȴ����˻��� */
void ry_task_standby(ry_task_t *task)
{
    if(task->status == RY_TASK_STBY)
        return;
    ry_list_remove(&task->list);
    ry_list_insert_after(SysList.standby.prev, &task->list);
    task->status = RY_TASK_STBY;
}
