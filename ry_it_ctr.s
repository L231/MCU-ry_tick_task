
;/**
 ;* 描述：开启/关闭所有中断
 ;*
 ;**/



	
	;汇编一个代码段，只读，2^2字节对齐，THUMB指令代码，栈8字节对齐
	AREA |.1225|, CODE, READONLY, ALIGN = 2
	THUMB
	REQUIRE8
	PRESERVE8
	



;/**
; * void ry_interrupt_on(ry_u32_t cmd);
; *
; * 开启中断
; *
; **/
ry_interrupt_on    PROC
	EXPORT ry_interrupt_on
		MSR      PRIMASK, r0
		BX       lr
		ENDP


;/**
; * ry_u32_t ry_interrupt_off(void);
; *
; * 关闭所有中断
; *
; **/
ry_interrupt_off   PROC
	EXPORT ry_interrupt_off
		MRS      r0, PRIMASK
		CPSID    I
		BX       lr
		ENDP



  ALIGN   4    ;当前文件的代码要求4字节对齐
	END          ;结束

