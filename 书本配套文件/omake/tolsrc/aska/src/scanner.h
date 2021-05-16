/*
	�����̓N���X�@�`scanner.h + scanner.cpp�`
*/
#ifndef	__SCANNER_H
#define	__SCANNER_H

#pragma warning(disable:4786)

#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include <string>
#include <list>
#include <map>
#include <stack>

using namespace std;

#include "macro.h"
#include "module.h"
#include "tokendef.h"

class ScannerSub{
	int		nLine;					// ���ݏ������Ă���s�ԍ�
	Module	Mdl;					// ���ݏ������Ă���t�@�C��
	FILE*	lpLogFP;				// �G���[���b�Z�[�W�o�͗p�t�@�C���|�C���^
	char	linebuf[1024];			// ���ݏ������Ă���s�̃o�b�t�@
	LPSTR	lpPos;					// ���ݏ������Ă���s�̒��̈ʒu
	Token	token;					// �g�[�N���̎��
	char	labelbuf[256];			// token��TK_LABEL���̎��̃o�b�t�@
	LONG	numbuf;					// token��TK_NUM�̎��̃o�b�t�@
	bool	bPeeked;				// ���ł�PeekToken()�Ńg�[�N���𓾂Ă��邩
	int		nErrorCount;			// ���̃\�[�X�ŋN�������G���[�̐�

	void	Init(void);				// �o�b�t�@�̏�����
	HRESULT	ReadLine(void);			// �P�s�ǂݍ���
	bool	IsToken(LPSTR &lp,LPSTR lp2);	// ��v���Ă����lp1���g�[�N���̏I���܂Ői�߂�
	void	CopyLabel(LPSTR& lpPos);		// lpPos���玟�̃g�[�N���܂ŃR�s�[
	HRESULT	NumCheck(LPSTR& lpPos);			// ���ׂĐ��l�Ȃ��numbuf�ɁB�����łȂ���Ζ߂�l�F��0
	void	GetQuotedLabel(LPSTR &lpPos);	// ���p���x���𓾂�
	Token	PeekToken2(void);				// PeekToken()�̖{��
	void	Error(LPSTR str);				// �G���[���b�Z�[�W��\��

  public:
	ScannerSub(){ lpLogFP=stderr; nErrorCount=0; Init(); }
	~ScannerSub(){}

#ifdef WINVC
	HRESULT	ReadFile(string& filename);// �t�@�C����ǂݍ���
#else
	HRESULT	ReadFile(string filename);// �t�@�C����ǂݍ���
#endif
	
	Token	GetToken(void);			// ����token�𓾂�i�ǂݍ��݃|�C���^�i�߂�j
	Token	PeekToken(void){ token=PeekToken2(); return token; }
									// ����token�𓾂�i�ǂݍ��݃|�C���^�i�߂Ȃ��j
	LPSTR	GetLabel(void){ return labelbuf; }	// token��TK_LABEL���̎��A���̃��x����������
	LONG	GetNum(void){ return numbuf; }		// token��TK_NUM�̎��A���̐��l��������

	int		GetScanline(void){ return nLine; }
	string	GetFileName(void){ return Mdl.GetFileName(); }
	void	SetLogFile(FILE* fp){ lpLogFP = fp; }
	int		GetErrorCount(void){ return nErrorCount; }
};

typedef stack<ScannerSub*> StacklpScannerSub;

class Scanner{
	StacklpScannerSub files;
	int		nIncludeNest;			// �C���N���[�h�̃l�X�g���B�Ȃ��̎��� 0
	FILE*	lpLogFP;				// �G���[���b�Z�[�W�o�͗p�t�@�C���|�C���^
	int		nErrorCount;			// �����͕��ŋN�������G���[�̐�

  public:
	Scanner(){ lpLogFP=stderr; nIncludeNest=0; nErrorCount=0; }
	~Scanner(){ while(!files.empty()){ DELETE_SAFE(files.top()); files.pop(); } }

#ifdef WINVC
	HRESULT	ReadFile(string& filename);// �t�@�C����ǂݍ���
#else
	HRESULT	ReadFile(string filename);// �t�@�C����ǂݍ���
#endif

	Token	GetToken(void);		// ����token�𓾂�i�ǂݍ��݃|�C���^�i�߂�j
	Token	PeekToken(void);	// ����token�𓾂�i�ǂݍ��݃|�C���^�i�߂Ȃ��j
	LPSTR	GetLabel(void){ return files.top()->GetLabel(); }
								// token��TK_LABEL���̎��A���̃��x����������
	LONG	GetNum(void){ return files.top()->GetNum(); }
								// token��TK_NUM�̎��A���̐��l��������
	int		GetScanline(void){ return files.top()->GetScanline(); }
	string	GetFileName(void){ return files.top()->GetFileName(); }
	void	SetLogFile(FILE* fp){ lpLogFP = fp; }
	int		GetErrorCount(void){ return nErrorCount; }
};

#endif
