#ifndef __RY_COMM_UART_H__
	#define	__RY_COMM_UART_H__
	

#include <stdint.h>

#include "ry_def.h"



/* ʹ�� ry_tick_task ����ͨ�� */
#define  RY_COMM_USE_RY_TICK_TASK                  1
#if RY_COMM_USE_RY_TICK_TASK == 1
#include "ry_task.h"
#endif

/* ʹ��DMA���գ���Ҫ�û����д��䴮�ڿ����ж� */
#define  RY_COMM_USE_DMA_RECEIVE_MODE              0

/* ��ͷ�쳣ʱ��Ӧ�� */
#define  RY_COMM_HEAD_FAIL_NACK                    0


/* ֻ���ٽ������䣬�ʺ���Դ�ٵ�MCU���û��ƣ���ǰ����δ������ʱ���޷������µı��ģ� */
#define  RY_COMM_ONLY_RXMSG                        1

/* ����Ĵ�С���ֽ� */
#define  RY_COMM_BUF_SIZE                          128



/* ��ͷ��󳤶� */
#define  COMM_MSG_MAX_LEN                          8

/* CSУ�� */
#define  COMM_CS_CHECK_OFF                         0
#define  COMM_CS_CHECK_ON                          1




/* ָ����Ľ�� */
typedef enum
{
    COMMAND_RUN_OK               = 0x00, /* ָ���������� */
    COMMAND_RUN_ERR              = 0xF1, /* ָ�������쳣 */
    COMMAND_PARAM_ERR            = 0xF2, /* ָ��Ĳ����Ƿ� */
    COMMAND_M1_BUSY              = 0xFB, /* M1ͨ��æ����������ӻ�ͨ���� */
    COMMAND_M2_NO_ACK            = 0xFC, /* �ӻ�����Ӧ */
    COMMAND_NULL                 = 0xFE, /* ָ��Ϊ�� */
    COMMAND_TRANSFER_FAIL        = 0xFF, /* ָ���ʧ�� */
    COMMAND_FAIL_MASK            = 0xFF, /* ָ��ʧ�ܵ����� */
}E_MsgErr_t;


typedef enum
{
	COMM_UART_IDLE,               /* ���У����������� */
	COMM_UART_CHECK,              /* У�飬�����ж����ݵĺϷ��� */
	COMM_UART_HANDLE,             /* ������ */
	COMM_UART_SEND,               /* Ӧ��ͨ�ŷ��ͷ� */
	COMM_UART_ERR,                /* �����쳣 */
}E_CommUartStatus_t;




/* ����ͨ�ŵ����� */
typedef struct
{
	uint16_t                   len;
	uint8_t                    buf[RY_COMM_BUF_SIZE];
}Msg_t;


/* ������Ϣ�ṹ�� */
typedef struct
{
	uint8_t                    mcu_no;                            /* MCU��� */
	uint8_t                    head[COMM_MSG_MAX_LEN];            /* ��ͷ����ʹ�ñ�ͷָ����ƫ��Ϊ0 */
	uint8_t                    cmd_offset;                        /* ָ�����ƫ��λ�� */
}MsgInfo_t;


/* ����ͨ�ŵĿ��ƿ� */
typedef struct
{
	uint8_t                    flag_ack       : 1;                /* �Ƿ�Ӧ��ı�־ */
	uint8_t                    flag_cs        : 1;                /* CSУ���ʶ */
	uint8_t                    reserve        : 2;                /* ���� */
	uint8_t                    comm_status    : 4;                /* ͨ�ŵ�״̬ */
	uint8_t                    err;                               /* ���Ĵ���Ĵ����� */
	uint16_t                   rx_cnt;                            /* ��������ļ����� */
	uint16_t                   last_rx_cnt;                       /* ��һ�ν�������ļ����� */
	MsgInfo_t                  msg_info;
	void                     (*send)(uint8_t *, uint16_t);        /* ͨ�ŵķ��ͺ��� */
	uint8_t                  (*msg_handle)(Msg_t *);              /* ���Ĵ��������û����Զ��� */
	Msg_t                     *msg;                               /* ������ı��� */
#if RY_COMM_ONLY_RXMSG == 0
	Msg_t                      tx;                                /* �������� */
#endif
	Msg_t                      rx;                                /* �������� */
#if RY_COMM_USE_RY_TICK_TASK == 1
	ry_task_t                  task;
#endif
#if RY_COMM_USE_DMA_RECEIVE_MODE == 1
	void                     (*dma_receive_init)(void);
#endif
}CommUart_t;



/* ָ��ע���ṹ�� */
typedef struct
{
	uint8_t                    cmd;
	uint8_t                  (*handle)(Msg_t *);
}CmdList_t;






/* ע�ᴮ��ͨ�ſ��ƿ飬�ڲ���ͬ��ע�ᴮ�ڽ��մ������� */
extern void ry_comm_uart_reg(CommUart_t  *uart,
                            uint8_t     flag_cs,                     /* CSУ���ʶ��1��ʾ����CSУ�� */
                            uint8_t     mcu_no,                      /* ��ǰMCU�ı�� */
                            uint8_t    *msg_head,                    /* ��ͷ����ʹ�ñ�ͷָ����ƫ��Ϊ0 */
                            uint8_t     cmd_offset,                  /* ָ�����ƫ��λ�� */
                            void      (*send)(uint8_t *, uint16_t),  /* ͨ�ŵķ��ͺ��� */
#if RY_COMM_USE_RY_TICK_TASK == 1
                            uint8_t   (*msg_handle)(Msg_t *),        /* ���Ĵ��������û����Զ��� */
                            uint8_t     task_tick,                   /* ��������ڣ������Դ���ͨ�� */
                            void      (*task_func)(void));           /* �������� */
#else
                            uint8_t   (*msg_handle)(Msg_t *));       /* ���Ĵ��������û����Զ��� */
#endif

extern uint8_t ry_cs_check(uint8_t *buf, uint16_t len);
/* ����CSУ�� */
extern void ry_comm_uart_cs_check_cfg(CommUart_t *uart, uint8_t p);


/* ���Ľ��մ������յ����������״̬������������˳� */
extern void ry_comm_handle(CommUart_t *uart);
/* ���ڽ��մ����״̬�� */
extern void ry_msg_state_machine(CommUart_t *uart);

/* ָ����������ע��Ϊ״̬���ı��Ĵ��� */
extern uint8_t ry_msg_cmd_lookup(Msg_t *msg);
/* ����ӻ�Ӧ��ı��� */
extern uint8_t ry_slave_ack_msg_handle(Msg_t *msg);



/* ��ȡ���ջ������ĵ�ַ */
extern uint8_t *ry_comm_uart_get_rx_buf(CommUart_t *uart);
#if RY_COMM_USE_DMA_RECEIVE_MODE == 0
extern uint8_t ry_comm_get_rx_finish_status(CommUart_t *uart);
/* �����жϽ��պ��� */
extern void ry_uart_it_rec(CommUart_t *uart, uint8_t data);
#else
/* ����������(���ڿ����ж�)������״̬�� */
extern uint8_t ry_dma_mode_start_state_machine(CommUart_t *uart, uint16_t rx_data_len);
/* ע��DMA���ճ�ʼ���ص����� */
extern void ry_dma_mode_callback_reg(CommUart_t *uart, void (*dma_rec_init)(void));
#endif
															
															


#endif

