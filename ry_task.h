#ifndef __RY_TASK_H__
	#define	__RY_TASK_H__
	
	
#include <stdint.h>
#include "ry_list.h"


#define  RY_TICK_MAX              0xFFFFFFFF
#define  RY_TICK_MAX_DIV_2        (RY_TICK_MAX >> 1)


/* 32λMCU */
typedef uint32_t         ry_core_t;

typedef enum
{
    RY_TASK_MODE_NORMAL,  /* ��ͨ���� */
    RY_TASK_MODE_SUPER,   /* ��Ȩ���� */
    
    RY_TASK_READY,        /* �������� */
    RY_TASK_TIMER_READY,  /* ��ʱ������,ʱ��Ƭ���� */
    RY_TASK_SUSPEND,      /* ���𣬱������������� */
    RY_TASK_STBY,         /* ���ڴ��������ô���̬�����к����������������� */
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


#define  TASK_CONTAINER_OF(ptr, type, member) \
            ((type *)((char *)(ptr) - (ry_core_t)(&((type *)0)->member)))



extern void ry_sys_list_init(void);
extern void ry_task_reg(ry_task_t *task, void (*func)(void),
                uint8_t mode, uint8_t status, char *name, uint32_t tick);

extern void ry_task_status_cfg(uint8_t status);

extern void ry_task_run(void);
extern void ry_task_recover(ry_task_t *task, uint8_t status);
extern void ry_task_suspend(ry_task_t *task, uint32_t tick);
extern void ry_task_standby(ry_task_t *task);



#endif

