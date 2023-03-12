
#include "ry_comm_uart.h"




extern const CmdList_t gUserCmdList[];







uint8_t ry_cs_check(uint8_t *buf, uint16_t len)
{
    uint8_t cs = 0;
    uint16_t pos;
    for(pos = 0; pos < len; pos++)
    {
        cs += *buf++;
    }
    return (~cs);
}

/* 设置CS校验 */
void ry_comm_uart_cs_check_cfg(CommUart_t *uart, uint8_t p)
{
    uart->flag_cs = p;
}


/* 拷贝通信数据，rx至tx */
static uint16_t _get_rx_msg(CommUart_t *uart)
{
    uint16_t len        = uart->rx_cnt;
#if RY_COMM_ONLY_RXMSG == 0
    uint16_t pos;
    
    for(pos = 0; pos < len; pos++)
    {
        uart->tx.buf[pos] = uart->rx.buf[pos];
    }
    uart->msg           = &uart->tx;
    uart->rx_cnt        = 0;
#else
    uart->msg           = &uart->rx;
#endif
    uart->msg->len      = len;
    return len;
}



/* 注册串口通信控制块，内部会同步注册串口接收处理任务 */
void ry_comm_uart_reg(CommUart_t *uart,
                    uint8_t     flag_cs,                     /* CS校验标识，1表示开启CS校验 */
                    uint8_t     mcu_no,                      /* 当前MCU的标号 */
                    uint8_t    *msg_head,                    /* 报头，不使用报头指令码偏移为0 */
                    uint8_t     cmd_offset,                  /* 指令码的偏移位置 */
                    void      (*send)(uint8_t *, uint16_t),  /* 通信的发送函数 */
#if RY_COMM_USE_RY_TICK_TASK == 1
                    uint8_t   (*msg_handle)(Msg_t *),        /* 报文处理函数，用户可自定义 */
                    uint8_t     task_tick,                   /* 任务的周期，周期性处理通信 */
                    void      (*task_func)(void))            /* 任务主体 */
#else
                    uint8_t   (*msg_handle)(Msg_t *))        /* 报文处理函数，用户可自定义 */
#endif
{
    uint8_t len = 0;
    uart->comm_status         = COMM_UART_IDLE;
    uart->last_rx_cnt         = 0;
    uart->rx_cnt              = 0;
    uart->err                 = COMMAND_RUN_OK;
    uart->msg                 = RY_NULL;
    uart->flag_ack            = 1;
    uart->flag_cs             = flag_cs;
    uart->send                = send;
    uart->msg_handle          = msg_handle;
    /* 指令码的偏移，也就是报头的长度，限定其大小 */
    if(cmd_offset > COMM_MSG_MAX_LEN)
        cmd_offset = COMM_MSG_MAX_LEN;
    for(; len < cmd_offset; len++)
    {
        /* 缓存报头 */
        uart->msg_info.head[len] = msg_head[len];
    }
    uart->msg_info.mcu_no     = mcu_no;
    uart->msg_info.cmd_offset = cmd_offset;
#if RY_COMM_USE_RY_TICK_TASK == 1
#if RY_COMM_USE_DMA_RECEIVE_MODE == 0
    ry_task_reg(&uart->task, task_func, RY_TASK_MODE_NORMAL, RY_TASK_READY, "CUM", task_tick);
#else
    ry_task_reg(&uart->task, task_func, RY_TASK_MODE_NORMAL, RY_TASK_STBY, "CUM", task_tick);
#endif
#endif
}


/* 串口接收处理的状态机，DMA模式、RTOS模式下，都会挂起状态机 */
void ry_msg_state_machine(CommUart_t *uart)
{
    switch(uart->comm_status)
    {
    /* 缓存报文 */
    case COMM_UART_IDLE :
#if RY_COMM_USE_DMA_RECEIVE_MODE == 0
        ry_comm_get_rx_finish_status(uart);
#endif
        break;
    case COMM_UART_CHECK :
        if(_get_rx_msg(uart))
        {
            uart->err = COMMAND_RUN_OK;
            /* 报文的指令有偏移时，检查报头 */
            if(uart->msg_info.cmd_offset)
            {
                uint8_t pos = 0;
                /* 判断报头是否相等 */
                for(; pos < uart->msg_info.cmd_offset; pos++)
                {
                    /* 报头不相等，当前报文舍弃，同时不回复主机 */
                    if(uart->msg_info.head[pos] != uart->msg->buf[pos])
                    {
                        uart->err         = COMMAND_TRANSFER_FAIL;
#if RY_COMM_HEAD_FAIL_NACK == 1
                        uart->msg->len    = 0;
                        uart->comm_status = COMM_UART_SEND;
#else
                        uart->comm_status = COMM_UART_ERR;
#endif
                        return;
                    }
                }
            }
            /* 不使用CS校验，或者校验通过 */
            if(0 == uart->flag_cs || ry_cs_check(uart->msg->buf, uart->msg->len - 1) == uart->msg->buf[uart->msg->len - 1])
            {
                uart->err         = COMMAND_RUN_OK;
                uart->comm_status = COMM_UART_HANDLE;
                break;
            }
            uart->err             = COMMAND_TRANSFER_FAIL;
            uart->comm_status     = COMM_UART_ERR;
            break;
        }
        uart->comm_status         = COMM_UART_SEND;
        break;
        /* 处理收到的报文 */
    case COMM_UART_HANDLE :
        uart->err                 = uart->msg_handle(uart->msg);
        if(COMMAND_RUN_OK == uart->err)
            uart->comm_status     = COMM_UART_SEND;
        else
            uart->comm_status     = COMM_UART_ERR;
        break;
        /* 异常情况 */
    case COMM_UART_ERR :
        uart->msg->buf[0]         = COMMAND_FAIL_MASK;
        uart->msg->buf[1]         = uart->msg_info.mcu_no;
        uart->msg->buf[2]         = uart->err;
        uart->msg->len            = 4;
        uart->comm_status         = COMM_UART_SEND;
        break;
        /* 响应报文的发送端 */
    case COMM_UART_SEND :
        /* 长度不为0，同时未转发报文给从机 */
        if(uart->msg->len && uart->flag_ack == 1)
        {
            uart->msg->buf[uart->msg->len - 1] = ry_cs_check(uart->msg->buf, uart->msg->len - 1);
            uart->send(uart->msg->buf, uart->msg->len);
        }
        /* 报文处理完，相关参数赋初始值 */
#if RY_COMM_ONLY_RXMSG == 1
        uart->rx_cnt              = 0;
#endif
        uart->last_rx_cnt         = 0;
        uart->msg                 = RY_NULL;
        uart->flag_ack            = 1;
        uart->comm_status         = COMM_UART_IDLE;
#if RY_COMM_USE_DMA_RECEIVE_MODE == 1
        /* 初始化DMA的接收 */
        if(uart->dma_receive_init)
            uart->dma_receive_init();
#if RY_COMM_USE_RY_TICK_TASK == 1
        /* 使任务进入待机，下一个报文收完时，会再次唤醒 */
        ry_task_standby(&uart->task, RY_BACK);
#endif
#endif
        break;
    default :
        uart->comm_status         = COMM_UART_IDLE;
        break;
    }
}


/* 报文接收处理，若收到报文则调用状态机，处理完后退出 */
void ry_comm_handle(CommUart_t *uart)
{
    do
    {
        ry_msg_state_machine(uart);
    }while(uart->comm_status != COMM_UART_IDLE);
}


/* 指令查表处理，用于注册为状态机的报文处理 */
uint8_t ry_msg_cmd_lookup(Msg_t *msg)
{
    uint8_t pos;
#if RY_COMM_ONLY_RXMSG == 0
    MsgInfo_t *pMsgInfo = &TASK_CONTAINER_OF(msg, CommUart_t, tx)->msg_info;
#else
    MsgInfo_t *pMsgInfo = &TASK_CONTAINER_OF(msg, CommUart_t, rx)->msg_info;
#endif
    
    for(pos = 0; gUserCmdList[pos].cmd != RY_NULL; pos++)
    {
        if(gUserCmdList[pos].cmd == msg->buf[pMsgInfo->cmd_offset])
        {
            if(gUserCmdList[pos].handle != RY_NULL)
                return gUserCmdList[pos].handle(msg);
        }
    }
    return COMMAND_NULL;
}


/* 处理从MCU应答的报文 */
uint8_t ry_slave_ack_msg_handle(Msg_t *msg)
{
    return COMMAND_RUN_OK;
}




/* 获取接收缓存区的地址 */
uint8_t *ry_comm_uart_get_rx_buf(CommUart_t *uart)
{
    return uart->rx.buf;
}

#if RY_COMM_USE_DMA_RECEIVE_MODE == 0
/* 串口中断接收函数 */
void ry_uart_it_rec(CommUart_t *uart, uint8_t data)
{
    if(RY_COMM_BUF_SIZE > uart->rx_cnt)
        uart->rx.buf[uart->rx_cnt++] = data;
}

/* 获取接收完成的标志 */
uint8_t ry_comm_get_rx_finish_status(CommUart_t *uart)
{
    if(uart->rx_cnt > 0 && uart->last_rx_cnt == uart->rx_cnt)
    {
        uart->comm_status = COMM_UART_CHECK;
        return 1;
    }
    uart->last_rx_cnt = uart->rx_cnt;
    return 0;
}
#else
/* 报文收完了(串口空闲中断)，启动状态机 */
uint8_t ry_dma_mode_start_state_machine(CommUart_t *uart, uint16_t rx_data_len)
{
    if(rx_data_len > 0)
    {
#if RY_COMM_USE_RY_TICK_TASK == 1
        ry_task_recover(&uart->task, RY_TASK_READY, RY_IT);
#endif
        uart->rx_cnt      = rx_data_len;
        uart->last_rx_cnt = rx_data_len;
        uart->comm_status = COMM_UART_CHECK;
        return 1;
    }
    return 0;
}

/* 注册DMA接收初始化回调函数 */
void ry_dma_mode_callback_reg(CommUart_t *uart, void (*dma_rec_init)(void))
{
    uart->dma_receive_init = dma_rec_init;
}

#endif
