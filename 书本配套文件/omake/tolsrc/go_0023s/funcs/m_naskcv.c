char *wordsrch(char *s, const char *t);
char *cwordsrch(char *s, const char *t);
void cnv_lea(char *p);

static char leaopt = 0;

UCHAR *convmain(UCHAR *src0, UCHAR *src1, UCHAR *dest0, UCHAR *dest1, struct STR_FLAGS flags)
{
	UCHAR *p, *q;
	int obj = 0, i, c;
	UCHAR farproc = 0;
	static char *ERASELIST[] = {
		"ENDP", "ENDS", "END", "ASSUME", NULL
	};
	static UCHAR linebuf0[1000];
	UCHAR *linebuf = linebuf0;

	leaopt = flags.opt[FLAG_L];
	dest0_ = dest0;
	dest1_ = dest1;

	output(16 + LEN_NL, (flags.opt[FLAG_W]) ? "[FORMAT \"WCOFF\"]" NL : "[FORMAT \"BIN\"  ]" NL);
	output(18 + LEN_NL + 12 + LEN_NL + 10 + LEN_NL + 9 + LEN_NL,
		"[INSTRSET \"i486p\"]" NL
		"[OPTIMIZE 1]" NL
		"[OPTION 1]" NL
		"[BITS 32]" NL
	);

	for (;;) {
		p = src0;
		for (;;) {
			if (p >= src1)
				break;
			if (*p++ == '\n')
				break;
		}
		if (p == src0)
			break;
		if (p - src0 > (int) (sizeof linebuf0) - 1) {
			/* ��������...�����ɍ���̂łƂ肠�����f�ʂ肷�� */
			output(p - src0, src0);
			src0 = p;
			continue;
		}
		q = linebuf;
		do {
			*q++ = *src0++;
		} while (src0 < p);
		*q = '\0';
		if (strchr(linebuf, '\"'))
			goto output; // �ϊ����Ȃ�

		// �����Ɂu"�v�����������̂ŁA�����Ȃ��ϊ�

		// segment�����o
		if (cwordsrch(linebuf, "SEGMENT")) {
			output(15 + LEN_NL, cwordsrch(linebuf, "CODE")
				? "[SECTION .text]" NL : "[SECTION .data]" NL);
			continue;
		}

		// proc�����o
		if (p = cwordsrch(linebuf, "PROC")) {
			farproc = (cwordsrch(p, "FAR") != NULL);
			for (p = linebuf; *p <= ' '; p++);
			while (*p > ' ')
				p++;
			p[0] = ':';
			#if (LEN_NL == 1)
				p[1] = '\n';
				p[2] = '\0';
			#else
				p[1] = '\r';
				p[2] = '\n';
				p[3] = '\0';
			#endif
		//	goto output; // ���̕ϊ��͂������Ȃ�
		}

		for (i = 0; ERASELIST[i]; i++) {
			if (cwordsrch(linebuf, ERASELIST[i]))
				goto noout; // ��؏o�͂��Ȃ�
		}

		// ret�����o
		if (p = cwordsrch(linebuf, "RET")) {
			p += 3;
			for (q = p; *q; q++);
			while (p <= q) {
				*(q + 1) = *q;
				q--;
			}
			p[-3] = 'R';
			p[-2] = 'E';
			p[-1] = 'T';
			p[ 0] = farproc ? 'F' : 'N';
		}

		// ���[�J�����x���ϊ�
		while (p = strchr(linebuf, '#')) {
			*p = '.';
		}

		// LEA�����o
		if (p = cwordsrch(linebuf, "LEA"))
			cnv_lea(p);

		/* �ȈՔ�����@�Ńp�����[�^�����o */
		/* �E�Ō��":"���t���Ă���j�[���j�b�N�̓��x���錾�Ɖ��� */
		p = linebuf;
		do {
			while (*p != '\0' && *p <= ' ')
				p++;
			while (*p > ' ')
				p++;
			if (*p == '\0')
				break;
		} while (p[-1] == ':');
		if (*p != '\0') {
			while (q = cwordsrch(p, "OR")) {
				q[0] = '|';
				q[1] = ' ';
			}
			while (q = cwordsrch(p, "AND")) {
				q[0] = '&';
				q[1] = ' ';
				q[2] = ' ';
			}
			while (q = cwordsrch(p, "XOR")) {
				q[0] = '^';
				q[1] = ' ';
				q[2] = ' ';
			}
			while (q = cwordsrch(p, "NOT")) {
				q[0] = '~';
				q[1] = ' ';
				q[2] = ' ';
			}
		}

		// ptr����
		while (p = cwordsrch(linebuf, "PTR")) {
			p[0] = p[1] = p[2] = ' ';
		}

		// offset����
		while (p = cwordsrch(linebuf, "OFFSET")) {
			p[0] = p[1] = p[2] = p[3] = p[4] = p[5] = ' ';
		}

		// dword, word, byte���� (�啶���͎c��)
		if (flags.opt[FLAG_S] != 0 && strchr(linebuf, '[') == NULL) {
			while (p = wordsrch(linebuf, "dword")) {
				p[0] = ' ';
			}
			while (p = wordsrch(linebuf, "word")) {
				p[0] = p[1] = p[2] = p[3] = ' ';
			}
			while (p = wordsrch(linebuf, "byte")) {
				p[0] = p[1] = p[2] = p[3] = ' ';
			}
		}

output:
		output(strlen(linebuf), linebuf);
noout:
		;
	}
	output(LEN_NL, NL);
	return dest0_;
}

void cnv_lea(char *p)
// LEA�����o
{
	char *q;

	// LEA������Z�O�����g�I�[�o�[���C�h�v���t�B�N�X����菜��
	if ((q = strstr(p + 3, "S:[")) || (q = strstr(p + 3, "s:["))) {
		q[-1] = ' '; // 'E', 'C', 'S', 'D', 'F', or 'G'
		q[ 0] = ' '; // 'S'
		q[ 1] = ' '; // ':'
	}

	// LEA���ŁA�萔MOV�ɕϊ��\�Ȃ�ϊ�����
	if (leaopt && (q = strchr(p + 3, '['))) {
		char *q0 = q++;
		do {
			while ((*q == '+' || *q == '-' || *q == '*'
				|| *q == '/' || *q == '(' || *q == ')'
				|| *q <= ' ') && *p != '\0')
				q++;
			if ('0' <= *q && *q <= '9') {
				while (*q > ' ' && *q != '+' && *q != '-'
					&& *q != '*' && *q != '/' && *q != ']')
					q++;
			} else
				break;
		} while (*q != ']' && *q != '\0');
		if (*q == ']') {
			p[0] = 'M';
			p[1] = 'O';
			p[2] = 'V';
			*q0 = ' ';
			*q = ' ';
		}
	}

	// LEA�����́udword�v�A�uword�v�A�ubyte�v�̏���
	if (q = cwordsrch(p, "DWORD")) {
		q[4] = ' ';
		goto LEA_space4;
	}
	if (q = cwordsrch(p, "WORD"))
		goto LEA_space4;
	if (q = cwordsrch(p, "BYTE")) {
LEA_space4:
		q[0] = ' ';
		q[1] = ' ';
		q[2] = ' ';
		q[3] = ' ';
	}
	return;
}

char *wordsrch(char *s, const char *t)
// s�̒���t�����邩�ǂ����𒲂ׂ�
// strstr�Ƃ̈Ⴂ�́Awordsrch���P��P�ʂŌ������邱�Ƃł���
{
	char *p = s, c;
	int l = strlen(t);

	for (p = s; p = strstr(p, t); p += l) {

		// �P��̍��͌�̐؂�ڂ��H
		if (p > s) {
			c = p[-1];
			if (c > ' ' && c != ',')
				continue;
		}

		// �P��̉E�͌�̐؂�ڂ��H
		c = p[l];
		if (c <= ' ' || c == ',')
			return p;
	}

	return NULL;
}

unsigned char tolower(unsigned char c)
{
	return ('A' <= c && c <= 'Z') ? c + ('a' - 'A') : c;
}

char *cwordsrch(char *s, const char *c)
// �啶�����w�肷��Ώ������ł��T��
{
	char *r, *p, l[100];

	if ((r = wordsrch(s, c)) == NULL) {
		for (p = l; *p++ = tolower(*c++); );
		r = wordsrch(s, l);
	}
	return r;
}
