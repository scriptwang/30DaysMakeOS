; �ւ�OS�p�A�v�� "GRAPHIC.NAS"
; TAB = 4
; copyright(C) 2003 �썇�G��, KL-01

;	prompt>nask graphic.nas graphic.hoa graphic.lst
; �ŃA�Z���u���ł��܂��Bnask��tolset05�ȍ~�Ɋ܂܂�Ă��܂��B
; tolset05�� http://www.imasy.orr/~kawai/osask/developers.html �ɂ���܂��B

[FORMAT "BIN"]
[INSTRSET "i386"]
[OPTIMIZE 1]
[OPTION 1]
[BITS 16]
			ORG		0x0100

;	�p���b�g�̐ݒ�
;	0-15�̓V�X�e���p���b�g�ŁA16�ȍ~�͎��R�Ɏg����A���Ă��Ƃɂ���(��)

			MOV		BX,64
PALLOP:
			MOV		AX,0x1010
			MOV		DH,BL
			AND		DH,0x3f
			MOV		CH,DH
			MOV		CL,0
			PUSH	BX
			INT		0x10
			POP		BX
			INC		BX
			CMP		BX,0x7f
			JBE		PALLOP

;	VRAM�ւ̏�������

			PUSH	DS
			MOV		AX,0xa000
			MOV		DS,AX
			XOR		BX,BX

;	�܂�8���C�����N���A

			MOV		AX,0x4040
CLR8LOP:
			MOV		[BX],AX
			ADD		BX,2
			CMP		BX,320*8
			JB		CLR8LOP

;	�O���f�[�V��������

GRALOP0:
			MOV		CX,320*3/2
GRALOP1:
			MOV		[BX],AX
			ADD		BX,2
			DEC		CX
			JNZ		GRALOP1
			ADD		AX,0x0101
			CMP		AL,0x7f
			JBE		GRALOP0

			POP		DS

;	�����L�[�������Ă��炤

			MOV		AH,0x00
			INT		0x16

;	��ʂ��N���A����OS�ɖ߂�

			MOV		AX,2
			INT		0x80
			XOR		AX,AX
			XOR		CX,CX
			INT		0x80
