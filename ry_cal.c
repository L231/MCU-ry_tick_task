
#include "ry_cal.h"
#include "ry_def.h"



typedef struct
{
    int32_t      actual;      /* 实际的目标值 */
    uint32_t     original;    /* 原始值 */
}CalPoint_t;
typedef struct
{
    uint16_t     start_addr;  /* EEPROM上的存储地址 */
    uint8_t      point_size;  /* 校准点的大小 */
    uint8_t      number;      /* 校准点数 */
    uint8_t      ch;          /* 通道 */
    uint8_t      linkage_ch;  /* 联动通道 */
    CalPoint_t  *point;       /* 校准点 */
}CalTable_t;
typedef struct
{
    uint8_t      ch_number;   /* 通道数 */
    CalTable_t  *table;       /* 校准表格 */
}CalObj_t;


/* 设备校准的标志位 */
static uint8_t __DeviceCalFlag = 0;

/* 定义各个通道的校准点 */
CAL_TABLE(__NULL, CHANNEL_POINT_REG, POINT_CFG, POINT_END, __NULL);
/* 定义校准表格 */
CAL_TABLE(TABLE_REG, CHANNEL_REG, __NULL, __NULL, END_CFG);
/* EEPROM储存信息控制块。结构已固定 */
static CalObj_t __userCalTable[] = {CAL_TABLE(TABLE_USER_REG, __NULL, __NULL, __NULL, __NULL)};

#define __CAL_TABLE_NUMBER   (sizeof(__userCalTable) / sizeof(CalObj_t))
#define __GET_CAL_OBJ(item)  &__userCalTable[item]
#define __IS_CHANNEL(_table, _ch, _return)  if(_table >= __CAL_TABLE_NUMBER)\
												return _return;\
											obj = __GET_CAL_OBJ(_table);\
											if(_ch >= obj->ch_number)\
												return _return;


ry_weak void ry_eeprom_read(uint16_t data_addr, uint8_t *pdata, uint16_t number)
{
	
}

void ry_cal_flag_cfg(uint8_t status)
{
    __DeviceCalFlag = status;
}
uint8_t ry_get_device_cal_flag(void)
{
    return __DeviceCalFlag;
}

/* 加载校准数据 */
uint8_t ry_cal_table_upload(uint8_t cal_table)
{
    uint8_t status = CAL_POINT_UPLOAD_OK;
    uint8_t ch, cnt, pos, pos1;
    uint8_t data[128];
    uint32_t p1, p2;
	CalObj_t *obj;
	
	if(cal_table >= __CAL_TABLE_NUMBER)
		return CAL_POINT_DATA_ERR;
	obj = __GET_CAL_OBJ(cal_table);
    for(ch = 0; ch < obj->ch_number; ch++)
    {
        ry_eeprom_read(obj->table[ch].start_addr, data, obj->table[ch].number * obj->table[ch].point_size);
        p1 = 0;
        p2 = 0;
        for(cnt = 0, pos = 0; cnt < obj->table[ch].number; cnt++)
        {
            p1 = 0;
            for(pos1 = 0; pos1 < obj->table[ch].point_size; pos1++)
                p1 = (p1 << 8) | data[pos + pos1];
            if(p1 <= p2)
            {
                __DeviceCalFlag = CAL_POINT_DATA_ERR;
                status = CAL_POINT_DATA_ERR;
                break;
            }
            obj->table[ch].point[cnt].original = p1;
            p2   = p1;
            pos += obj->table[ch].point_size;
        }
    }
    return status;
}

/* 获取校准状态，并加载校准数据 */
uint8_t ry_get_device_cal_status(void)
{
    uint8_t pos;
    uint8_t buf[1];
    ry_eeprom_read(DEVICE_CAL_DATA_ADDR, buf, 1);
    if(buf[0] == DEVICE_CAL_FINISH)
    {
        /* 加载校准数据 */
        for(pos = 0; pos < __CAL_TABLE_NUMBER; pos++)
        {
            __DeviceCalFlag = ry_cal_table_upload(pos);
        }
        return __DeviceCalFlag;
    }
    __DeviceCalFlag = DEVICE_CAL_NO;
    return __DeviceCalFlag;
}


/* 获取实际电压值、电流值 */
float ry_get_actual(uint8_t cal_table, uint8_t ch, uint32_t val)
{
    uint8_t pos, number;
    int32_t prev, next;
    CalPoint_t *CalPoint;
    float   k;
	CalObj_t *obj;
	
	__IS_CHANNEL(cal_table, ch, 0);
    CalPoint = obj->table[ch].point;
    number   = obj->table[ch].number;

    /* 找出在哪个区间内 */
    for(pos = 0; pos < number; pos++)
    {
        if(val <= CalPoint[pos].original)
        {
            if(val == CalPoint[pos].original)
                return (float)CalPoint[pos].actual;
            if(pos == 0)
                pos = 1;
//                return ((float)CalPoint[0].actual) / CalPoint[0].original * val;
            break;
        }
    }
    if(pos >= number)
        pos = number - 1;

    /* y = k * x + b */
    /* k = (y2 - y1) / (x2 - x1) */
    /* b = y1 - k * x1 */
    /* f(data) = k(data - x1) + y1 */
    prev = CalPoint[pos-1].actual;
    next = CalPoint[pos].actual;
    k    = (float)(next - prev) / (float)(CalPoint[pos].original - CalPoint[pos-1].original);
    return (float)((float)((int32_t)val - (int32_t)CalPoint[pos-1].original) * k + prev);
}


/* 获取电压、电流的原始值 */
uint32_t ry_get_original(uint8_t cal_table, uint8_t ch, int32_t actual)
{
    uint8_t pos, number;
    uint32_t prev, next;
    CalPoint_t *CalPoint;
    CalTable_t *Table;
    float   k;
	CalObj_t *obj;
	
	__IS_CHANNEL(cal_table, ch, 0);
    Table    = &obj->table[ch];
    CalPoint = Table->point;
    number   = Table->number;

    /* 找出在哪个区间内 */
    for(pos = 0; pos < number; pos++)
    {
        if(actual <= CalPoint[pos].actual)
        {
            if(actual == CalPoint[pos].actual)
                return CalPoint[pos].original;
            if(pos == 0)
                pos = 1;
//                return (uint32_t)(((float)CalPoint[0].original) / CalPoint[0].actual * val);
            break;
        }
    }
    if(pos >= number)
        pos = number - 1;

    /* 判断是否为联动通道 */
    if(Table->ch != Table->linkage_ch)
    {
        /* 找出联动通道 */
        while(obj->table[pos].ch != ch)
        {
            pos++;
            if(pos >= obj->ch_number)
                return 0;
        }
        prev   = CalPoint[pos-1].original;
        next   = obj->table[pos].point[pos-1].original;
    }
    else
    {
        prev   = CalPoint[pos-1].original;
        next   = CalPoint[pos].original;
    }

    /* y = k * x + b */
    /* k = (y2 - y1) / (x2 - x1) */
    /* b = y1 - k * x1 */
    /* f(data) = k(data - x1) + y1 */
    k    = (float)((int32_t)next - (int32_t)prev) / (float)(CalPoint[pos].actual - CalPoint[pos-1].actual);
    return (uint32_t)((float)(actual - CalPoint[pos-1].actual) * k + prev);
}

/* 获取电压、电流的基准原始值 */
uint32_t ry_get_basic_original(uint8_t cal_table, uint8_t ch, int32_t actual)
{
    uint8_t pos, number;
    CalPoint_t *CalPoint;
	CalObj_t   *obj;
	
	__IS_CHANNEL(cal_table, ch, 0);
    CalPoint = obj->table[ch].point;
    number   = obj->table[ch].number;

    /* 找出在哪个区间内 */
    for(pos = 0; pos < number; pos++)
    {
        /* 校准点实际值从大到小。遍历表格，找出它小于哪个点，那么它必定 >= 上一个点 */
        if(actual < CalPoint[pos].actual)
        {
            /* 返回上一个点 */
            if(pos == 0)
                return 0;
            else
                return CalPoint[pos - 1].original;
        }
    }
    /* actual大于所有校准点，则返回最后一个校准点 */
    return CalPoint[number - 1].original;
}

/* 获取校准点处于校准表格中的位置 */
int8_t ry_get_cal_point_pos(uint8_t cal_table, uint8_t ch, int32_t actual)
{
    uint8_t pos;
	CalObj_t *obj;
	
	__IS_CHANNEL(cal_table, ch, -1);
    /* 找点 */
    for(pos = 0; pos < obj->table[ch].number; pos++)
    {
        if(actual == obj->table[ch].point[pos].actual)
			return pos;
    }
	return -1;
}



