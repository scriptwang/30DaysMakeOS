#include "windows.h"

/* �R���p�N�g�ł͂��邪�A�x�� */

void free(void *p)
{
	HeapFree(GetProcessHeap(), 0, p);
	return;
}
