/*
	�e�L�X�g�t�@�C�����W���[�����N���X�@�`textmodule.h + textmodule.cpp�`
															Ver.[2000/02/17]
*/
#ifndef	__TEXTMODULE_H
#define	__TEXTMODULE_H

#pragma warning(disable:4786)

#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <list>
#include <map>
#include <stack>

using namespace std;


class	TextModule{
  public:
// �ȉ���ɂ��AString�̎��̂�string��wstring���ɂ͈ˑ����Ȃ��悤�ɂ���
	typedef string					String;
	typedef	unsigned char			Letter;
	typedef	long					SizeType;
	typedef	list<String>			ListString;
	typedef	ListString::iterator	LineData;

  protected:
	ListString	TextData;			// �e�L�X�g�f�[�^��ۑ�����o�b�t�@
	
	String		FileName;			// ���݊֘A�Â���ꂽ�t�@�C����
	LineData	LineItr;			// ���݂̍s�̃C�e���[�^
	int			LinePos;			// ���݂̍s�ԍ�
	bool		BufferEOF;			// EOF�Ȃ�true�ɂȂ�

  public:
	TextModule(){ LineItr=TextData.begin(); LinePos=1; BufferEOF=false; }
	~TextModule(){}
	
	void		New();					// �V�K�쐬
	void		Open(String& filename);	// �J��
	void		Close(){ New(); }		// ����
	void		Save();					// �㏑���ۑ�
	void		Save(String& filename){ FileName=filename; Save(); }	// ���O��t���ĕۑ�
	
	String		GetFileName(){ return FileName; }	// �t�@�C�����𓾂�
	SizeType	GetLinePos(){ return LinePos; }		// �����s�𓾂�
//	SizeType	GetFileSize();						// �t�@�C���T�C�Y�𓾂�
	SizeType	GetMaxLinePos(){ return TextData.size(); }	// �ő�s���ŏI�s
	bool		IsEOF(){ return BufferEOF; }		// EOF�𒲂ׂ�
	
	void		NextLine(SizeType line=1);		// ���̍s�֐i��
	void		PrevLine(SizeType line=1);		// �O�̍s�֖߂�
	void		SeekLine(SizeType linepos);		// �s�ԍ��֐i��
	void		SeekLine(LineData);				// �s�֐i��
	SizeType	Seek(LineData);					// �s��񂩂�s�ԍ��𓾂�

	String		PeekLine();						// �s�ǂݍ��݁B���ɐi�܂Ȃ�
	String		GetLine();						// �s�ǂݍ��݁B���ɐi��

#ifdef WINVC
	void		PutLine(String& str){ InsertLine(LineItr, str); }
#else
	void		PutLine(String str){ InsertLine(LineItr, str); }		// �s�������݁B���ɐi��
#endif
	LineData	ReserveLine(){ return InsertLine(LineItr, String()); }	// �󔒍s��}���B�u�b�N�}�[�N�Ƃ��Ďg���B�g������EraseLine()�ŏ����̂���
#ifdef WINVC
	LineData	InsertLine(LineData, String&);
#else
	LineData	InsertLine(LineData, String);	// �s�}��
#endif
	void		EraseLine(LineData);			// �s����
};

#endif
