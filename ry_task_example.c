/**
 *
**/

#include "ry_task.h"




/* ����ͨ�Ŵ������� */
static ry_task_t UartCommandHandle;
/* M2���ָ��Ĵ������� */
static ry_task_t UartToM2CommandHandle;
/* M2����Ӧʱ���еĴ��� */
static ry_task_t M2NoResponse;
/* xxx���������� */
static ry_task_t xxxTask;




/* ���ղ���������PC��ָ��*/
static void UartCommandHandle_task(void)
{
	/* �����߼����� */
	
	
	/* (��ʾ)���ѡ�xxxTask�� */
    ry_task_recover(&xxxTask, RY_TASK_TIMER_READY);
}

/* ����M2����Ӧ */
static void UartToM2CommandHandle_task(void)
{
	/* �����߼����� */
	
	
	/* (��ʾ)�Գ�ʼ���õ�ʱ���������M2NoResponse�� */
    ry_task_suspend(&M2NoResponse, M2NoResponse.init_tick);
}

/* �ȴ�ʱ���ѹ���M2����Ӧ���������� */
static void M2NoResponse_task(void)
{
	/* �����߼����� */
	
	
	/* ����������� */
    ry_task_standby(&M2NoResponse);
}

/* xxx�����������ȴ����˻��� */
static void xxxTask_task(void)
{
	/* ���������� */
}



void uart_task_reg(void)
{
	/* ���ղ���������PC��ָ���ͨ��1msִ��һ�� */
    ry_task_reg(&UartCommandHandle, UartCommandHandle_task,
                RY_TASK_MODE_NORMAL, RY_TASK_READY, "UCH", 1);
				
	/* ����M2����Ӧ����Ȩ�����þ�����ʱ�����������Ч���� */
    ry_task_reg(&UartToM2CommandHandle, UartToM2CommandHandle_task,
                RY_TASK_MODE_SUPER, RY_TASK_READY, "UTM2CH", 0);
				
	/* M2��Ӧ��ʱ�Ĵ�����ͨ���������������ȴ�500ms���� */
    ry_task_reg(&M2NoResponse, M2NoResponse_task,
                RY_TASK_MODE_NORMAL, RY_TASK_STBY, "M2NR", 500);
				
	/* xxx������������Ȩ�������������Ѻ����2000ms�Զ����� */
    ry_task_reg(&xxxTask, xxxTask_task,
                RY_TASK_MODE_SUPER, RY_TASK_STBY, "xxxT", 2000);
}



int main(void)
{
	/* ϵͳʱ����ʼ���������ó�1ms�Ķ�ʱ�жϣ��ṩ��ϵͳʹ�� */
	
	ry_sys_list_init();
	uart_task_reg();
	while(1)
	{
		ry_task_run();
	}
}






