/*
	�t�@�C�����W���[�����N���X�@�`module.h + module.cpp�`
*/
#ifndef	__MODULE_H
#define	__MODULE_H

#pragma warning(disable:4786)

#include <stdio.h>
#include <stdlib.h>
#ifndef LINUX
#include <io.h>
#endif
#include <iostream>

#include <string>
#include <list>
#include <map>
#include <stack>

using namespace std;

#include "macro.h"

class Module{
	LPVOID	lpMdlAdr;		// ���W���[���A�h���X
	DWORD	dwMdlSize;		// ���W���[���T�C�Y
	LPSTR	lpMdlPos;		// ReadLine�̏����ʒu
	string	FileName;		// ���W���[�������Ă���t�@�C����

	string	MakeFullPath(string& filename);		// �f�B���N�g����₤

  public:
	Module(void){ lpMdlAdr=NULL; dwMdlSize=0; lpMdlPos=NULL; }
	~Module(){ Release(); }
#ifdef WINVC
	void	Release(void){ DELETEPTR_SAFE(lpMdlAdr); dwMdlSize=0; lpMdlPos=NULL; }
#else
	void	Release(void){ DELETEPTR_SAFE((unsigned char*)lpMdlAdr); dwMdlSize=0; lpMdlPos=NULL; }
#endif

	string	GetFileName(void){ return FileName; }
	LPVOID	GetModuleAddress(void){ return lpMdlAdr; }
	DWORD	GetModuleSize(void){ return dwMdlSize; }
	HRESULT ReadFile(string& filename);
		// �߂�l 0:����I��  1:fileopen���s  3:�������m�ێ��s  4:�ǂݍ��ݎ��s  5:fileclose���s
	HRESULT ReadLine(LPSTR buf);
		// �߂�l 0:����I��  1:EOF  2:�o�b�t�@���ӂ�  3:�t�@�C���ǂݍ���łȂ�
};

//���i�K�ł̓f�B���N�g���̒ǉ��Ȃǂ͂��Ă��Ȃ��B���̂���
//include�f�B���N�g����ǉ����āA���Ɍ������Ă��������ɂ������B

#endif
