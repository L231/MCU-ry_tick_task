#ifndef __RY_DEF_H__
	#define	__RY_DEF_H__
	
	

/* 是否使用临界段代码保护 */
#define  RY_USE_CRITICAL_SECTION  0


#define  RY_NULL                  0

#define  RY_TICK_MAX              0xFFFFFFFF
#define  RY_TICK_MAX_DIV_2        (RY_TICK_MAX >> 1)



#define  ry_inline                static __inline
#define  ry_weak                  __weak


/* 32位MCU */
typedef unsigned int              ry_core_t;


#define  TASK_CONTAINER_OF(ptr, type, member) \
            ((type *)((char *)(ptr) - (ry_core_t)(&((type *)0)->member)))



#endif

