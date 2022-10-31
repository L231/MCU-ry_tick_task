
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
	ry_list_t *List = 0;
	RY_NEW_IT_VARI;

	/* ˢ�¹��������о������������θ������������� */
	if(gLastTick != ry_get_systick())
	{
		_task_update();
		gLastTick = ry_get_systick();
	}

	List = gSysList.ready.next;
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
				ry_task_suspend(task, task->init_tick);
				break;
		}
		RY_INTERRUPT_ON;
	}
}


/* ������������������ʱ��ʱ������ת�Ƶ��������� */
static void _task_update(void)
{
	ry_task_t *task;
	ry_list_t *List  = gSysList.suspend.next;
	RY_NEW_IT_VARI;
	RY_INTERRUPT_OFF;
	while(List != &gSysList.suspend) 
	{
		ry_list_t *l   = List;
		List           = List->next;
		task           = TASK_CONTAINER_OF(l, ry_task_t, list);
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
void ry_task_recover(ry_task_t *task, uint8_t status)
{
	RY_NEW_IT_VARI;
	RY_INTERRUPT_OFF;
	ry_list_remove(&task->list);
	ry_list_insert_after(gSysList.ready.prev, &task->list);
	RY_INTERRUPT_ON;
	task->remaining_tick = task->init_tick + ry_get_systick();
	task->status         = status;
}


/* ����ʱʱ���Ŷӣ��������� */
void ry_task_suspend(ry_task_t *task, uint32_t tick)
{
	ry_task_t *t;
	ry_list_t *List;
	RY_NEW_IT_VARI;
	
	if(task->mode == RY_TASK_MODE_SUPER || task->status == RY_TASK_SUSPEND)
		return;
	
	RY_INTERRUPT_OFF;
	ry_list_remove(&task->list);
	task->remaining_tick = tick + ry_get_systick();
	/* ��ʱʱ���С�������� */
	for(List = gSysList.suspend.next; List != &gSysList.suspend; List = List->next)
	{
		t = TASK_CONTAINER_OF(List, ry_task_t, list);
		/* ʱ��ȵ�ǰ�ڵ��С�����ڵ�ǰ�ڵ�ǰ�� */
		if(task->remaining_tick < t->remaining_tick)
			break;
	}
	ry_list_insert_after(List->prev, &task->list);
	RY_INTERRUPT_ON;
	task->status = RY_TASK_SUSPEND;
}


/* ʹ����������̬�������ȴ����˻��� */
void ry_task_standby(ry_task_t *task)
{
	RY_NEW_IT_VARI;
	if(task->status == RY_TASK_STBY)
		return;
	RY_INTERRUPT_OFF;
	ry_list_remove(&task->list);
	ry_list_insert_after(gSysList.standby.prev, &task->list);
	RY_INTERRUPT_ON;
	task->status = RY_TASK_STBY;
}
