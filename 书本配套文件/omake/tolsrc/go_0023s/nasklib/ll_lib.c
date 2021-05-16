#include "../include/stdio.h"

#define DEBUG		0

UCHAR *LL_skip_expr(UCHAR *p);
UCHAR *LL_skip_mc30(UCHAR *s, UCHAR *bytes, char flag);

UCHAR *LL_skipcode(UCHAR *p)
{
	UCHAR c, len;
retry:
	c = *p++;
	if (0x30 <= c && c <= 0x37) {
		p += c - 0x30; /* 0�`7�o�C�g�o�� */
		goto fin;
	}
	if (0x78 <= c && c <= 0x7f)
		goto fin; /* virtual-byte�W�J */
	if (0x70 <= c && c <= 0x77)
		goto retry; /* virtual-byte��` */
	if (0xe0 <= c && c <= 0xef)
		goto fin; /* �p�����[�^�Ȃ�1�o�C�g���}�[�N */
	if (0xf0 <= c && c <= 0xf7) {
		p += c - (0xf0 - 1); /* �p�����[�^�t�����}�[�N(1�`8) */
		goto fin;
	}
	if (c == 0x0e) {
		p = LL_skip_expr(p); /* �v���O�����J�E���^���x����` */
		goto fin;
	}
	if (c == 0x2d) {
		p = LL_skip_expr(p); /* equ���x����` */
		p = LL_skip_expr(p);
		goto fin;
	}
	if (0x0c <= c && c <= 0x0d) { /* equ���x���W�J */
		p += 4;
		goto fin;
	}
	if (c == 0x0f) {
		p += p[1] - (0x08 - 3); /* GLOBAL�p�ϊ� */
		goto fin;
		/* 0x0f 3 0x08 xx */
	}
	if (c == 0x2c) { /* extern���x����` */
		p = LL_skip_expr(p);
		goto fin;
	}
	if (0x2e <= c && c <= 0x2f) { /* �����P�[�V������� */
		p += 3;
		goto fin;
	}
	if (0x38 <= c && c <= 0x3f) {
		p = LL_skip_expr(p + 1); /* 1�`8�o�C�g���o�� */
		goto fin;
	}
	if (c == 0x58) {
		p = LL_skip_expr(p); /* ORG */
		goto fin;
	}
	if (c == 0x5a) { /* ORG������ */
		p += 4;
		goto fin;
	}
	if (c == 0x68) {
		p++; /* endian */
		goto fin;
	}
	if (0x90 <= c && c <= 0x91) {
		/* microcode90 */
		/* microcode91 */
		len = ((p[2] /* z-bbb-d-aaa */ >> 7) & 0x01) + 2;
		p = LL_skip_expr(p + 3 /* code, decision, zbbbdaaa */);
		if (c == 0x91)
			len *= 2;
		do {
			p = LL_skip_mc30(p, NULL, 1);
		} while (--len);
		goto fin;
	}
	if (0x94 <= c && c <= 0x95) {
		/* microcode94 */
		/* microcode95 */
		p = LL_skip_expr(p + 3);
		len = *p++; /* len */
		goto skipmc94_skip;
		do {
			p = LL_skip_expr(p);
skipmc94_skip:
			p = LL_skip_mc30(p, NULL, 0);
			p = LL_skip_mc30(p, NULL, 1);
			if (c == 0x95)
				p = LL_skip_mc30(p, NULL, 1);
		} while (--len);
		goto fin;
	}
	if (c == 0x59) {
		p = LL_skip_expr(p + 4); /* TIMES microcode (prefix) */
		p = LL_skip_expr(p);
		goto fin;
	}

	#if (DEBUG)
		fprintf(stderr, "LL_skipcode error:%02X\n", c);
		GOL_sysabort(GO_TERM_BUGTRAP);
	#endif

fin:
	return p;
}

UCHAR *LL_skip_expr(UCHAR *expr)
{
	UCHAR c;
	c = *expr++;
	if (c <= 6) {
		/* �萔 */
		c >>= 1;
		expr += c + 1;
		goto fin;
	}
	if (c == 0x07)
		goto fin;
	if (c <= 0x0b) {
		/* ���x�����p */
		expr += c - 7; /* c - 8 + 1 */
		goto fin;
	}
	#if (DEBUG)
		if (c < 0x10)
			goto dberr;
	#endif
	if (c < 0x20) {
		expr = LL_skip_expr(expr);
		if (c <= 0x12)
			goto fin; /* �P�����Z�q */
		expr = LL_skip_expr(expr);
		goto fin; /* �񍀉��Z�q */
	}

	#if (DEBUG)
		if (c < 0x80)
			goto dberr;
	#endif
	if (c == 0x80) {
		/* subsect size sum */
		expr = LL_skip_expr(expr);
		expr = LL_skip_expr(expr);
		goto fin;
	}
	#if (DEBUG)
dberr:
		fprintf(stderr, "LL_skip_expr:%02x\n", c);
		GOL_sysabort(GO_TERM_BUGTRAP);
	#endif

fin:
	return expr;
}

UCHAR *LL_skip_mc30(UCHAR *s, UCHAR *bytes, char flag)
{
	static char mc98_width[] = { 1, 1, 2, 2, 4, 4, 1 };
	UCHAR c;

	c = *s++;
	#if (DEBUG)
		if (c < 0x30)
		goto dberr;
	#endif
	if (c < 0x38) {
		s += (c &= 0x07);
		goto fin;
	}
	if (c < 0x3c) {
		c -= 0x37;
		s = LL_skip_expr(s + 1 /* �����W��ǂݔ�΂� */);
		goto fin;
	}
	#if (DEBUG)
		if (c < 0x98)
			goto dberr;
	#endif
	if (c < 0x9f) {
		c = mc98_width[c & 0x07];
		if (flag)
			s = LL_skip_expr(s);
		goto fin;
	}
	#if (DEBUG)
dberr:
		fprintf(stderr, "LL_skip_mc30:%02x\n", c);
		GOL_sysabort(GO_TERM_BUGTRAP);
	#endif
fin:
	if (bytes)
		*bytes = c;
	return s;
}
