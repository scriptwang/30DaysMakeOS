/*
	�}�N���W�@�`macro.h�`
*/
#ifndef	__MACRO_H
#define	__MACRO_H

#pragma warning(disable:4786)

#define HRESULT	long
#define LPVOID	void*
#define LPSTR	char*
#define LONG	long
#define DWORD	unsigned long
#define WORD	unsigned short
#define BYTE	unsigned char
//#define BOOL int
//#define BOOLEAN char

// �[���Ŗ��߂�}�N��
#define ZERO(var) memset(&var, 0, sizeof(var));

// �|�C���^����NULL�Ȃ��delete����}�N��
#define DELETE_SAFE(var) if (var != NULL) { delete var; var = NULL; }

// �N���X�̃|�C���^����delete����}�N��
#define DELETEPTR_SAFE(var) if (var != NULL) { delete [] var; var = NULL; }

#define IS_ALPHA(c)		(((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z') || ((c) == '_'))
#define IS_DIGIT(c)		((c) >= '0' && (c) <= '9')
#define IS_ALNUM(c)		(IS_ALPHA(c) || IS_DIGIT(c))
#define IS_KANJI1(c)	(((c) >= 0x81 && (c) <= 0x9f) || ((c) >= 0xe0 && (c) <= 0xfc))
#define IS_CHARACTOR(c)	(IS_ALNUM(c) || IS_KANJI1(c))

#endif
