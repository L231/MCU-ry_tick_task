
;/**
 ;* ����������/�ر������ж�
 ;*
 ;**/



	
	;���һ������Σ�ֻ����2^2�ֽڶ��룬THUMBָ����룬ջ8�ֽڶ���
	AREA |.1225|, CODE, READONLY, ALIGN = 2
	THUMB
	REQUIRE8
	PRESERVE8
	



;/**
; * void ry_interrupt_on(ry_u32_t cmd);
; *
; * �����ж�
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
; * �ر������ж�
; *
; **/
ry_interrupt_off   PROC
	EXPORT ry_interrupt_off
		MRS      r0, PRIMASK
		CPSID    I
		BX       lr
		ENDP



  ALIGN   4    ;��ǰ�ļ��Ĵ���Ҫ��4�ֽڶ���
	END          ;����

