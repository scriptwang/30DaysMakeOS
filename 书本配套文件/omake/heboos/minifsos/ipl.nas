; NASK�ō�����ւ�OS��pIPL(AT�݊��@�p)
; TAB = 4
; copyright(C) 2003 �썇�G��, KL-01
; OS��512�o�C�g�����ł���Ɖ���
; OS�̓V�����_0�A�w�b�h0�A�Z�N�^2��������Ă���Ƒz��

;	prompt>nask ipl.nas ipl.bin ipl.lst
; �ŃA�Z���u���ł��܂��Bnask��tolset05�ȍ~�Ɋ܂܂�Ă��܂��B
; tolset05�� http://www.imasy.orr/~kawai/osask/developers.html �ɂ���܂��B

; �f�B�X�N�C���[�W�����ɂ͂������܂��B
;	prompt>copy /B ipl.bin+os.com fdimage.bin

; ������f�B�X�N�ɏ������ނɂ͂������܂��B
; �܂��t�H�[�}�b�g�ς݂̃f�B�X�N��A:�ɓ����B
;	prompt>imgtol w a: fdimage.bin
; imgtol��tolset05�ȍ~�Ɋ܂܂�Ă��܂��B


[FORMAT "BIN"]
[INSTRSET "i386"]
[OPTIMIZE 1]
[OPTION 1]
[BITS 16]
			ORG		0x7c00

			JMP		START
			NOP
			DB		'hogehoge'

; FAT�Ȃ񂩗p�ӂ��Ă����Ȃ������ɁABPB�������FAT12���Ƃ������Ƃɂ��Ă���
; ������Ȃ����ƁA�f�B�X�N���t�H�[�}�b�g���Ȃ���imgtol�œǂݏ����ł��Ȃ��Ȃ�̂�
; ���傤���Ȃ��̂œ���Ă���

			DW		512 ; �Z�N�^��(�o�C�g�P��)
			DB		1	; �N���X�^��(�Z�N�^�P��)
			DW		1	; �u�[�g�Z�N�^��(�Z�N�^�P��)
			DB		2	; FAT�̐�
			DW		0x00e0	; root directory entries.
			DW		2880	; ���Z�N�^��
			DB		0xf0	; media descriptor byte.
			DW		9		; 1��FAT�̒���(�Z�N�^�P��)
			DW		18		; 1�g���b�N�Ɏ��e����Ă���Z�N�^��
			DW		2		; �w�b�h��
			DD		0		; �s���Z�N�^��
			DD		2880	; ���Z�N�^��
			DB		0, 0	; /* unknown */
			DB		0x29
			DD		0xffffffff
			DB		"hogehoge   "
			DB		"FAT12   "

; ���̃f�B�X�N��DOS��Win�ł͓ǂݏ����ł��܂���̂ŁA��낵��

START:
			MOV		AX,0x0201
			MOV		CX,0x0002
			XOR		DX,DX
			MOV		BX,0x0800
			MOV		ES,BX
			MOV		BX,0x0100
			INT		0x13
			JC		ERR

; .COM�t�@�C���̂Ȃ̂ŁADOS�݊��̃��W�X�^��Ԃɂ��Ă��

			MOV		AX,0x0800
			MOV		SS,AX
			MOV		SP,0xfffe
			MOV		DS,AX
			MOV		ES,AX
			JMP		0x0800:0x0100
ERR:
			INT		0x18	; ROM-BASIC�ցi�΁j

			RESB	0x7dfe-$	; 0x7dfe�܂�0x00�Ŗ��߂�

			DB		0x55,0xaa
			