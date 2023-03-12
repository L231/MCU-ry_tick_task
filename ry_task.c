
#include "ry_task.h"



/* ϵͳ���� */
static ry_sys_list_t gSysList;
static uint32_t gLastTick = 0;




static void _task_update(void);


/* �鶨�庯������ȡ��ǰϵͳʱ�� */
ry_weak uint32_t ry_get_systick(void)
{
    return 0;
}


/* �鶨�庯�����弶��ʼ�� */
ry_weak void ry_board_init(void)
{
    
}

void ry_task_init(void)
{
    ry_list_init(&gSysList.ready);
    ry_list_init(&gSysList.suspend);
    ry_list_init(&gSysList.standby);
    ry_list_init(&gSysList.it);
    ry_board_init();
}


/* ����ע�� */
void ry_task_reg(ry_task_t *task, void (*func)(void),
                uint8_t mode, uint8_t status, char *name, uint32_t tick)
{
    RY_NEW_IT_VARI;
    task->name           = name;
    task->mode           = mode;
    task->status         = status;
    task->func           = func;
    task->init_tick      = tick;
    task->remaining_tick = tick + ry_get_systick();
    ry_list_init(&task->list);
    ry_list_init(&task->it_list);
    RY_INTERRUPT_OFF;
    if(status == RY_TASK_STBY)
        ry_list_insert_after(gSysList.standby.prev, &task->list);
    else
        ry_list_insert_after(gSysList.ready.prev, &task->list);
    RY_INTERRUPT_ON;
}


/* ���¾�������ִ������ת��״̬ */
void ry_task_run(void)
{
    ry_list_t *List;
    RY_NEW_IT_VARI;

    /* ˢ�¹��������о������������θ������������� */
    if(gLastTick != ry_get_systick())
    {
        _task_update();
        gLastTick = ry_get_systick();
    }

    List = gSysList.ready.next;
    /* ����������� */
    if(List != &gSysList.ready) 
    {
        ry_task_t *task = TASK_CONTAINER_OF(List, ry_task_t, list);
        task->func();
        
        /* ��������������������̬ */
        if(task->status == RY_TASK_STBY)
            return;
        RY_INTERRUPT_OFF;
        switch(task->mode)
        {
        case RY_TASK_MODE_SUPER :
            /* �ӵ�ǰ�ڵ����� */
            ry_list_remove(&task->list);
            if(task->status == RY_TASK_TIMER_READY)
            {
                /* ����ʱ�����꣬ת����� */
                if(ry_get_systick() - task->remaining_tick < RY_TICK_MAX_DIV_2)
                {
                    ry_list_insert_after(gSysList.standby.prev, &task->list);
                    task->status = RY_TASK_STBY;
                    break;
                }
            }
            ry_list_insert_after(gSysList.ready.prev, &task->list);
            break;
        case RY_TASK_MODE_NORMAL :
            /* ��ͨ�������������� */
            ry_task_suspend(task, task->init_tick, RY_BACK);
            break;
        }
        RY_INTERRUPT_ON;
    }
}


/* ����״̬ת�ƣ�ת�Ƶ��µ����� */
static void _task_ctr(ry_task_t *task, uint8_t run_mode)
{
    ry_list_t *List;
    RY_NEW_IT_VARI;

    RY_INTERRUPT_OFF;
    /* ��ǰ�ں�̨���� */
    if(run_mode == RY_BACK)
    {
        ry_list_remove(&task->list);
        switch(task->status)
        {
            case RY_TASK_READY :
            case RY_TASK_TIMER_READY :
                ry_list_insert_after(gSysList.ready.prev, &task->list);
                break;
            case RY_TASK_SUSPEND :
                /* ��ʱʱ���С�������� */
                for(List = gSysList.suspend.next; List != &gSysList.suspend; List = List->next)
                {
                    /* ʱ��ȵ�ǰ�ڵ��С�����ڵ�ǰ�ڵ�ǰ�� */
                    if(task->remaining_tick < TASK_CONTAINER_OF(List, ry_task_t, list)->remaining_tick)
                        break;
                }
                ry_list_insert_after(List->prev, &task->list);
                break;
            case RY_TASK_STBY :
                ry_list_insert_after(gSysList.standby.prev, &task->list);
                break;
        }
    }
    /* ��ǰ���ж������У���δ���� */
    else if(&task->it_list == task->it_list.next && &task->it_list == task->it_list.prev)
        ry_list_insert_after(gSysList.it.prev, &task->it_list);
    RY_INTERRUPT_ON;
}

/* ������������������ʱ��ʱ������ת�Ƶ��������� */
static void _task_update(void)
{
    uint32_t tick;
    ry_task_t *task;
    ry_list_t *List;
    RY_NEW_IT_VARI;

    RY_INTERRUPT_OFF;
    /* �����ж����� */
    for(List = gSysList.it.next; List != &gSysList.it;)
    {
        task = TASK_CONTAINER_OF(List, ry_task_t, it_list);
        List = List->next;
        tick = ry_get_systick();
        if(tick - task->remaining_tick < RY_TICK_MAX_DIV_2)
        {
            /* �ж��й���������Ѿ���ʱ�ˣ�ֱ��ת��Ϊ����̬ */
            if(task->status == RY_TASK_SUSPEND)
                task->status = RY_TASK_READY;
            /* ��ʱ�������������ѳ�ʱ���������̬��*/
            else if(task->status == RY_TASK_TIMER_READY)
                task->status = RY_TASK_STBY;
        }
        /* ���ں�̨�������ж�����Ľڵ� */
        ry_list_remove(&task->it_list);
        _task_ctr(task, RY_BACK);
    }
    /* �ָ������������ѳ�ʱ������ */
    for(List = gSysList.suspend.next; List != &gSysList.suspend;)
    {
        task = TASK_CONTAINER_OF(List, ry_task_t, list);
        List = List->next;
        /* ��ʱʱ�䵽�ˣ���ӵ��������� */
        if(ry_get_systick() - task->remaining_tick < RY_TICK_MAX_DIV_2)
        {
            ry_list_remove(&task->list);
            ry_list_insert_after(gSysList.ready.prev, &task->list);
            task->status = RY_TASK_READY;
        }
        else
            break;
    }
    RY_INTERRUPT_ON;
}


/* ʹ�������������� */
void ry_task_recover(ry_task_t *task, uint8_t status, uint8_t run_mode)
{
    if(task->status == RY_TASK_READY || task->status == RY_TASK_TIMER_READY)
        return;
    task->status         = status;
    task->remaining_tick = task->init_tick + ry_get_systick();
    _task_ctr(task, run_mode);
}


/* ����ʱʱ���Ŷӣ��������� */
void ry_task_suspend(ry_task_t *task, uint32_t tick, uint8_t run_mode)
{
    if(task->mode == RY_TASK_MODE_SUPER || task->status == RY_TASK_SUSPEND)
        return;
    task->status         = RY_TASK_SUSPEND;
    task->remaining_tick = tick + ry_get_systick();
    _task_ctr(task, run_mode);
}


/* ʹ����������̬�������ȴ����˻��� */
void ry_task_standby(ry_task_t *task, uint8_t run_mode)
{
    if(task->status == RY_TASK_STBY)
        return;
    task->status = RY_TASK_STBY;
    _task_ctr(task, run_mode);
}
