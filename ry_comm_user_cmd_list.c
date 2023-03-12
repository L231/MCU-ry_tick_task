
#include "ry_comm_uart.h"

#include "t_usb.h"







#define  CMD_MSG_TO_SLAVE               0x12
#define  CMD_T_USB                      0x25



extern uint8_t ry_cmd_pcmsg_to_slave(Msg_t *msg);


/* 用户指令注册表 */
const CmdList_t gUserCmdList[] =
{
    
    {CMD_MSG_TO_SLAVE,           ry_cmd_pcmsg_to_slave},
    {CMD_T_USB,                  t_usb_task},
    
    {RY_NULL,           RY_NULL},
};



