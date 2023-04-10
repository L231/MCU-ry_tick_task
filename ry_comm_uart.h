#ifndef __RY_COMM_UART_H__
	#define	__RY_COMM_UART_H__
	

#include <stdint.h>

#include "ry_def.h"



/* 使用 ry_tick_task 处理通信 */
#define  RY_COMM_USE_RY_TICK_TASK                  1
#if RY_COMM_USE_RY_TICK_TASK == 1
#include "ry_task.h"
#endif

/* 使用DMA接收，需要用户自行搭配串口空闲中断 */
#define  RY_COMM_USE_DMA_RECEIVE_MODE              0

/* 报头异常时不应答 */
#define  RY_COMM_HEAD_FAIL_NACK                    0

/* 只开辟接收邮箱，适合资源少的MCU（该机制：当前报文未处理完时，无法缓存新的报文） */
#define  RY_COMM_ONLY_RXMSG                        1



/* 邮箱的大小，字节 */
#define  RY_COMM_BUF_SIZE                          128

/* 报头最大长度 */
#define  COMM_MSG_MAX_LEN                          8






typedef enum
{
	COMM_UART_IDLE,               /* 空闲，接收数据中 */
	COMM_UART_CHECK,              /* 校验，初步判断数据的合法性 */
	COMM_UART_HANDLE,             /* 处理报文 */
	COMM_UART_SEND,               /* 应答通信发送方 */
	COMM_UART_ERR,                /* 报文异常 */
}E_CommUartStatus_t;




/* 数据通信的邮箱 */
typedef struct
{
	uint16_t                   len;
	uint8_t                    buf[RY_COMM_BUF_SIZE];
}Msg_t;


/* 报文信息结构体 */
typedef struct
{
	uint8_t                    mcu_no;                            /* MCU标号 */
	uint8_t                    head[COMM_MSG_MAX_LEN];            /* 报头，不使用报头指令码偏移为0 */
	uint8_t                    cmd_offset;                        /* 指令码的偏移位置 */
}MsgInfo_t;


/* 串口通信的控制块 */
typedef struct
{
	uint8_t                    flag_ack       : 1;                /* 是否应答的标志 */
	uint8_t                    flag_cs        : 1;                /* CS校验标识 */
	uint8_t                    reserve        : 2;                /* 保留 */
	uint8_t                    comm_status    : 4;                /* 通信的状态 */
	uint8_t                    err;                               /* 报文处理的错误码 */
	uint16_t                   rx_cnt;                            /* 接收邮箱的计数器 */
	uint16_t                   last_rx_cnt;                       /* 上一次接收邮箱的计数器 */
	MsgInfo_t                  msg_info;
	void                     (*send)(uint8_t *, uint16_t);        /* 通信的发送函数 */
	uint8_t                  (*msg_handle)(Msg_t *);              /* 报文处理函数，用户可自定义 */
	uint8_t                  (*handle_hook)(Msg_t *);             /* 报文处理的钩子函数，用户可自定义 */
	Msg_t                     *msg;                               /* 待处理的报文 */
#if RY_COMM_ONLY_RXMSG == 0
	Msg_t                      tx;                                /* 发送邮箱 */
#endif
	Msg_t                      rx;                                /* 接收邮箱 */
#if RY_COMM_USE_RY_TICK_TASK == 1
	ry_task_t                  task;
#endif
#if RY_COMM_USE_DMA_RECEIVE_MODE == 1
	void                     (*dma_receive_init)(void);
#endif
}CommUart_t;



/* 指令注册表结构体 */
typedef struct
{
	uint8_t                    cmd;
	uint8_t                  (*handle)(Msg_t *);
}CmdList_t;






/* 注册串口通信控制块，内部会同步注册串口接收处理任务 */
extern void ry_comm_uart_reg(CommUart_t  *uart,
                            uint8_t     flag_cs,                     /* CS校验标识，1表示开启CS校验 */
                            uint8_t     mcu_no,                      /* 当前MCU的标号 */
                            uint8_t    *msg_head,                    /* 报头，不使用报头指令码偏移为0 */
                            uint8_t     cmd_offset,                  /* 指令码的偏移位置 */
                            void      (*send)(uint8_t *, uint16_t),  /* 通信的发送函数 */
#if RY_COMM_USE_RY_TICK_TASK == 1
                            uint8_t   (*msg_handle)(Msg_t *),        /* 报文处理函数，用户可自定义 */
                            uint8_t     task_tick,                   /* 任务的周期，周期性处理通信 */
                            void      (*task_func)(void));           /* 任务主体 */
#else
                            uint8_t   (*msg_handle)(Msg_t *));       /* 报文处理函数，用户可自定义 */
#endif

extern uint8_t ry_cs_check(uint8_t *buf, uint16_t len);
/* 设置CS校验 */
extern void ry_comm_uart_cs_check_cfg(CommUart_t *uart, uint8_t p);


/* 报文接收处理，若收到报文则调用状态机，处理完后退出 */
extern void ry_comm_handle(CommUart_t *uart);
/* 串口接收处理的状态机 */
extern void ry_msg_state_machine(CommUart_t *uart);

/* 指令查表处理，用于注册为状态机的报文处理 */
extern uint8_t ry_msg_cmd_lookup(Msg_t *msg);
/* 处理从机应答的报文 */
extern uint8_t ry_slave_ack_msg_handle(Msg_t *msg);



/* 获取接收缓存区的地址 */
extern uint8_t *ry_comm_uart_get_rx_buf(CommUart_t *uart);
#if RY_COMM_USE_DMA_RECEIVE_MODE == 0
extern uint8_t ry_comm_get_rx_finish_status(CommUart_t *uart);
/* 串口中断接收函数 */
extern void ry_uart_it_rec(CommUart_t *uart, uint8_t data);
#else
/* 报文收完了(串口空闲中断)，启动状态机 */
extern uint8_t ry_dma_mode_start_state_machine(CommUart_t *uart, uint16_t rx_data_len);
/* 注册DMA接收初始化回调函数 */
extern void ry_dma_mode_callback_reg(CommUart_t *uart, void (*dma_rec_init)(void));
#endif




#endif

