
#ifndef RY_CAL_H_
#define RY_CAL_H_

#include <stdint.h>



#define  CH_VOLT_ADC_OVP_ADDR         1
#define  CH_VOLT_ADC_PACK_ADDR        10
#define  CH_SET_CC1_10A_ADDR          100
#define  CH_SET_CC2_10A_ADDR          1000

#define  DEVICE_CAL_DATA_ADDR         0x0000
#define  DEVICE_CAL_FINISH            0xA5
#define  DEVICE_CAL_NO                0xFF



/* 注册校准表格。每个通道至少要有一个点，否则编译报错 */
#define  CAL_TABLE(__table_name, __ch, __p, __pend, __end) \
__table_name(ADC_CalTable)\
__ch(CH_VOLT_ADC_OVP, 2, CH_VOLT_ADC_OVP)\
__p(500, 100)\
__p(3500, 800)\
__pend()\
__ch(CH_VOLT_ADC_PACK, 2, CH_VOLT_ADC_PACK)\
__p(-100, 10)\
__p(0,    3276)\
__p(200,  5000)\
__pend()\
__end()\
__table_name(DAC_CalTable)\
__ch(CH_SET_CC1_10A, 2, CH_SET_CC1_10A)\
__p(500, 100)\
__p(4000, 1000)\
__pend()\
__end()






#define  GET_STR_NUM(_S, type)                (sizeof(_S)/sizeof(type))
#define  GET_POINT_NUM(_T)                    GET_STR_NUM(_T, CalPoint_t)
#define  GET_CH_NUM(_T)                       GET_STR_NUM(_T, CalTable_t)


/* 注册参数全为空 */
#define  __NULL(...) 

/* 注册表格的枚举 */
#define  TABLE_ENUM_REG(_table)           _table,

/* 注册通道的枚举 */
#define  TABLE_NAME_REG(_name)            enum E_##_name##_t{
#define  CHANNEL_ENUM_REG(ch, ...)        ch,
#define  END_ENUM_CFG()                   };

/* 注册独立的校准通道 */
#define  CHANNEL_POINT_REG(_ch, ...)      static CalPoint_t _ch##_Table[] = {
#define  POINT_CFG(x, y)                  {x, y},
#define  POINT_END()                      };

/* 注册校准表格 */
#define  TABLE_REG(_t)              static CalTable_t _t##Item[] = {
#define  CHANNEL_REG(_ch, _n, _lch) {_ch##_ADDR, _n, GET_POINT_NUM(_ch##_Table), _ch, _lch, _ch##_Table},
#define  END_CFG()                  };

/* 注册用户校准表格 */
#define  TABLE_USER_REG(_t)         {GET_CH_NUM(_t##Item), _t##Item},



/* 定义各个通道的枚举 */
CAL_TABLE(TABLE_NAME_REG, CHANNEL_ENUM_REG, __NULL, __NULL, END_ENUM_CFG);

/* 定义校准表格的通道 */
typedef enum
{
	CAL_TABLE(TABLE_ENUM_REG, __NULL, __NULL, __NULL, __NULL)
}E_CalTable_t;





extern float    ry_get_actual(uint8_t cal_table, uint8_t ch, uint32_t val);
extern uint32_t ry_get_original(uint8_t cal_table, uint8_t ch, int32_t val);
extern uint32_t ry_get_basic_original(uint8_t cal_table, uint8_t ch, int32_t val);
extern uint8_t  ry_save_cal_point(uint8_t cal_table, uint8_t ch, int32_t actual, uint32_t data);

extern uint8_t  ry_get_cal_flag(void);
extern void     ry_set_cal_flag(uint8_t status);
extern uint8_t  ry_cal_table_upload(void);




#endif /* RY_CAL_H_ */
