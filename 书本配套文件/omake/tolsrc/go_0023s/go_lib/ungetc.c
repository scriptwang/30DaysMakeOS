/* copyright(C) 2002 H.Kawai (under KL-01). */

#include <stdio.h>
#include <stdlib.h>

/* �ǂ񂾕��������ǂݖ߂��Ȃ��o�[�W���� */
/* �{����ungetc�Ƃ͎d�l���قȂ� */

int GO_ungetc(int c, GO_FILE *stream)
{
	if (stream->p0 < stream->p && stream->p[-1] == c) {
		stream->p--;
		return c;
	}
	fputs("GO_ungetc:error!\n", stderr);
	abort();
}
