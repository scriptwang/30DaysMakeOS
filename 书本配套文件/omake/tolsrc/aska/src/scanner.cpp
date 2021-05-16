#include "scanner.h"

#ifdef WINVC
HRESULT	Scanner::ReadFile(string& filename){
#else
HRESULT	Scanner::ReadFile(string filename){
#endif

	ScannerSub* scannersubx = new ScannerSub;
	files.push(scannersubx);
	files.top()->SetLogFile(lpLogFP);
	return files.top()->ReadFile(filename);
}

Token	Scanner::GetToken(void){
	Token token;
	while(true){
		token = files.top()->GetToken();
		if(token == TK_INCLUDE){
			nIncludeNest++;
			files.top()->GetToken();	// include�̎���"�t�@�C����"�����o��
			ReadFile(string(files.top()->GetLabel()));	// ���蕶���t���œn���Ă�
			continue;
		}
//		if(token == TK_DEFINE){
//			files.top()->GetToken();	// include�̎���"�t�@�C����"�����o��
//			continue;
//		}
		if(token == TK_EOF){
			if(nIncludeNest == 0) break;		// �{���ɏI���
			nErrorCount += files.top()->GetErrorCount();
			DELETE_SAFE(files.top());
			files.pop();
			nIncludeNest--;
			continue;
		}
		break;
	}
	return token;
}

Token	Scanner::PeekToken(void){
	Token token;
	while(true){
		token = files.top()->PeekToken();
		if(token == TK_INCLUDE){
			nIncludeNest++;
			files.top()->GetToken();	// ���ɐi�߂�
			files.top()->GetToken();	// include�̎���"�t�@�C����"�����o��
			ReadFile(string(files.top()->GetLabel()));	// ���蕶���t���œn���Ă�
			continue;
		}
		if(token == TK_EOF){
			if(nIncludeNest == 0) break;	// �{���ɏI���
			nErrorCount += files.top()->GetErrorCount();
			DELETE_SAFE(files.top());
			files.pop();
			nIncludeNest--;
			continue;
		}
		break;
	}
	return token;
}

bool	ScannerSub::IsToken(LPSTR &lp, LPSTR lp2){
	bool	bReserved = false;		// �͂��߂̕��������Z�q�łȂ���Η\���ƌ��Ȃ�
	unsigned char c;
	LPSTR	lpx = lp;
	LPSTR	lpt = labelbuf;

#ifdef WINVC
	if(IS_CHARACTOR(*lpx)) bReserved = true;
#else
	if(IS_CHARACTOR((unsigned char)*lpx)) bReserved = true;
#endif

	while(true){
		*(lpt++) = *lp2;
		if(*lp2 == '\0'){
			c = *lpx;
			if(bReserved == true && IS_CHARACTOR(c)) return false;
			lp = lpx;		//token�̏I���̈ʒu�܂Ń|�C���^��i�߂�
			return true;
		}
		if(*(lpx++) != *(lp2++)) return false;
	}
}

// lpPos���玟�̃g�[�N���܂ŃR�s�[
void	ScannerSub::CopyLabel(LPSTR& lpPos){
	unsigned char c;
	LPSTR lp = labelbuf;
	while(true){
		c = *lpPos;
		if(IS_KANJI1(c)){
			*(lp++) = c;
			*(lp++) = c;	//�ق�Ƃ͂�����sjis��2�o�C�g�ڂ��̔��ʂ�����
			lpPos++;
		}else if(IS_ALNUM(c) || c == '#' || c == '@' || c == '$'){
			*(lp++) = c;
		}else{
			break;
		}
		lpPos++;
	}
	*lp = '\0';
}

// ���l�Ȃ��numbuf�ɁB�����łȂ���Ζ߂�l�F��0
// ������CopyLabel()�Ɠ��l�̋@�\���ǉ�����AGetLabel()�ł���悤�ɂȂ���
HRESULT	ScannerSub::NumCheck(LPSTR& lpPos){
	int num;
	unsigned char c;
	LPSTR lp = labelbuf;

	if(*lpPos < '0' || *lpPos > '9') return 1;
	if((*lpPos == '0') && (*(lpPos+1) == 'x')){
		num = 16;
		lpPos+=2;
	}else if((*lpPos == '0') && (*(lpPos+1) == 'b')){
		num = 2;
		lpPos+=2;
	}else{
		num = 10;
	}

	numbuf = 0;
	bool	first = true;
	while(true){
		c = *lpPos;
		switch(num){
		  case 10:		// 10�i�@
			if((c >= '0') && (c <= '9')){
				numbuf *= num;
				numbuf += (LONG)(c-'0');
			}else{
				*lp = '\0';
				return 0;
			}
			break;
		  case 16:		// 16�i�@
			if((c >= '0') && (c <= '9')){
				numbuf *= num;
				numbuf += (LONG)(c-'0');
			}else if((c >= 'A') && (c <= 'F')){
				if(first == true) *lp++ = '0';
				numbuf *= num;
				numbuf += (LONG)(c-'A') + 10;
			}else if((c>='a') && (c<='f')){
				if(first == true) *lp++ = '0';
				numbuf *= num;
				numbuf += (LONG)(c-'a') + 10;
			}else{
				*lp++ = 'H';
				*lp = '\0';
				return 0;
			}
			break;
		  case 2:		// 2�i�@
			if(c == '0' || c == '1'){
				numbuf *= num;
				numbuf += (LONG)(c-'0');
			}else{
				return 0;
			}
			*lp++ = 'B';
			*lp = '\0';
			break;
		}
		lpPos++;
		*(lp++) = c;
		first = false;
	}
}

void	ScannerSub::GetQuotedLabel(LPSTR &p){
	unsigned char c = *(p++);			// '��"��c�ɓ���
	LPSTR x = labelbuf;
	*(x++) = c;
	while(true){
		if(IS_KANJI1(*((unsigned char*)p))){
				*(x++) = *(p++);
		}else if(*p == c){
			*(x++) = c;
			p++;				// '�`'��"�`"�̎����w��
			break;
		}else if(*p == '\0') break;
		*(x++) = *(p++);
	}
	*x = '\0';
}

void	ScannerSub::Init(void){
	nLine = 0;
	lpPos	= linebuf;
	bPeeked = false;
	strcpy(linebuf,"");
}

HRESULT	ScannerSub::ReadLine(void){
	lpPos = linebuf;
	nLine++;
	return Mdl.ReadLine(linebuf);
}

void	ScannerSub::Error(LPSTR str){
	nErrorCount++;
	fprintf(lpLogFP, "%s(%d):(Scanner)%s \n", GetFileName().c_str(), GetScanline(), str);
}

#ifdef WINVC
HRESULT ScannerSub::ReadFile(string& filename){
#else
HRESULT ScannerSub::ReadFile(string filename){
#endif
	Init();
	return Mdl.ReadFile(filename);
}

Token	ScannerSub::GetToken(void){
	if(bPeeked){
		bPeeked = false;
		return token;
	}
	Token e = PeekToken();
	bPeeked = false;
	return e;
}

Token	ScannerSub::PeekToken2(void){
	int		nCommentNest = 0;

	if(bPeeked) return token;

	bPeeked = true;

	while(true){
		unsigned char c = *lpPos;
		if(c == ' ' || c == '\t'){ lpPos++; continue; }	// �X�y�[�X or TAB
		if(c == '\0'){									// �s��
			if(ReadLine()) return TK_EOF;				// EOF
			continue;
		}
		if(IsToken(lpPos,"/*")){						// �R�����g�s
			nCommentNest++;
			continue;	
		}
		if(IsToken(lpPos,"*/")){						// �R�����g�s
			nCommentNest--;
			continue;	
		}
		if(nCommentNest != 0){
			if(IS_KANJI1(c)) lpPos++;
			lpPos++;
			continue;
		}
		if(IsToken(lpPos,"//")){						// �R�����g�s
			if(ReadLine()) return TK_EOF;				// EOF
			continue;	
		}
		
		// �����͕��̖���
		if(IsToken(lpPos,"include"))return TK_INCLUDE;
		if(IsToken(lpPos,"#include"))return TK_INCLUDE;
		if(IsToken(lpPos,"define"))return TK_DEFINE;
		if(IsToken(lpPos,"#define"))return TK_DEFINE;


		// ����
		if(IsToken(lpPos,"if"))		return TK_IF;
		if(IsToken(lpPos,"else"))	return TK_ELSE;
		if(IsToken(lpPos,"loop"))	return TK_LOOP;
		if(IsToken(lpPos,"break"))	return TK_BREAK;
		if(IsToken(lpPos,"alt"))	return TK_ALT;
		if(IsToken(lpPos,"case"))	return TK_CASE;
		if(IsToken(lpPos,"default"))return TK_DEFAULT;
		if(IsToken(lpPos,"goto"))	return TK_GOTO;
		if(IsToken(lpPos,"return"))	return TK_RETURN;
		if(IsToken(lpPos,"struct"))	return TK_STRUCT;
		if(IsToken(lpPos,"asmout"))	return TK_ASMOUT;
		if(IsToken(lpPos,"for"))	return TK_FOR;
		if(IsToken(lpPos,"while"))	return TK_WHILE;
		if(IsToken(lpPos,"do"))		return TK_DO;
		if(IsToken(lpPos,"continue"))return TK_CONTINUE;

		// �^�錾
		if(IsToken(lpPos,"int"))	return TK_INT;
		if(IsToken(lpPos,"long"))	return TK_LONG;
		if(IsToken(lpPos,"short"))	return TK_SHORT;
		if(IsToken(lpPos,"char"))	return TK_CHAR;
		if(IsToken(lpPos,"dword"))	return TK_DWORD;
		if(IsToken(lpPos,"word"))	return TK_WORD;
		if(IsToken(lpPos,"byte"))	return TK_BYTE;
		if(IsToken(lpPos,"void"))	return TK_VOID;
		if(IsToken(lpPos,"near"))	return TK_NEAR;
		if(IsToken(lpPos,"far"))	return TK_FAR;
		if(IsToken(lpPos,"static"))	return TK_STATIC;
		if(IsToken(lpPos,"signed"))	return TK_SIGNED;
		if(IsToken(lpPos,"unsigned"))return TK_UNSIGNED;
		if(IsToken(lpPos,"offset"))	return TK_OFFSET;
		if(IsToken(lpPos,"segment"))return TK_SEGMENT;
		if(IsToken(lpPos,"rep"))	return TK_REP;
		if(IsToken(lpPos,"true"))	return TK_TRUE;
		if(IsToken(lpPos,"false"))	return TK_FALSE;
		if(IsToken(lpPos,"sizeof"))	return TK_SIZEOF;
//		if(IsToken(lpPos,"code"))	return TK_CODE;
//		if(IsToken(lpPos,"data"))	return TK_DATA;
//		if(IsToken(lpPos,"local"))	return TK_LOCAL;
		
		// �t���O�Ȃǂ̐錾
		if(IsToken(lpPos,"CF"))		return TK_CF;
		if(IsToken(lpPos,"ZF"))		return TK_ZF;

		// segment���߂̂��߂̐錾
		if(IsToken(lpPos,"DWORD"))	return TK_DWORD;
		if(IsToken(lpPos,"WORD"))	return TK_WORD;
		if(IsToken(lpPos,"BYTE"))	return TK_BYTE;
		if(IsToken(lpPos,"PARA"))	return TK_PARA;
		if(IsToken(lpPos,"PAGE"))	return TK_PAGE;
		if(IsToken(lpPos,"PAGE4K"))	return TK_PAGE4K;
		if(IsToken(lpPos,"PRIVATE"))return TK_PRIVATE;
		if(IsToken(lpPos,"PUBLIC"))	return TK_PUBLIC;
		if(IsToken(lpPos,"STACK"))	return TK_STACK;
		if(IsToken(lpPos,"COMMON"))	return TK_COMMON;
		if(IsToken(lpPos,"USE16"))	return TK_USE16;
		if(IsToken(lpPos,"USE32"))	return TK_USE32;
		if(IsToken(lpPos,"RO"))		return TK_RO;
		if(IsToken(lpPos,"EO"))		return TK_EO;
		if(IsToken(lpPos,"ER"))		return TK_ER;
		if(IsToken(lpPos,"RW"))		return TK_RW;

		if(IsToken(lpPos,"<="))		return TK_LE;
		if(IsToken(lpPos,">="))		return TK_GE;
		if(IsToken(lpPos,"=="))		return TK_EQ;
		if(IsToken(lpPos,"!="))		return TK_NE;

		if(IsToken(lpPos,"+="))		return TK_ADD;
		if(IsToken(lpPos,"-="))		return TK_SUB;
		if(IsToken(lpPos,"*="))		return TK_MULA;
		if(IsToken(lpPos,"/="))		return TK_DIVA;
		if(IsToken(lpPos,"&="))		return TK_ANDA;
		if(IsToken(lpPos,"|="))		return TK_ORA;
		if(IsToken(lpPos,"^="))		return TK_XORA;
		if(IsToken(lpPos,">>="))	return TK_SHRA;
		if(IsToken(lpPos,"<<="))	return TK_SHLA;
		if(IsToken(lpPos,"=-"))		return TK_NEG;
		if(IsToken(lpPos,"=~"))		return TK_CPLA;
		
		if(IsToken(lpPos,">>"))		return TK_SHR;
		if(IsToken(lpPos,"<<"))		return TK_SHL;
		if(IsToken(lpPos,"->"))		return TK_MEMBER;
		if(IsToken(lpPos,"++"))		return TK_INC;
		if(IsToken(lpPos,"--"))		return TK_DEC;
		if(IsToken(lpPos,"::"))		return TK_DCOLON;
		if(IsToken(lpPos,"&&"))		return TK_DAND;

		// �ꕶ���g�[�N��
		if(c=='<'){ lpPos++; return TK_BE; }
		if(c=='>'){ lpPos++; return TK_AB; }

		if(c=='.'){ lpPos++; return TK_DOT; }
		if(c==','){ lpPos++; return TK_COMMA; }

		if(c==';'){ lpPos++; return TK_DLM; }
		if(c==':'){ lpPos++; return TK_COLON; }

		if(c=='+'){ lpPos++; return TK_PLUS; }
		if(c=='-'){ lpPos++; return TK_MINUS; }
		if(c=='*'){ lpPos++; return TK_MUL; }
		if(c=='/'){ lpPos++; return TK_DIV; }
		if(c=='%'){ lpPos++; return TK_REM; }

		if(c=='|'){ lpPos++; return TK_OR; }
		if(c=='&'){ lpPos++; return TK_AND; }
		if(c=='^'){ lpPos++; return TK_XOR; }
		if(c=='~'){ lpPos++; return TK_CPL; }
		if(c=='!'){ lpPos++; return TK_NOT; }
		if(c=='.'){ lpPos++; return TK_DOT; }

		if(c=='{'){ lpPos++; return TK_LBR; }
		if(c=='}'){ lpPos++; return TK_RBR; }
		if(c=='('){ lpPos++; return TK_LPR; }
		if(c==')'){ lpPos++; return TK_RPR; }
		if(c=='['){ lpPos++; return TK_LSQ; }
		if(c==']'){ lpPos++; return TK_RSQ; }
		if(c=='='){ lpPos++; return TK_BECOME; }
//		if(c=='#'){ lpPos++; return TK_SHARP; }
//		if(c=='@'){ lpPos++; return TK_AT; }

		// '�`' �������� "�`"
		if(c=='\''){ GetQuotedLabel(lpPos); return TK_QUOTE; }
		if(c=='\"'){ GetQuotedLabel(lpPos); return TK_WQUOTE; }

		if(!NumCheck(lpPos)){	// ���l
			return TK_NUM;
		}

		CopyLabel(lpPos);			// lpPos���玟�̃g�[�N���܂ŃR�s�[
		return TK_LABEL;			// ���x��
	}
}
