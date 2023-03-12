
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

/* ����CSУ�� */
void ry_comm_uart_cs_check_cfg(CommUart_t *uart, uint8_t p)
{
    uart->flag_cs = p;
}


/* ����ͨ�����ݣ�rx��tx */
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



/* ע�ᴮ��ͨ�ſ��ƿ飬�ڲ���ͬ��ע�ᴮ�ڽ��մ������� */
void ry_comm_uart_reg(CommUart_t *uart,
                    uint8_t     flag_cs,                     /* CSУ���ʶ��1��ʾ����CSУ�� */
                    uint8_t     mcu_no,                      /* ��ǰMCU�ı�� */
                    uint8_t    *msg_head,                    /* ��ͷ����ʹ�ñ�ͷָ����ƫ��Ϊ0 */
                    uint8_t     cmd_offset,                  /* ָ�����ƫ��λ�� */
                    void      (*send)(uint8_t *, uint16_t),  /* ͨ�ŵķ��ͺ��� */
#if RY_COMM_USE_RY_TICK_TASK == 1
                    uint8_t   (*msg_handle)(Msg_t *),        /* ���Ĵ��������û����Զ��� */
                    uint8_t     task_tick,                   /* ��������ڣ������Դ���ͨ�� */
                    void      (*task_func)(void))            /* �������� */
#else
                    uint8_t   (*msg_handle)(Msg_t *))        /* ���Ĵ��������û����Զ��� */
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
    /* ָ�����ƫ�ƣ�Ҳ���Ǳ�ͷ�ĳ��ȣ��޶����С */
    if(cmd_offset > COMM_MSG_MAX_LEN)
        cmd_offset = COMM_MSG_MAX_LEN;
    for(; len < cmd_offset; len++)
    {
        /* ���汨ͷ */
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


/* ���ڽ��մ����״̬����DMAģʽ��RTOSģʽ�£��������״̬�� */
void ry_msg_state_machine(CommUart_t *uart)
{
    switch(uart->comm_status)
    {
    /* ���汨�� */
    case COMM_UART_IDLE :
#if RY_COMM_USE_DMA_RECEIVE_MODE == 0
        ry_comm_get_rx_finish_status(uart);
#endif
        break;
    case COMM_UART_CHECK :
        if(_get_rx_msg(uart))
        {
            uart->err = COMMAND_RUN_OK;
            /* ���ĵ�ָ����ƫ��ʱ����鱨ͷ */
            if(uart->msg_info.cmd_offset)
            {
                uint8_t pos = 0;
                /* �жϱ�ͷ�Ƿ���� */
                for(; pos < uart->msg_info.cmd_offset; pos++)
                {
                    /* ��ͷ����ȣ���ǰ����������ͬʱ���ظ����� */
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
            /* ��ʹ��CSУ�飬����У��ͨ�� */
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
        /* �����յ��ı��� */
    case COMM_UART_HANDLE :
        uart->err                 = uart->msg_handle(uart->msg);
        if(COMMAND_RUN_OK == uart->err)
            uart->comm_status     = COMM_UART_SEND;
        else
            uart->comm_status     = COMM_UART_ERR;
        break;
        /* �쳣��� */
    case COMM_UART_ERR :
        uart->msg->buf[0]         = COMMAND_FAIL_MASK;
        uart->msg->buf[1]         = uart->msg_info.mcu_no;
        uart->msg->buf[2]         = uart->err;
        uart->msg->len            = 4;
        uart->comm_status         = COMM_UART_SEND;
        break;
        /* ��Ӧ���ĵķ��Ͷ� */
    case COMM_UART_SEND :
        /* ���Ȳ�Ϊ0��ͬʱδת�����ĸ��ӻ� */
        if(uart->msg->len && uart->flag_ack == 1)
        {
            uart->msg->buf[uart->msg->len - 1] = ry_cs_check(uart->msg->buf, uart->msg->len - 1);
            uart->send(uart->msg->buf, uart->msg->len);
        }
        /* ���Ĵ����꣬��ز�������ʼֵ */
#if RY_COMM_ONLY_RXMSG == 1
        uart->rx_cnt              = 0;
#endif
        uart->last_rx_cnt         = 0;
        uart->msg                 = RY_NULL;
        uart->flag_ack            = 1;
        uart->comm_status         = COMM_UART_IDLE;
#if RY_COMM_USE_DMA_RECEIVE_MODE == 1
        /* ��ʼ��DMA�Ľ��� */
        if(uart->dma_receive_init)
            uart->dma_receive_init();
#if RY_COMM_USE_RY_TICK_TASK == 1
        /* ʹ��������������һ����������ʱ�����ٴλ��� */
        ry_task_standby(&uart->task, RY_BACK);
#endif
#endif
        break;
    default :
        uart->comm_status         = COMM_UART_IDLE;
        break;
    }
}


/* ���Ľ��մ������յ����������״̬������������˳� */
void ry_comm_handle(CommUart_t *uart)
{
    do
    {
        ry_msg_state_machine(uart);
    }while(uart->comm_status != COMM_UART_IDLE);
}


/* ָ����������ע��Ϊ״̬���ı��Ĵ��� */
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


/* �����MCUӦ��ı��� */
uint8_t ry_slave_ack_msg_handle(Msg_t *msg)
{
    return COMMAND_RUN_OK;
}




/* ��ȡ���ջ������ĵ�ַ */
uint8_t *ry_comm_uart_get_rx_buf(CommUart_t *uart)
{
    return uart->rx.buf;
}

#if RY_COMM_USE_DMA_RECEIVE_MODE == 0
/* �����жϽ��պ��� */
void ry_uart_it_rec(CommUart_t *uart, uint8_t data)
{
    if(RY_COMM_BUF_SIZE > uart->rx_cnt)
        uart->rx.buf[uart->rx_cnt++] = data;
}

/* ��ȡ������ɵı�־ */
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
/* ����������(���ڿ����ж�)������״̬�� */
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

/* ע��DMA���ճ�ʼ���ص����� */
void ry_dma_mode_callback_reg(CommUart_t *uart, void (*dma_rec_init)(void))
{
    uart->dma_receive_init = dma_rec_init;
}

#endif
