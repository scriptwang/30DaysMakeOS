/*
	�f�[�^�e�[�u���N���X�@�`table.h + table.cpp�`
*/
#ifndef	__TABLE_H
#define	__TABLE_H

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
#include "scanner.h"

enum RegisterType{
	R_GENERAL,		// �ėp���W�X�^
	R_SEGREG,		// �Z�O�����g���W�X�^
	R_CTRL,			// �R���g���[�����W�X�^
	R_DEBUG,		// �f�o�b�O���W�X�^
	R_TEST,			// �e�X�g���W�X�^
	R_FLOAT,		// ���������_���W�X�^
	R_MMX,			// MMX���W�X�^
};

class RegisterList{
  public:
	LPSTR			name;	// ���W�X�^��
	RegisterType	type;	// ���W�X�^�^�C�v
	int				size;	// ���W�X�^�T�C�Y
	bool			bBase;	// �x�[�X���W�X�^�ł��邩
	bool			bIndex;	// �C���f�b�N�X���W�X�^�ł��邩

	RegisterList(LPSTR n, RegisterType t, int s, bool b, bool i)
			{ name=n; type=t; size=s; bBase=b; bIndex=i; }
};

typedef map<string, RegisterList*> MaplpRegisterList;

class Register{
	MaplpRegisterList	mapregister;
	
  public:
	Register(void);
	~Register();
	
	void			Add(LPSTR key, RegisterList* r){ string k=key; mapregister[k]=r; }
	RegisterList*	Find(LPSTR key){
		string k = key;
		MaplpRegisterList::iterator it = mapregister.find(k);
		if(it == mapregister.end()) return NULL; else return it->second;
	}
};

class LabelList;

class StaticDataList{
  public:
	LPSTR			name;
	LabelList*		label;
	LPSTR			lpInit;
};

typedef list<StaticDataList> ListStaticDataList;

class SegmentList{
  public:
	ListStaticDataList liststatic;

	LPSTR			name;			// �Z�O�����g�l�[��
	Token			align;			// �A���C������
	Token			combine;		// �R���o�C������
	Token			use;			// USE����
	Token			access;			// �A�N�Z�X����
	LPSTR			segmentclass;	// �Z�O�����g�N���X

	RegisterList*	assume;			// segment CODE == ES;��ES��ۑ����邽�߂Ɏg��

	SegmentList(void)
		{ name=NULL; align=TK_BYTE; combine=TK_PRIVATE; use=TK_USE32;
			access=TK_ER; segmentclass=NULL; assume=NULL; }
	SegmentList(LPSTR n, Token al, Token co, Token us, Token ac, LPSTR cl)
		{ name=n; align=al; combine=co; use=us; access=ac; segmentclass=cl; assume=NULL; }

	void	AddStaticData(LPSTR varname, LabelList* label, LPSTR initialize);
};

typedef map<string, SegmentList*> MaplpSegmentList;

class Segment{
  public:
	MaplpSegmentList	mapsegment;
	
	Segment(void);
	~Segment();
	
	void			Add(LPSTR key, SegmentList* s){ string k=key; mapsegment[k]=s; }
	SegmentList*	Find(LPSTR key){
		string k = key;
		MaplpSegmentList::iterator it = mapsegment.find(k);
		if(it == mapsegment.end()) return NULL; else return it->second;
	}
};

class TagList;

class MemberList{
  public:
	bool			bSigned;	// ��������^���ǂ���
	TagList*		type;		// �ϐ��̌^
	Token			ptype;		// near:TK_NEAR, far:TK_FAR
	int				pdepth;		// �|�C���^�̐[���B���ꂪ0�Ȃ�|�C���^�łȂ�
	bool			bArray;		// �z��ł��邩
	int				size;		// �ϐ��̃T�C�Y�i�z��A�|�C���^�A�\���̂������j
	int				offset;		// �\���̂ł̃����o�̃I�t�Z�b�g

	MemberList(void)
		{ type=NULL; bSigned=true; ptype=TK_FAR; pdepth=0; bArray=false; size=0; offset=0; }
	MemberList(bool bs, TagList* t, Token pt, int pd, bool ba, int s, int o)
		{bSigned=bs; type=t; ptype=pt; pdepth=pd; bArray=ba; size=s; offset=o; }
	MemberList(MemberList* m)
		{bSigned=m->bSigned; type=m->type; ptype=m->ptype; pdepth=m->pdepth;
			bArray=m->bArray; size=m->size; offset=m->offset; }
	MemberList&	operator=(MemberList& label);
};

typedef map<string, MemberList*> MaplpMemberList;

// typedef��͂��߂��炠��^��"type"�Ƃ��������o������Btypedef�Ȃ�member->type!=NULL
class TagList{
  public:
	int				size;		// ���̌^�̃T�C�Y
	bool			bStruct;	// �\���̂ł��邩
	MaplpMemberList	mapmember;	// �\���̂ł���΃����o�̃��X�g
	
	TagList(bool bs){ bStruct=bs; size=0;}
	~TagList();

	void			AddMemberList(LPSTR n, bool bs, TagList* t, Token pt, int pd, bool ba, int s);
	MemberList*		FindMemberList(LPSTR key){
		string k = key;
		MaplpMemberList::iterator it = mapmember.find(k);
		if(it == mapmember.end()) return NULL; else return it->second;
	}
};

typedef map<string, TagList*> MaplpTagList;

class Tag{
	MaplpTagList	maptag;
	
  public:
	Tag(void);
	~Tag();
	
	void			Add(LPSTR key, TagList* t){ string k=key; maptag[k]=t; }
	TagList*		Find(LPSTR key){
		string k = key;
		MaplpTagList::iterator it = maptag.find(k);
		if(it == maptag.end()) return NULL; else return it->second;
	}
};


enum ParameterType{
	P_ERR,						// ���������������̓G���[
	P_REG,						// �p�����[�^�̃^�C�v�����W�X�^�Bbase�ɓ���
	P_MEM,						// �p�����[�^�̃^�C�v���������Bbase�ɓ���
	P_IMM,						// �p�����[�^�̃^�C�v�����W�X�^�Bbase�ɓ���
};

class Parameter{
  public:
	ParameterType	paramtype;	// P_ERR, P_REG, P_MEM, P_IMM
	RegisterList*	seg;
	RegisterList*	base;
	RegisterList*	index;
	int				scale;		// 1, 2, 4, 8
	string			disp;
	int				ndisp;		// �����̓����o�����̎��̃I�t�Z�b�g�̂��߂ɂ���
	bool			bLabel;		// P_MEM�ł�Label���ǂ���
	bool			bSigned;	// ��������^���ǂ���
	TagList*		type;		// �ϐ��̌^
	Token			ptype;		// near:TK_NEAR, far:TK_FAR, offset:TK_OFFSET, segment:TK_SEGMENT
	int				pdepth;		// �|�C���^�̐[���B���ꂪ0�Ȃ�|�C���^�łȂ�
	int				size;		// �ϐ��̃T�C�Y�i�z��A�|�C���^�A�\���̂������j
	bool			bArray;		// �z��ł��邩

	void	Initialize()
		{ paramtype=P_ERR; seg=NULL; base=NULL; index=NULL; scale=1; disp=""; ndisp=0; size=0;
			bLabel=false; bSigned=true; type=NULL; ptype=TK_FAR; pdepth=0; bArray=false; }
	
	Parameter(void){ Initialize(); }
	Parameter&	operator=(Parameter& param);
};

//typedef list<TagList*> ListlpTagList;

//label��type:dword, ptype:normal, bAddress:false, size:4�Ƃ���
class LabelList{
  public:
	bool			bStatic;
	bool			bSigned;	// ��������^���ǂ���
	TagList*		type;		// �ϐ��̌^
	Token			ptype;		// near:TK_NEAR, far:TK_FAR
	int				pdepth;		// �|�C���^�̐[���B���ꂪ0�Ȃ�|�C���^�łȂ�
	SegmentList*	segment;	// �f�[�^���ǂ��Ɋ���t���邩
	int				size;		// �ϐ��̃T�C�Y�i�z��A�|�C���^�A�\���̂������j
	bool			bArray;		// �z��ł��邩
	bool			bAlias;		// �G�C���A�X�ł��邩
	Parameter		alias;		// �G�C���A�X(�萔�͂����֐��������鎖�ōs��)
	int				nLocalAddress;	// ���[�J�����x���Ȃ烍�[�J���A�h���X������
	bool			bFunction;	// �֐��ł��邩
//	ListlpTagList	listtag;	// �֐��Ȃ�֐��̈����̌^������

	LabelList(void)
		{bSigned=true; type=NULL; ptype=TK_FAR; pdepth=0; segment=NULL; size=0; bArray=false;
			bAlias=false; nLocalAddress=0; bFunction=false; bStatic=false; }
	LabelList(bool bs, TagList* t, Token pt, int pd, SegmentList* s
			, int sz, bool bary, bool balias, Parameter& a, int nla, bool bf)
		{ bSigned=bs; type=t; ptype=pt; pdepth=pd; bArray=bary; bFunction=bf; nLocalAddress=nla;
			size=sz; bAlias=balias; alias=a; segment=s; bStatic=false; }
	LabelList(LabelList* l)
		{ bSigned=l->bSigned; type=l->type; ptype=l->ptype; pdepth=l->pdepth; bArray=l->bArray;
			bFunction=l->bFunction; nLocalAddress=l->nLocalAddress; size=l->size; bAlias=l->bAlias;
			alias=l->alias; segment=l->segment; bStatic=l->bStatic; }
	LabelList&	operator=(LabelList& label);
};

typedef map<string, LabelList*> MaplpLabelList;

class Label{
	MaplpLabelList	maplabel;

  public:
	Label(void){}
	~Label(){ Clear(); }
	
	void			Clear(void);
	void			Add(LPSTR key, LabelList* l){ string k=key; maplabel[k]=l; }
	LabelList*		Find(LPSTR key){
		string k = key;
		MaplpLabelList::iterator it = maplabel.find(k);
		if(it == maplabel.end()) return NULL; else return it->second;
	}
};

enum CompareType{
	C_NOTHING,	// �Ȃ�����
	C_JA,		// (unsigned)>
	C_JAE,		// (unsigned)>=
	C_JB,		// (unsigned)<
	C_JBE,		// (unsigned)<=
	C_JG,		//   (signed)>
	C_JGE,		//   (signed)>=
	C_JL,		//   (signed)<
	C_JLE,		//   (signed)<=
	C_JE,		// ==
	C_JNE,		// !=
	C_JC,		// CF == 1
	C_JNC,		// CF == 0
	C_JMP,		// 
};

#endif
