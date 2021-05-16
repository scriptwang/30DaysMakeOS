#include "windows.h"
#include <stdio.h>
#include <stdlib.h>

int fclose(FILE *stream)
{
	if (stream == NULL)
		return EOF; /* �I�[���N���[�Y�͂܂��T�|�[�g���ĂȂ� */
	if (CloseHandle(stream->handle) == 0)
		return EOF;
	free(stream);
	return 0;
}
