#include "windows.h"

/* �R���p�N�g�ł͂��邪�A�x�� */

void *malloc(unsigned int bytes)
{
	return HeapAlloc(GetProcessHeap(), 0, bytes);
}
