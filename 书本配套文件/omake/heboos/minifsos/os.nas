; NASK�ō����.COM�t�@�C���^�ւ�OS (AT�݊��@��p)
;		mini-FirstStepOS
;
; TAB = 4
; copyright(C) 2003 �썇�G��, KL-01
;
; .COM�t�@�C���̓v���O������64KB���z���Ȃ�����ɂ����Ă͂����Ƃ��ȒP
; �����Ă��̐l�͍ŏI�I��OS��64KB���z���邩��Ƃ������R��.COM��
; �I���������Ȃ��Ǝv�����낤�B�������ŏ�����.EXE�̋�J�𖡂키�K�v�͂Ȃ��B
; 64KB���z�����炻�̎���.EXE������΂����ł͂Ȃ����B
; .COM�Ȃ�Z�O�����g���C�ɂ��Ȃ��Ă����̂ŋC�y�Ȃ��̂��B

;	prompt>nask os.nas os.com os.lst
; �ŃA�Z���u���ł��܂��Bnask��tolset05�ȍ~�Ɋ܂܂�Ă��܂��B
; tolset05�� http://www.imasy.orr/~kawai/osask/developers.html �ɂ���܂��B

[FORMAT "BIN"]
[INSTRSET "i386"]
[OPTIMIZE 1]
[OPTION 1]
[BITS 16]
			ORG		0x0100 ; .COM�͕K��ORG 0x0100�Ŏn�܂�

; ���ĉ������悤���B�Ƃ肠����320x200x256�F�ł�����ăO���f�[�V������
; �o���ėV�Ԃ��ȁH�i"hello"�Ƃ��͕��}�����邾�낤����j

;	��ʃ��[�h�ƃp���b�g�ݒ�

			MOV 	AX,0x0013
			INT		0x10
			XOR		BX,BX
PALLOP:
			MOV		AX,0x1010
			MOV		DH,BL
			MOV		CH,BL
			MOV		CL,0
			PUSH	BX
			INT		0x10
			POP		BX
			INC		BX
			CMP		BX,0x3f
			JBE		PALLOP

			MOV		AX,0x1010
			MOV		BL,255
			MOV		DH,0x3f
			MOV		CX,0x3f3f
			INT		0x10

;	VRAM�ւ̏�������

			PUSH	DS
			MOV		AX,0xa000
			MOV		DS,AX
			XOR		BX,BX

;	�܂�8���C�����N���A

			XOR		AX,AX
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
			CMP		AL,0x3f
			JBE		GRALOP0

			POP		DS

			JMP		$	; �����܂��i���Z�b�g�{�^���������Ăˁj
