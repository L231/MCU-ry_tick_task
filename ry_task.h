#ifndef __RY_TASK_H__
	#define	__RY_TASK_H__



#include <stdint.h>
#include "ry_list.h"
#include "ry_def.h"







typedef enum
{
    RY_TASK_MODE_NORMAL,  /* ��ͨ����,���к��Զ����� */
    RY_TASK_MODE_SUPER,   /* ��Ȩ���� */
    
    RY_TASK_READY,        /* ����̬ */
    RY_TASK_TIMER_READY,  /* ��ʱ������,ʱ��Ƭ���ƣ���Ȩ����ר�� */
    RY_TASK_SUSPEND,      /* ����̬���������������� */
    RY_TASK_STBY,         /* ����̬����������������ֻ�ܵȴ����˻��� */
}ry_task_type_t;



typedef struct
{
    ry_list_t            ready;       /* �������� */
    ry_list_t            suspend;     /* �������� */
    ry_list_t            standby;     /* �������� */
}ry_sys_list_t;


/* ����ṹ�� */
typedef struct
{
    uint8_t              mode;                /* ģʽ */
    uint8_t              status;              /* ���������𡢴���״̬ */
    char                *name;                /* �������� */
    void               (*func)(void);         /* �������� */
    uint32_t             remaining_tick;      /* ����ʱ�� */
    uint32_t             init_tick;           /* ʱ�������ϵͳʱ����������ʱ�� */
    ry_list_t            list;                /* ����ڵ� */
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

