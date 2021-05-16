#include <string.h>
#include "module.h"
#ifdef LINUX
#include <sys/stat.h>
unsigned int filelength(int fd){
  struct stat st;
  if (fstat(fd, &st))
    return 0;
  return st.st_size;
}
#endif

//���݂ł̓f�B���N�g���̌����Ȃǂ͂��Ă��Ȃ��̂ŁA�P����""��<>���͂�������
string	Module::MakeFullPath(string& p){
	string	str = p.substr(0, 0);
	if(str == "\"" || str == "\'" || str == "<"){
		str = p.substr(1, str.size()-1);
	}else{
		str = p.substr();
	}
	return str;
}


HRESULT	Module::ReadFile(string& filename){
	FILE* lpFP;

	Release();
	FileName = MakeFullPath(filename);
	lpFP = fopen(FileName.c_str(), "rb");
	if(lpFP == NULL){
		return 1;	// fileopen���s
	}
	
	dwMdlSize = filelength(fileno(lpFP));

	if (dwMdlSize == 0){
	  fclose(lpFP);
	  return 2;
	}

	lpMdlAdr = (LPVOID) new unsigned char[dwMdlSize];
	
	if(lpMdlAdr == NULL){
		dwMdlSize = 0;
		fclose(lpFP);
		return 3;	// �������m�ێ��s
	}
	if(fread(lpMdlAdr, 1, dwMdlSize, lpFP) != dwMdlSize){
		dwMdlSize = 0;
		fclose(lpFP);
		return 4;	// �ǂݍ��ݎ��s
	}
	if(fclose(lpFP)){
		lpMdlAdr = NULL;
		dwMdlSize = 0;
		return 5;	// fileclose���s
	}
	lpMdlPos = (LPSTR)lpMdlAdr;
	return 0;		// ����I��
}

HRESULT Module::ReadLine(LPSTR buf){
	int i, j;
	
	if(lpMdlPos == NULL) return 3;
	j = ((LPSTR)lpMdlAdr + dwMdlSize) - lpMdlPos;
	if (j <= 0) return 1;	// �����t�@�C�����Ȃ�
	if (j > 1023) i = 1023;	// �o�b�t�@�T�C�Y�œ��ł�
	for(i = j; i > 0; i--){
		if(*lpMdlPos == 0x0D && *(lpMdlPos+1) == 0x0A){
			*buf = '\0';
			lpMdlPos += 2;
			return 0;	// �P�s�I��
		}

		if (*lpMdlPos == 0x0a || *lpMdlPos == 0x0d){
		  *buf = '\0';
		  lpMdlPos++;
		  return 0;
		}

		*(buf++) = *(lpMdlPos++);
	}
	*buf = '\0';
	if (j < 1023)
	  return 0;		// �t�@�C���I�[
	else
	  return 2;	// �o�b�t�@���ӂ�
}
