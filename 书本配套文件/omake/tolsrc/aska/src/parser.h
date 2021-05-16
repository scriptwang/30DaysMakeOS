/*
	�\����̓N���X�@�`parser.h + parser.cpp�`
*/
#ifndef	__PARSER_H
#define	__PARSER_H

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
#include "generator.h"
#include "filepath.h"

class Parser{
  public:
	typedef	Generator::LineData	LineData;

  private:
	Scanner		scanner;			// �X�L�����N���X
	Generator	generator;			// �R�[�h�W�F�l���[�^
	
	int			StructAlignCount;	// align�̂��߂̋^�����x����
	int			LoopLabel[32];		// �l�X�g����loop����break���邽��
	int			LoopLabelPoint;		// �l�X�g��
	int			LocalLabelCounter;	// ��������⃋�[�v�œ����I�Ɏg�����x��
	int			StatementLevel;		// statement�̃��x��
	LPSTR		lpFunctionName;		// ���ݏ������Ă���֐���
	Parameter	defaultlocal;		// ���[�J���ϐ��plocal == SS:EBP;
	SegmentList*defaultsegment;		// �֐�������t����default segment
	SegmentList*defaultdatasegment;	// �ϐ�������t����default segment
	int			SysVarLocalValue;	// �֐����̃��[�J���ϐ��̈�̃o�C�g��
	
	LPSTR		lpLogFileName;		// �G���[���b�Z�[�W�o�̓t�@�C����			
	FILE*		lpLogFP;			// �G���[���b�Z�[�W�o�͗p�t�@�C���|�C���^
	int			nErrorCount;		// �\����͕��ŋN�����G���[�̐�

	//int			stackpoint;		// �֐����Ăяo���ꂽ�Ƃ���stackpoint����add���Ă����

	// �ȉ�HRESULT��������̂́A����I������0�A���ʂ̃��[�`���ŃG���[���N�����Ƃ���1�A
	// ����ȊO�̐����͊֐������ł̃G���[�Ƃ����Ӗ������B�i�܂�!=0�Ȃ�G���[�j
	HRESULT	Sizeof(Parameter& param);
	HRESULT	Address(Parameter& param);
	HRESULT	LocalAddress(Parameter& param);
	HRESULT	Selector(Parameter& param, LabelList* label);
	HRESULT	Selector2(Parameter& param);
	void	Member2Param(Parameter& param, MemberList* member);
	HRESULT	Param2Param(Parameter& to, Parameter& from);
	HRESULT	Array(Parameter& param);
	void	PointerCheck(RegisterList* reg);
	HRESULT	ArrayReg(Parameter& param, RegisterList* reg);
	HRESULT	Pointer(Parameter& param);
	HRESULT	GetParameter(Parameter& param);
	HRESULT	Cast(Parameter& param);
	HRESULT	Immedeate(Parameter& param);
	void	Expression(void);
	void	Statement(void);
	void	DefineFunction(void);
	void	DefineVariable(void);
	void	InitialArray(LabelList* label);
	void	DefineAlias(LabelList* label);
	void	DefineInitial(LabelList* label, LPSTR buf);
	void	DefineStruct(void);
	void	DefineMember(TagList* tag);
	void	DefineSegment(void);
	void	DefineDefault(void);
	void	StatementSequence(void);
	void	IfStatement(void);
	CompareType Compare(void);
	CompareType IsCmpOperator(bool bSigned);
	CompareType TransCompare(CompareType cmptype);
	CompareType FlagCompare(void);
	void	LoopStatement(void);
	void	BreakStatement(CompareType cmptype = C_JMP);
	void	AltStatement(void);
	void	AsmoutStatement(void);
	void	ForStatement(void);
	void	WhileStatement(void);
	void	DoStatement(void);
	void	ContinueStatement(CompareType cmptype = C_JMP);
	void	AssumeSegment(void);


//	void	alt_statement(void);
//	bool	case_block(DWORD dw); // ���x��ID
	void	StartParse(void);

	void	Error(LPSTR str);

  public:
	Parser(void);
	~Parser(){}
	
#ifdef WINVC
	HRESULT	Compile(string& filename);
#else
	HRESULT	Compile(string filename);
#endif

#ifdef WINVC
	HRESULT	Compile(string& filename, string& outfilename);
#else
	HRESULT	Compile(string filename, string outfilename);
#endif

	void	SetLogFile(LPSTR filename);
	int		GetErrorCount(void){ return nErrorCount; }
};

#endif
