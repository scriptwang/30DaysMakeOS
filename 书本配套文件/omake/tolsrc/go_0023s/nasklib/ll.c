/* "LL.c":Lilte-Linker */
/* Copyright(C) 2002 H.Kawai */

/* ���j�F�x���Ă���������A�Ƃ肠�����������ƁI */
/* �A���S���Y���ɋÂ�̂́A�����悤�ɂȂ��Ă��� */

#define	DEBUG			0

#include "../include/stdlib.h"

#if (DEBUG)
	#include "../include/stdio.h"
#endif

#define	INVALID_DELTA		0x40000000

extern int nask_maxlabels;

#define	MAXSIGMAS				  4

#define	UCHAR			unsigned char

struct STR_SIGMA {
	int scale;
	unsigned int subsect, terms;
};

struct STR_VALUE {
	int min;
	unsigned int delta, flags;
	int scale[2];
	unsigned int label[2];
	struct STR_SIGMA sigma[MAXSIGMAS];
};

#define	VFLG_ERROR			0x01	/* �G���[ */
#define	VFLG_SLFREF			0x02	/* ���ȎQ�ƃG���[ */
#define	VFLG_UNDEF			0x04	/* ����`�G���[ */
#define	VFLG_EXTERN			0x10	/* �O�����x�� */
#define	VFLG_CALC			0x20	/* �v�Z�� */
#define	VFLG_ENABLE			0x40	/* STR_LABEL�ŗL���Ȃ��Ƃ����� */

struct STR_LABEL {
	struct STR_VALUE value;
	UCHAR *define; /* ���ꂪNULL���ƁAextlabel */
};

struct STR_SUBSECTION {
	unsigned int min, delta, unsolved; /* unsolved == 0 �Ȃ�œK���̕K�v�Ȃ� */
	UCHAR *sect0, *sect1;
};

static struct STR_LABEL *label0;
static struct STR_SUBSECTION *subsect0;

unsigned int solve_subsection(struct STR_SUBSECTION *subsect, char force);
UCHAR *skip_mc30(UCHAR *s, UCHAR *bytes, char flag);
void calcsigma(struct STR_VALUE *value);
void addsigma(struct STR_VALUE *value, struct STR_SIGMA sigma);
UCHAR *LL_skip_expr(UCHAR *p);
UCHAR *LL_skip_mc30(UCHAR *s, UCHAR *bytes, char flag);
UCHAR *LL_skipcode(UCHAR *p);

void init_value(struct STR_VALUE *value)
{
	int i;
	value->min = value->delta = value->scale[0] = value->scale[1] = 0;
	value->flags = 0;
	value->label[0] = value->label[1] = -1;
	for (i = 0; i < MAXSIGMAS; i++) {
		value->sigma[i].scale = 0;
		value->sigma[i].subsect = 0;
		value->sigma[i].terms = 0;
	}
	return;
}

unsigned int get_id(int len, UCHAR **ps, int i)
/* len = 0�`3 */
{
	union UINT_UCHAR {
		unsigned int i;
		UCHAR c[4];
	} u;
	UCHAR *s = *ps;
	u.i = i;
	i = 0;
	do {
		u.c[i++] = *s++;
	} while (--len >= 0);
	*ps = s;
	return u.i;
}

void calc_value0(struct STR_VALUE *value, UCHAR **pexpr);

void calc_value(struct STR_VALUE *value, UCHAR **pexpr)
{
	calc_value0(value, pexpr);
	if (value->sigma[0].scale)
		calcsigma(value);
	return;
}

void enable_label(struct STR_LABEL *label)
{
	struct STR_VALUE value;
	UCHAR *t;

//	if (label->value.flags & VFLG_CALC) {
//		label->value.flags |= VFLG_SLFREF;
//		return;
//	}
//	init_value(&value);
//	value.label[0] = label - label0; /* for EXTERN */
//	value.scale[0] = 1;
//	value.flags |= label->value.flags & VFLG_EXTERN;
	if ((t = label->define) != NULL) {
		label->value.flags |= VFLG_CALC;
		calc_value0(&value, &t);
		if (value.flags & VFLG_SLFREF) {
			init_value(&value);
			value.flags |= VFLG_SLFREF;
		}
	} else /* if ((label->value.flags & VFLG_EXTERN) == 0) */ {
		/* EXTERN�̎��͂�����ENABLE�ɂȂ�̂ŁA�����ɗ���͂��Ȃ� */
		init_value(&value);
		value.flags |= VFLG_UNDEF;
	}
	label->value = value;
	label->value.flags |= VFLG_ENABLE;
	return;
}

void calc_value0(struct STR_VALUE *value, UCHAR **pexpr)
{
	UCHAR *expr = *pexpr, c, *t;
	int i, j, k;
	struct STR_VALUE tmp, tmp2;
	struct STR_SUBSECTION *subsect;
	c = *expr++;
	init_value(value);
	if (c <= 6) {
		/* �萔 */
		i = 0;
		if (c & 0x01)
			i--;
		value->min = get_id(c >> 1, &expr, i);
		goto fin;
	}
	if (c == 0x07) {
		/* �萔1 */
		value->min = 1;
		goto fin;
	}
	if (c <= 0x0b) {
		/* ���x�����p */
		struct STR_LABEL *label;
		label = &label0[i = get_id(c - 8, &expr, 0)];
		if ((label->value.flags & VFLG_ENABLE) == 0) {
			if (label->value.flags & VFLG_CALC) {
				value->flags |= VFLG_SLFREF;
				goto fin;
			}
			enable_label(label);
		}
		*value = label->value;
		goto fin;
	}
	#if (DEBUG)
		if (c < 0x10)
			goto dberr;
	#endif
	if (c < 0x20) {
		/* ���Z�q */
		calc_value0(value, &expr);
		if (c == 0x10)
			goto fin; /* �P�����Z�q + */
		if (c == 0x11) {
			/* �P�����Z�q - */
			value->min = - (int) (value->min + value->delta);
	minus2:
			value->scale[0] *= -1;
 			value->scale[1] *= -1;
			for (i = 0; i < MAXSIGMAS; i++)
				value->sigma[i].scale *= -1;
			goto fin;
		}
		if (c == 0x12) {
			/* �P�����Z�q ~ */
			/* ~a == - a - 1 */
			value->min = - (int) (value->min + value->delta) - 1;
			goto minus2;
		}
		calc_value0(&tmp, &expr);
		value->flags |= tmp.flags;
		if (c == 0x13) {
			/* �񍀉��Z�q + */
add2:
			for (i = 0; i < 2; i++) {
				if (value->scale[i] == 0)
					break;
			}
			for (j = 0; j < 2; j++) {
				if (tmp.scale[j] == 0)
					break;
				for (k = 0; k < i; k++) {
					if (value->label[k] == tmp.label[j]) {
						/* ���ލ����o */
						if ((value->scale[k] += tmp.scale[j]) == 0) {
							/* ���̏��� */
							i--;
							value->label[k] = 0xffffffff;
							if (k)
								goto next_product;
							value->scale[0] = value->scale[1];
							value->label[0] = value->label[1];
							value->scale[1] = 0;
							value->label[1] = 0xffffffff;
						}
						goto next_product;
					}
				}
				/* ���ލ������������̂ŁA���̒ǉ� */
				if (i >= 2) {
					value->flags |= VFLG_ERROR;
					goto fin;
				}
				value->scale[i] = tmp.scale[j];
				value->label[i] = tmp.label[j];
				i++;
next_product:
				;
			}
			for (j = 0; j < MAXSIGMAS; j++) {
				if (tmp.sigma[j].scale == 0)
					break;
				addsigma(value, tmp.sigma[j]);
			}
			value->min += tmp.min;
			value->delta += tmp.delta;
			goto fin;
		}
		if (c == 0x14) {
			/* �񍀉��Z�q - */
			tmp.min = - (int) (tmp.min + tmp.delta);
			tmp.scale[0] *= -1;
 			tmp.scale[1] *= -1;
			for (i = 0; i < MAXSIGMAS; i++)
				tmp.sigma[i].scale *= -1;
			goto add2;
		}
		if (c == 0x15) { /* "*" */
			if (value->scale[0])
				goto no_exchange;
			if (tmp.scale[0])
				goto exchange;
		}
		if (c == 0x15 || 0x1d <= c && c <= 0x1f) { /* "*", "&", "|", "^" */
			if (value->delta)
				goto no_exchange;
			if (tmp.delta)
				goto exchange;
			if (value->sigma[0].scale)
				goto no_exchange;
			if (tmp.sigma[0].scale) {
		exchange:
				/* ���̌��� */
				/* �s��l��1���ڂɎ����Ă��邽�߂Ɍ��� */
				tmp.flags = value->flags;
				tmp2 = *value;
				*value = tmp;
				tmp = tmp2;
			}
		}
	no_exchange:
		calcsigma(&tmp);
		if (tmp.delta) {
			/* �s��l���g�����߂ɁA�l���܂�����ł��Ȃ� */
			value->flags |= VFLG_ERROR;
			goto fin;
		}
		if (c == 0x15) {
			/* �񍀉��Z�q * (signed) */
			if (tmp.scale[0]) {
	error:
				/* �����Ƃ�label���܂�ł���΃G���[ */
				value->flags |= VFLG_ERROR;
				goto fin;
			}
multiply2:
			if (tmp.min == 0) {
				init_value(value);
				goto fin;
			}
			value->scale[0] *= tmp.min;
			value->scale[1] *= tmp.min;
			for (i = 0; i < MAXSIGMAS; i++)
				value->sigma[i].scale *= tmp.min;
			i = (signed int) value->min * (signed int) tmp.min;
			j = (signed int) (value->min + value->delta) * (signed int) tmp.min;
minmax:
			value->min = i;
			value->delta = j - i;
			if (i > j) {
				value->min = j;
				value->delta = i - j;
			}
			goto fin;
		}
		/* ����ȍ~��tmp����label�̓L�����Z������� */
		if (c == 0x16) {
			/* �񍀉��Z�q << */
			tmp.min = 1 << tmp.min;
			goto multiply2;
		}
		if (c == 0x17) {
			/* �񍀉��Z�q / (unsigned) */
divu2:
			if (tmp.min == 0)
				goto error;
divu_ij:
			i = (unsigned int) value->min / (unsigned int) tmp.min;
			j = (unsigned int) (value->min + value->delta) / (unsigned int) tmp.min;
			for (k = 0; k < MAXSIGMAS; k++) {
				if (value->sigma[k].scale % tmp.min) {
					calcsigma(value);
					goto divu_ij;
				}
			}
divall:
			if (value->scale[0] % tmp.min)
				goto error;
			if (value->scale[1] % tmp.min)
				goto error;
			value->scale[0] /= tmp.min;
			value->scale[1] /= tmp.min;
			for (k = 0; k < MAXSIGMAS; k++)
				value->sigma[k].scale /= tmp.min;
			goto minmax;
		}
		if (c == 0x19) {
			/* �񍀉��Z�q / (signed) */
divs2:
			if (tmp.min == 0)
				goto error;
divs_ij:
			i = (signed int) value->min / (signed int) tmp.min;
			j = (signed int) (value->min + value->delta) / (signed int) tmp.min;
			for (k = 0; k < MAXSIGMAS; k++) {
				if (value->sigma[k].scale % tmp.min) {
					calcsigma(value);
					goto divs_ij;
				}
			}
			goto divall;
		}
		if (c == 0x1b) {
			/* �񍀉��Z�q >> (unsigned) */
			tmp.min = 1 << tmp.min;
			goto divu2;
		}
		if (c == 0x1c) {
			/* �񍀉��Z�q >> (signed) */
			tmp.min = 1 << tmp.min;
			goto divs2;
		}
		value->scale[0] = value->scale[1] = 0; /* �����L�����Z�� */
		calcsigma(value);
		if (c == 0x18) {
			/* �񍀉��Z�q % (unsigned) */
			if (tmp.min == 0)
				goto error;
			value->min = (unsigned int) value->min % (unsigned int) tmp.min;
			if (value->delta) {
				value->min = 0;
				value->delta = tmp.min - 1;
			}
			goto fin;
		}
		if (c == 0x1a) {
			/* �񍀉��Z�q % (signed) */
			if (tmp.min == 0)
				goto error;
			value->min = (signed int) value->min % (signed int) tmp.min;
			if (value->delta) {
				value->delta = tmp.min - 1;
				if (value->min >= 0) {
					value->min = 0;
				} else if ((signed int) (value->min + value->delta) <= 0) {
					value->min = 1 - tmp.min;
				} else {
					value->min = 1 - tmp.min;
					value->delta *= 2;
				}
			}
			goto fin;
		}
		if (c == 0x1d) {
			/* �񍀉��Z�q & (unsigned) */
			if (tmp.min == -1)
				goto fin; /* �f�ʂ� */
			if (value->delta == 0) {
				value->min &= tmp.min;
				goto fin;
			}
			if (tmp.min & 0x80000000)
				goto error; /* �肪�t�����Ȃ� */
			/* �{���͂����ƍׂ����ł��邪�A�ʓ|�Ȃ̂ł���ĂȂ� */
			value->min = 0;
			value->delta = tmp.min;
			goto fin;
		}
		if (c == 0x1e) {
			/* �񍀉��Z�q | (unsigned) */
			if (tmp.min == 0)
				goto fin; /* �f�ʂ� */
			if (value->delta == 0) {
				value->min |= tmp.min;
				goto fin;
			}
			if (tmp.min & 0x80000000)
				goto error; /* �肪�t�����Ȃ� */
			value->delta += tmp.min;
			if ((value->delta & 0x80000000) == 0)
				goto fin;
			value->delta = 0;
			goto error;
		}
	//	if (c == 0x1f) {
			/* �񍀉��Z�q ^ (unsigned) */
			if (tmp.min == 0)
				goto fin; /* �f�ʂ� */
			if (value->delta == 0) {
				value->min ^= tmp.min;
				goto fin;
			}
			if (tmp.min & 0x80000000)
				goto error; /* �肪�t�����Ȃ� */
			if ((signed int) value->min > (signed int) 0 && value->min >= tmp.min) {
				value->min -= tmp.min;
				value->delta += tmp.min;
				goto fin;
			}
			goto error; /* �肪�t�����Ȃ� */
	//	}
	}
	#if (DEBUG)
		if (c < 0x80)
			goto dberr;
	#endif
#if 0
	if (c == 0x80) {
		/* subsect size sum */
		/* sum�́Adelta��2GB�Ȃ�G���[�ɂ��Ă��܂�(�ʓ|�Ȃ̂�) */
		calc_value(&tmp, &expr);
		i = tmp.min;
		calc_value(&tmp, &expr);
		subsect = subsect0 + tmp.min;
		do {
			value->min += subsect->min;
			value->delta += subsect->delta;
			if (subsect->delta >= INVALID_DELTA)
				value->flags |= VFLG_ERROR;
		} while (--i);
		goto fin;
	}
#endif
	#if (DEBUG)
dberr:
		fprintf(stderr, "calc_value:%02x\n", c);
		GOL_sysabort(GO_TERM_BUGTRAP);
	#endif

fin:
	*pexpr = expr;
	return;
}

static UCHAR *skip_expr(UCHAR *expr)
{
	return LL_skip_expr(expr);
}





/* ���x���̒�`���@:
	��ʎ�
	80 variable-sum, 0000bbaa(aa:����-1, bb:�ŏ��̔ԍ�), 
	84�`87 sum, i - 1, expr, expr, ...

  �E80�`8f:LL�����������p�Ɏg��
	80�`83:variable�Q��(1�`4�o�C�g)
	88�`8f:sum(variable), (1�`4, 1�`4) : �ŏ��͍���-1, ���͍ŏ��̔ԍ�
		{ "|>", 12, 18 }, { "&>", 12, 17 },
		{ "<<", 12, 16 }, { ">>", 12, 17 },
		{ "//", 14,  9 }, { "%%", 14, 10 },
		{ "+",  13,  4 }, { "-",  13,  5 },
		{ "*",  14,  6 }, { "/",  14,  7 },
		{ "%",  14,  8 }, { "^",   7, 14 },
		{ "&",   8, 12 }, { "|",   6, 13 },
		{ "",    0,  0 }

	s+
	s-
	s~

	+, -, *, <<, /u, %u, /s, %s
	>>u, >>s, &, |, ^

*/






#if 0

	< ���@ >

�ŏ��̓w�b�_�B
�E�w�b�_�T�C�Y(DW) = 12
�E�o�[�W�����R�[�h(DW)
�E���x������(DW)


  �E38:���̒l��DB�ɂ��Đݒu
  �E39:���̒l��DW�ɂ��Đݒu
  �E3a:���̒l��3�o�C�g�Őݒu
  �E3b:���̒l��DD�ɂ��Đݒu
  �ȉ��A�E3f�܂ł���B
  �E40�`47:�u���b�N�R�}���h�Bif���Ȃǂ̌㑱�����u���b�N������(2�`9)�B
  �E48:�o�C�g�u���b�N�i�u���b�N�����o�C�g�Ō㑱�j�B
  �E49:���[�h�u���b�N�B
  �E4a:�o�C�g�u���b�N�B
  �E4b:�_�u�����[�h�u���b�N�B
  �E4c:�r���Iif�J�n�B
  �E4d:�I��Iif�J�n�B
  �E4e:�I��I�o�E���_���[if�J�n�B�ϐ��ݒ�̒���A�o�E���_���[�l�������B
  �E4f:endif�B
  �r���Iif�́Aendif������܂ł��������ׂ���Bendif������܂ŁA
  �S��else-if�Ƃ��Ĉ�����B�Ō��else����肽����΁A������萔1�ɂ���B
  �E�^�[�~�l�[�^�[�̓��x����`��0xffffffff�B

  �E58:ORG

  �E60:�A���C���B�o�C�g�̖��ߕ��͌ʂɐݒ肷��B�E�E�E����͔r���Iif�ł��L�q�ł���B

  �E70�`77:�ϒ��o�C�g�錾(���@��ł�40�`4b���㑱���邱�Ƃ��������A�T�|�[�g���Ă��Ȃ��B�������̂�30�`3f)
  �E78�`7f:�ϒ��o�C�g�Q��

  �E80�`8f:LL�����������p�Ɏg��
	80�`83:variable�Q��(1�`4�o�C�g)
	88�`8f:sum(variable), (1�`4, 1�`4) : �ŏ��͍���-1, ���͍ŏ��̔ԍ�


  if�����ł́A�ϒ��o�C�g�錾�����ł��Ȃ��B

	ORG�ɂ��āB�{�������͎��ł����Ă悢���A���̃o�[�W�����ł͒萔�������肵�Ă���B




#endif

/* ibuf, obuf�̊�b�\�� */
/* �V�O�l�`���[8�o�C�g, ����4�o�C�g, ���U�[�u4�o�C�g */
/* ���C���f�[�^�����O�X4B, ���C���f�[�^�X�^�[�g4B */

/* ������������₱�������Ƃ͂��Ȃ� */
/* �X�L�b�v�R�}���h������đΏ����� */

static UCHAR table98typlen[] = { 0x38, 0x38, 0x39, 0x39, 0x3b, 0x3b, 0x38 };
static UCHAR table98range [] = { 0x00, 0x02, 0x00, 0x03, 0x00, 0x03, 0x03 };

struct STR_LL_VB {
	UCHAR *expr, typlen, range;
};

UCHAR *LL_define_VB(struct STR_LL_VB *virtualbyte, UCHAR *s)
{
	UCHAR c;
	virtualbyte->typlen = c = *s++;
	#if (DEBUG)
		if (c < 0x30)
			goto dberr0;
	#endif
	if (c < 0x38) {
		virtualbyte->expr = s;
		s += c & 0x07;
		goto fin;
	}
	if (c < 0x3c)
		virtualbyte->range = *s++;
	else {
		#if (DEBUG)
			if (c < 0x98)
				goto dberr0;
			if (c >= 0x9f)
				goto dberr0;
		#endif
		virtualbyte->typlen = table98typlen[c & 0x07];
		virtualbyte->range  = table98range [c & 0x07];
	}
	virtualbyte->expr = s;
	s = skip_expr(s);
fin:
	return s;	

	#if (DEBUG)
dberr0:
		fprintf(stderr, "LL_define_VB:%02x\n", c);
		GOL_sysabort(GO_TERM_BUGTRAP);
		return NULL;
	#endif
}

UCHAR *lccbug_LL_mc90_func(UCHAR *src0, struct STR_LL_VB *vbb, struct STR_LL_VB *vbc, char len, char c)
{
	do {
		if (c != 0) {
			src0 = skip_mc30(src0, NULL, 1);
			if (vbc)
				src0 = skip_mc30(src0, NULL, 1);
		} else {
			src0 = LL_define_VB(vbb, src0);
			if (vbc)
				src0 = LL_define_VB(vbc, src0);
		}
		c--;
	} while (--len);
	return src0;
}

UCHAR *LL(UCHAR *src0, UCHAR *src1, UCHAR *dest0, UCHAR *dest1)
/* �Ȃ�src�ɏ������݂�����̂ŗv���� */
/* �Vdest1��Ԃ� */
{
	struct STR_SUBSECTION *subsect1, *subsect;
	unsigned int l0, min, max, unsolved, unsolved0;
	struct STR_LABEL *label;
	struct STR_VALUE *value, *labelvalue;
	struct STR_LL_VB virtualbyte[8], *vba, *vbb, *vbc;
	signed int times_count;
	UCHAR *s, *times_src0, *times_dest0, c, len, range;

	label0 = malloc(nask_maxlabels * sizeof (struct STR_LABEL));
	subsect0 = malloc(nask_maxlabels /* == MAXSUBSETCS */ * sizeof (struct STR_SUBSECTION));
	value = malloc(sizeof (struct STR_VALUE));
	labelvalue = malloc(sizeof (struct STR_VALUE));

	/* ���x������o�^���� */
//	l0 = 0; /* ���݂̃A�h���X��l0����lc�̘a */
//	lc = 0;
//	min = max = 0; /* ���ɐ�������̂́Avariable0[l0 + lc] */

	label = label0;
	for (unsolved = nask_maxlabels; unsolved > 0; label++, unsolved--) {
		label->value.flags = 0; /* enable���������� */
		label->define = NULL;
	}

	/* �؂蕪���� */
	/* ���x�������o���� */
	subsect = subsect0;
	subsect->sect0 = src0;
	init_value(labelvalue); /* ���x���ێ� */
	while (src0 < src1) {
		c = *src0;
		if (c == 0x2d) {
			/* EQU */
			src0++;
			calc_value(value, &src0);
			label = &label0[value->min];
			label->define = src0;
			src0 = skip_expr(src0);
			continue;
		}
		if (c == 0x0e) {
			/* ���x����` */
			struct STR_SIGMA sigma;
			sigma.scale = 1;
			sigma.subsect = subsect - subsect0;
			sigma.terms = 1;
			subsect->sect1 = src0;
			subsect++;
			subsect->sect0 = src0;
			src0++;
			calc_value(value, &src0);
			label = &label0[value->min];
			label->define = src0;
			addsigma(labelvalue, sigma);
			label->value = *labelvalue;
			label->value.flags |= VFLG_ENABLE;
			continue;
		}
		if (c == 0x2c) {
			/* extern���x���錾 */
			src0++;
			calc_value(value, &src0);
			label = &label0[value->min];
			init_value(&label->value);
		//	label->define = NULL;
			label->value.flags |= VFLG_EXTERN | VFLG_ENABLE;
			label->value.label[0] = value->min;
			label->value.scale[0] = 1;
			continue;
		}
		if (c == 0x58) {
			/* ORG */
			subsect->sect1 = src0;
			subsect++;
			subsect->sect0 = src0;
			src0++;
			calc_value(labelvalue, &src0);
			continue;
		}
		src0 = LL_skipcode(src0);
	}
	subsect->sect1 = src1;
	subsect1 = subsect + 1;

	for (subsect = subsect0; subsect < subsect1; subsect++) {
		subsect->min = 0;
		subsect->delta = INVALID_DELTA;
		subsect->unsolved = 1;
	}
	len = 0;
	c = 0;
	unsolved0 = 0x7fffffff;
	do {
		for (subsect = subsect0; subsect < subsect1; subsect++) {
			if (subsect->unsolved)
				solve_subsection(subsect, len);
		}
		unsolved = 0;
		for (subsect = subsect1 - 1; subsect >= subsect0; subsect--) {
			if (subsect->unsolved)
				unsolved += solve_subsection(subsect, len);
		}
		c++;
		if (unsolved0 > unsolved) {
			c = 0;
			unsolved0 = unsolved;
		}
		if (c >= 3) {
			len++;
			c = 0;
		}
	} while (unsolved);

	for (subsect = subsect0; subsect < subsect1; subsect++) {
		times_count = 0;
		for (src0 = subsect->sect0; src0 < subsect->sect1; ) {
			c = *src0++;
			if (dest0 + 64 >= dest1) {
	error:
				return NULL; /* buffer overrun */
			}
			if (c == 0x2d) {
				/* EQU */
				calc_value(value, &src0);
				label = &label0[value->min];
				if ((label->value.flags & VFLG_ENABLE) == 0)
					enable_label(label);
				*value = label->value;
				calcsigma(value);
				c = 0x0c;
				if (value->scale[0])
					c++;
				*dest0++ = c;
				len = 4;
				src0 = skip_expr(src0);
				goto putvalue0;
			}
			if (c == 0x0e) {
				src0 = skip_expr(src0);
				continue;
			}
			if (c == 0x0f) {
				dest0[0] = 0xf6; /* REM_8B */
				dest0[1] = *src0++;
				c = *src0++;
				label = &label0[get_id(c - 8, &src0, 0)];
				if ((label->value.flags & VFLG_ENABLE) == 0)
					enable_label(label);
				*value = label->value;
				calcsigma(value);
				dest0[2] = 0; /* base extern label (none) */
				dest0[3] = 0;
				dest0[4] = value->min & 0xff;
				dest0[5] = (value->min >>  8) & 0xff;
				dest0[6] = (value->min >> 16) & 0xff;
				dest0[7] = (value->min >> 24) & 0xff;
				dest0 += 8;
				if (value->scale[0]) {
					dest0[2 - 8] = value->label[0] & 0xff;
					dest0[3 - 8] = (value->label[0] >> 8) & 0xff;
				//	if (value->scale[0] != 1 || value->scale[1] != 0) {
				//		/* �G���[���o�� */
				//	}
				}
				continue;
			}
			if (c == 0x2c) {
				dest0[0] = 0x2c;
				dest0[1] = src0[0];
				dest0[2] = src0[1];
				dest0[3] = src0[2];
				src0 += 3;
				dest0 += 4;
				continue;
			}
			#if (DEBUG)
				if (c < 0x30)
					goto dberr0;
			#endif
			if (c < 0x37) {
				/* 30�`37:�V���[�gDB(�萔�A0�`7�o�C�g) */
				len = c & 0x07;
	copy:
				*dest0++ = c;
				if (len) {
					times_count -= len;
					do {
						*dest0++ = *src0++;
					} while (--len);
	check_times_count:
					if (times_count == 0) {
	times_expand:
						s = skip_expr(times_src0 + 4); /* len */
						calc_value(value, &s); /* �J��Ԃ��� */
						c = 0;
						value->min = times_src0[0] | times_src0[1] << 8
							| times_src0[2] << 16 | times_src0[3] << 24;
						/* ��force�ȂǂŐH���Ⴂ�������Ȃ��悤�ɂ��邽�� */
						if (value->delta) {
							/* �s��l�G���[ */
							c = 1;
							*dest0++ = 0xe9;
						}
						if (value->flags & VFLG_UNDEF) {
							/* ����`�G���[ */
							c = 1;
							*dest0++ = 0xec;
						}
						if (value->flags & VFLG_SLFREF) {
							/* ���ȎQ�ƃG���[ */
							c = 1;
							*dest0++ = 0xea;
						}
						if (value->flags & VFLG_ERROR) {
							/* ���x���l��ʃG���[ */
							c = 1;
							*dest0++ = 0xeb;
						}
						if (c != 0 || (unsigned int) value->min >= 0xfffffff0) {
							*dest0++ = 0xe6; /* TIMES error */
							dest0 = times_dest0; /* �J��Ԃ���0�Ƃ݂Ȃ� */
							continue;
						}
						value->delta = dest0 - times_dest0;
						if (dest0 + value->delta * value->min + 64 >= dest1)
							goto error; /* 1�񕪗]�v�ɔ�r���Ă��邪�A�܁A������ */
						for (dest0 = times_src0 = times_dest0; value->min > 0; value->min--) {
							s = times_src0;
							for (times_count = value->delta; times_count > 0; times_count--)
								*dest0++ = *s++;
						}
					}
	times_count0:
					times_count = 0;
				}
				continue;
			}
			if (c < 0x3c) {
				/* ���̒l��u�� */
				len = c - 0x37;
				range = *src0++;
				calc_value(value, &src0);
	putvalue:
				if (value->scale[0] && len == 4 && value->delta == 0
					&& (value->flags & (VFLG_UNDEF | VFLG_SLFREF | VFLG_ERROR)) == 0) {
					/* �v�����P�[�V����(1��-1) */
					if (value->delta == 0 && value->flags)
					if (value->scale[0] == -1) {
						value->label[0] = value->label[1];
						value->scale[1] = -1;
					}
					/* �����őz�肷�ׂ��P�[�X */
					/* scale[0] == 1, scale[1] ==  0 : ��Ε␳ */
					/* scale[0] == 1, scale[1] == -1 : ���Ε␳ */
					/* 0x2e : ��Ε␳�v��, 0x2f : ���Ε␳�v�� */
					dest0[0] = 0x2e - value->scale[1];
					dest0[1] = 0x09;
					dest0[2] = value->label[0] & 0xff;
					dest0[3] = (value->label[0] >> 8) & 0xff;
					dest0 += 4;
				}
				*dest0++ = 0x30 | len; /* 31�`34 */
				/* �����W�`�F�b�N�Ȃǂ���� */
				/* scale�`�F�b�N������āA�K�v�Ȃ烊���N�ɕK�v�ȏ������� */
	putvalue0:
				/* len�o�C�g��u�� */
				dest0[0] = value->min         & 0xff;
				dest0[1] = (value->min >>  8) & 0xff;
				dest0[2] = (value->min >> 16) & 0xff;
				dest0[3] = (value->min >> 24) & 0xff;
				dest0 += len;
				if (value->delta) {
					/* �s��l�G���[ */
					*dest0++ = 0xe9;
				}
				if (value->flags & VFLG_UNDEF) {
					/* ����`�G���[ */
					*dest0++ = 0xec;
				}
				if (value->flags & VFLG_SLFREF) {
					/* ���ȎQ�ƃG���[ */
					*dest0++ = 0xea;
				}
				if (value->flags & VFLG_ERROR) {
					/* ���x���l��ʃG���[ */
					*dest0++ = 0xeb;
				}
				if ((times_count -= len) == 0)
					goto times_expand;
				goto times_count0;
			//	times_count = 0;
			//	continue;
			}
			if (c == 0x58) {
				/* ORG */
				calc_value(value, &src0);
				*dest0++ = 0x5a;
				len = 4;
				goto putvalue0;
			}
			if (c == 0x59) {
				/* TIMES */
				times_dest0 = dest0;
				times_src0 = src0;
				src0 += 4;
				calc_value(value, &src0); /* len */
				/* len�̕����ɃG���[�͂Ȃ��Ƃ������ƂɂȂ��Ă���(�蔲��) */
				times_count = value->min;
				src0 = skip_expr(src0);
				continue;
			}
			if (c == 0x68) {
				/* endian */
				src0++;
				continue;
			}
			#if (DEBUG)
				if (c < 0x70)
					goto dberr0;
			#endif
			if (c < 0x78) {
				/* 0x70�`0x77:virtual byte��` */
				src0 = LL_define_VB(&virtualbyte[c & 0x07], src0);
				continue;
			}
			if (c < 0x80) {
				/* 0x78�`0x7f:virtual byte�W�J */
				vba = &virtualbyte[c & 0x07];
				s = vba->expr;
				if (vba->typlen < 0x38) {
					len = vba->typlen & 0x07;
					*dest0++ = vba->typlen;
					if (len) {
						times_count -= len;
						do {
							*dest0++ = *s++;
						} while (--len);
						goto check_times_count;
					}
					continue;
				}
				len = vba->typlen - 0x37;
				range = vba->range;
				calc_value(value, &s);
				goto putvalue;
			}
			#if (DEBUG)
				if (c < 0x90)
					goto dberr0;
			#endif
			if (c <= 0x91) {
				/* microcode90 */
				/* microcode91 */
				len = src0[2]; /* z-bbb-d-aaa */
				vba = &virtualbyte[len & 0x07];
				vbb = &virtualbyte[(len >> 4) & 0x07];
				vbc = vbb + 1;
				if (c == 0x90)
					vbc = NULL;
				vba->typlen = 0x37 + (src0[1] & 0x0f);
				vba->range = 0x03; /* non-overflow */
				if (vba->typlen == 1)
					vba->range = 0x02; /* normal-signed */
				c = src0[0] & 0x0f;
				len = ((len /* z-bbb-d-aaa */ >> 7) & 0x01) + 2;
				vba->expr = src0 + 3;
				src0 = skip_expr(vba->expr);

				src0 = lccbug_LL_mc90_func(src0, vbb, vbc, len, c);
#if 0
				do {
					if (c != 0) {
						src0 = skip_mc30(src0, NULL, 1);
						if (vbc)
							src0 = skip_mc30(src0, NULL, 1);
					} else {
						src0 = LL_define_VB(vbb, src0);
						if (vbc)
							src0 = LL_define_VB(vbc, src0);
					}
					c--;
				} while (--len);
#endif
				continue;
			}
			#if (DEBUG)
				if (c < 0x94)
					goto dberr0;
			#endif
			if (c <= 0x95) {
				/* microcode94 */
				/* microcode95 */
				len = src0[2]; /* z-bbb-d-aaa */
				vba = &virtualbyte[len & 0x07];
				vbb = &virtualbyte[(len >> 4) & 0x07];
				vbc = vbb + 1;
				if (c == 0x94)
					vbc = NULL;
				vba->expr = src0 + 3;
				c = src0[0] & 0x0f;
				src0 = skip_expr(vba->expr);
				len = *src0++;
				goto outmc94_skip;
				do {
					c--;
					src0 = skip_expr(src0);
			outmc94_skip:
					if (c == 0) {
						range = *src0++;
						if (range < 0x3c) {
							vba->typlen = range;
							vba->range = *src0++;
						} else {
							vba->typlen = table98typlen[range & 0x07];
							vba->range = table98range[range & 0x07];
						}
						src0 = LL_define_VB(vbb, src0);
					} else {
						src0 = skip_mc30(src0, NULL, 0);
						src0 = skip_mc30(src0, NULL, 1);
					}
					if (vbc) {
						if (c == 0)
							src0 = LL_define_VB(vbc, src0);
						else
							src0 = skip_mc30(src0, NULL, 1);
					}
				} while (--len);
				continue;
			}
			#if (DEBUG)
				if (c < 0x98)
					goto dberr0;
			#endif
			if (c < 0x9f) {
				len = table98typlen[c & 0x07] & 0x07;
				range = table98range[c & 0x07];
				calc_value(value, &src0);
				goto putvalue;
			}
			#if (DEBUG)
				if (c < 0xe0)
					goto dberr0;
			#endif
			if (c < 0xf0) {
				/* 1�o�C�g���}�[�N */
				*dest0++ = c;
				continue;
			}
			if (c < 0xf8) {
				/* n�o�C�g���}�[�N */
				len = c - 0xef;
				goto copy;
			}
			#if (DEBUG)
	dberr0:
				fprintf(stderr, "LL:%02x\n", c);
				GOL_sysabort(GO_TERM_BUGTRAP);
			#endif
		}
	}

	free(label0);
	free(subsect0);
	free(value);
	free(labelvalue);

	return dest0;
}


UCHAR *skip_mc30(UCHAR *s, UCHAR *bytes, char flag)
{
	return LL_skip_mc30(s, bytes, flag);
}

unsigned int solve_subsection(struct STR_SUBSECTION *subsect, char force)
/* unsolved����Ԃ� */
/* subsect->unsolved == 0 �Ȃ�Ă΂Ȃ����� */
{
	struct {
		signed char min, max;
	} vb[8], *vba, *vbb, *vbc;
	unsigned int unsolved = 0, min = 0, delta = 0;
	int i;
	struct STR_VALUE value, tmp;
	UCHAR *s, *t, c, d, e, f, g, h;

	for (s = subsect->sect0; s < subsect->sect1; ) {
		c = *s++;
		if (c == 0x2d) {
			/* EQU���x����` */
			s = skip_expr(s);
			s = skip_expr(s);
			continue;
		}
		if (c == 0x0e) {
			/* ��EQU���x���̒�` */
			/* ���͎蔲���Ȃ̂Ŏ�荇�������� */
			s = skip_expr(s);
			continue;
			/* �{���͂���͗��Ȃ�(�H) */
		}
		if (c == 0x0f) {
			s += s[1] - (0x08 - 3); /* GLOBAL�p�ϊ� */
			continue;
		}
		if (c == 0x2c) {
			s = skip_expr(s);
			continue;
		}
		#if (DEBUG)
			if (c < 0x30)
				goto dberr0;
		#endif
		if (c < 0x3c) {
skipmc30:
			/* 30�`37:�V���[�gDB(�萔�A0�`7�o�C�g) */
			/* ����DB�`DD�Œu�� */
			s = skip_mc30(s - 1, &c, 1);
			min += c;
			continue;
		}
		if (c == 0x58) {
			/* ORG */
			s = skip_expr(s);
			continue;
		}
		if (c == 0x59) {
			/* TIMES */
			t = s;
			i = s[0] | s[1] << 8 | s[2] << 16 | s[3] << 24;
			s += 4;
			calc_value(&value, &s); /* len */
			/* len�ɃG���[�͂Ȃ��Ƃ����O��(�v���O�����J�E���^���܂܂Ȃ��萔��) */
			if ((unsigned int) i < 0xfffffff0) {
				min += (i - 1) * value.min; /* i�͌J��Ԃ��� */
				s = skip_expr(s);
				continue;
			}
			i = value.min;
			calc_value(&value, &s); /* �J��Ԃ��� */
			if (value.flags & (VFLG_SLFREF | VFLG_UNDEF)) /* ���ȎQ�ƃG���[ */
				goto mc59_force;
			if (value.flags & VFLG_ERROR) {
				/* ���x���l��ʃG���[ */
				if (force >= 2)
					goto mc59_force;
				delta = INVALID_DELTA;
				unsolved++;
				continue;
			}
			min += i * (value.min - 1);
			if (value.delta == 0) {
				t[0] = value.min         & 0xff;
				t[1] = (value.min >>  8) & 0xff;
				t[2] = (value.min >> 16) & 0xff;
				t[3] = (value.min >> 24) & 0xff;
				continue;
			}
			if (force < 2) {
				delta += i * value.delta;
				unsolved++;
				continue;
			}
	mc59_force:
			t[0] = 0xfe; /* force */
		//	t[1] = 0xff;
		//	t[2] = 0xff;
		//	t[3] = 0xff;
			continue;
		}
		if (c == 0x68) {
			/* endian */
			s++;
			continue;
		}
		#if (DEBUG)
			if (c < 0x70)
				goto dberr0;
		#endif
		if (c < 0x78) {
			/* 0x70�`0x77:virtual byte��` */
			vba = &vb[c & 0x07];
			s = skip_mc30(s, &vba->min, 1);
			vba->max = vba->min;
			continue;
		}
		if (c < 0x80) {
			/* 0x78�`0x7f:virtual byte�W�J */
			vba = &vb[c & 0x07];
			min += vba->min;
			delta += vba->max - vba->min;
			continue;
		}
		#if (DEBUG)
			if (c < 0x90)
				goto dberr0;
		#endif
		if (c <= 0x91) {
			/* microcode90, microcode91 */
			/* [90 decision z-bbb-d-aaa expr] [3x (d,w)] [3x (b)] ([3x (z)]) */
			/* [91 decision z-bbb-d-aaa expr] [3x 3x (d,w)] [3x 3x (b)] ([3x 3x (z)]) */
			/* ��������vba�� */
			/* decision��2�o�C�g�B�I��ԍ���bbbb-aaaa */
			d = c;
			c = s[2]; /* z-bbb-d-aaa */
			vba = &vb[c        & 0x07];
			vbb = &vb[(c >> 4) & 0x07];
			vbc = vbb + 1;
			if (d == 0x90)
				vbc = NULL;
			if (((e = s[0]) & 0x0f) != 0x0f) {
				/* ���ɉ����ς� */
				d = s[1];
				s = skip_expr(s + 3);
				vba->min = vba->max = d        & 0x0f;
				vbb->min = vbb->max = (d >> 4) & 0x0f;
			//	if (vbc)
			//		vbc->min = vbc->max = (e >> 4) & 0x0f;
				d = 2;
				if (c & 0x80)
					d = 3;
				if (vbc) {
					vbc->min = vbc->max = (e >> 4) & 0x0f;
					d *= 2;
				}
				do {
					s = skip_mc30(s, NULL, 1);
				} while (--d);
				continue;
			}
			t = s;
			s += 3;
			calc_value(&value, &s);
			d = 1 | 2 | 4; /* unknown */
			if ((value.flags & VFLG_ERROR) == 0) {
				i = value.min + value.delta; /* max */
				if (value.scale[0])
					d = 1; /* word(dword) */
				else if (value.min == 0 && i == 0)
					d = 4; /* zero */
				else if (-128 <= value.min && i <= 127) {
					d = 2; /* byte */
					if (value.min <= 0 && 0 <= i)
						d = 2 | 4; /* byte | zero */
				} else {
					d = 1; /* word(dword) */
					if (value.min <= 127 && -128 <= i) {
						d = 1 | 2; /* word | byte */
						if (value.min <= 0 && 0 <= i)
							d = 1 | 2 | 4; /* word | byte | zero */
					}
				}
			}
			if (force) {
				if (d & 1)
					d = 1;
				else
					d = 2;
			}
			if ((c & 0x80) == 0) { /* zero�Ȃ� */
				if ((d &= (1 | 2)) == 0)
					d = 2; /* byte */
			}
			vba->min = vbb->min =  127;
			vba->max = vbb->max = -128;
			if (vbc) {
				vbc->min =  127;
				vbc->max =  -128;
			}
			for (e = 1; e <= 4; e <<= 1) {
				if (e == 4 && (c & 0x80) == 0)
					break;
				s = skip_mc30(s, &f, 1);
				if (vbc)
					s = skip_mc30(s, &g, 1);
				if (d & e) {
					if (vbb->min > f)
						vbb->min = f;
					if (vbb->max < f)
						vbb->max = f;
					if (vbc) {
						if (vbc->min > g)
							vbc->min = g;
						if (vbc->max < g)
							vbc->max = g;
					}
					f = 2;
					if (c & 0x08)
						f = 4;
					if (e == 2)
						f = 1;
					if (e == 4)
						f = 0;
					if (vba->min > f)
						vba->min = f;
					if (vba->max < f)
						vba->max = f;
				}
			}
			if ((d & (d - 1)) == 0) {
				c = 0;
				if (d == 2)
					c = 1;
				if (d == 4)
					c = 2;
				t[0] = c;
				if (vbc)
					t[0] |= vbc->min << 4;
				t[1] = vba->min | vbb->min << 4;
			} else
				unsolved++;
			continue;
		}
		#if (DEBUG)
			if (c < 0x94)
				goto dberr0;
		#endif
		if (c <= 0x95) {
			/* microcode94, microcode95 */
			/* [94 decision 0-bbb-0-aaa expr] [len] [def-a def-b] ([expr def-a def-b] ...) */
			/* [95 decision 0-bbb-0-aaa expr] [len] [def-a def-b def-c] ([expr def-a def-b def-c] ...) */
			/* def-a �� 0x98�`0x9e (�l�͏�����) */
			/* len�͍Œ�1(case�Ȃ�), �ŏ��͖�������`, �㑱�͈�v�������`, ��v�����x������΁A�Ōオ�L�� */
			d = c;
			c = s[2]; /* 0-bbb-0-aaa */
			vba = &vb[c        & 0x07];
			vbb = &vb[(c >> 4) & 0x07];
			vbc = vbb + 1;
			if (d == 0x94)
				vbc = NULL;
			if (((g = s[0]) & 0x0f) != 0x0f) {
				/* ���ɉ����ς� */
				d = s[1];
				s = skip_expr(s + 3);
				e = *s++; /* len */
				vba->min = vba->max = d        & 0x0f;
				vbb->min = vbb->max = (d >> 4) & 0x0f;
				if (vbc)
					vbc->min = vbc->max = (g >> 4) & 0x0f;
				s = skip_mc30(s, NULL, 0);
				s = skip_mc30(s, NULL, 1);
				if (vbc)
					s = skip_mc30(s, NULL, 1);
				while (--e) {
					s = skip_expr(s);
					s = skip_mc30(s, NULL, 0);
					s = skip_mc30(s, NULL, 1);
					if (vbc)
						s = skip_mc30(s, NULL, 1);
				}
				continue;
			}
			t = s;
			s += 3;
			calc_value(&value, &s);
			c = *s++; /* len */
			s = skip_mc30(s, &vba->min, 0);
			s = skip_mc30(s, &vbb->min, 1);
			vba->max = vba->min;
			vbb->max = vbb->min;
			if (vbc) {
				s = skip_mc30(s, &vbc->min, 1);
				vbc->max = vbc->min;
			}
			f = 0;
			if (force == 0) {
				if (value.delta != 0 || (value.flags & VFLG_ERROR) != 0)
					f = 0xff;
				for (g = 1; g < c; g++) {
					/* �ǂ�����ē��������Ƃ����ɂ߂�̂��H */
					/* �ʓ|�Ȃ̂ŁA�S�Ă��m�肷��܂ŁAdecide���Ȃ� */
					calc_value(&tmp, &s);
					if (tmp.delta != 0 || (tmp.flags & VFLG_ERROR) != 0)
						f = 0xff;
					s = skip_mc30(s, &d, 0);
					s = skip_mc30(s, &e, 1);
					if (vbc)
						s = skip_mc30(s, &h, 1);
					if (f != 0xff) {
						if ((value.min ^ tmp.min |
							value.scale[0] ^ tmp.scale[0] | value.label[0] ^ tmp.label[0] |
							value.scale[1] ^ tmp.scale[1] | value.label[1] ^ tmp.label[1]) == 0) {
							vba->min = vba->max = d;
							vbb->min = vbb->max = e;
							if (vbc)
								vbc->min = vbc->max = h;
							f = g;
						}
					} else {
						if (vba->min > d)
							vba->min = d;
						if (vba->max < d)
							vba->max = d;
						if (vbb->min > e)
							vbb->min = e;
						if (vbb->max < e)
							vbb->max = e;
						if (vbc) {
							if (vbb->min > h)
								vbb->min = h;
							if (vbb->max < h)
								vbb->max = h;
						}
					}
				}
			}
			if (f != 0xff) {
				t[0] = f;
				if (vbc)
					t[0] |= vbc->min << 4;
				t[1] = vba->min | vbb->min << 4;
			} else
				unsolved++;
			continue;
		}
		#if (DEBUG)
			if (c < 0x98)
				goto dberr0;
		#endif
		if (c < 0x9f)
			goto skipmc30;
		#if (DEBUG)
			if (c < 0xe0)
				goto dberr0;
		#endif
		if (c < 0xf0)
			continue; /* 1�o�C�g���}�[�N */
		if (c < 0xf8) {
			/* n�o�C�g���}�[�N */
			s += c - 0xef;
			continue;
		}
		#if (DEBUG)
dberr0:
			fprintf(stderr, "solve_subsect:%02X\n", c);
			GOL_sysabort(GO_TERM_BUGTRAP);
		#endif


/*
  [90(91������)] [disp��] [ba] : a��0�`7, b��0�`7 : virtual byte�ԍ� : B=8�͖��g�p���Ӗ�����
  a��bit3�́AD�r�b�g�Ba�ɂ��ẮA��������B�����W�`�F�b�N��03�^�C�v�B
  [90] [disp��] [09] [31, 00-nnn-r/m] [31 01-nnn-r/m] [31 10-nnn-r/m]
  [bits 32]�ł�PUSH�͂����Ȃ�:[91] [imm��] [3a] [31 6a] [31 68]
  94�������悤�Ȍ`���ɕύX(a�̎�����`�͂Ȃ�)�B
  [94] [��] [ba] [case] [def0] [def1] [case] [def0] [def1] ...
  �܂��ȉ����`
  [98] = [38 00 08 00] UCHAR
  [99] = [38 02 08 00] SCHAR
  [9a] = [39 00 08 00] USHORT
  [9b] = [39 03 08 00] non-overflow SHORT
  [9c] = [3b 00 08 00] UINT
  [9d] = [3b 03 08 00] non-overflow INT
  [9e] = [38 03 08 00] non-overflow CHAR




  �E70�`77:�ϒ��o�C�g�錾(���@��ł�40�`4b���㑱���邱�Ƃ��������A�T�|�[�g���Ă��Ȃ��B�������̂�30�`3f)
  �E78�`7f:�ϒ��o�C�g�Q��

  �E80�`8f:LL�����������p�Ɏg��
	80�`83:variable�Q��(1�`4�o�C�g)
	88�`8f:sum(variable), (1�`4, 1�`4) : �ŏ��͍���-1, ���͍ŏ��̔ԍ�

  �E90:0-B-else�o�E���_���[if
  �E91:B-else�o�E���_���[if
  �E92:0BWD�o�E���_���[if
  �E93:else
  �E�I��I�萔if
*/

	}
	if (delta > INVALID_DELTA)
		delta = INVALID_DELTA;
	subsect->min = min;
	subsect->delta = delta;
	subsect->unsolved = unsolved;

	return unsolved;
}

void calcsigma(struct STR_VALUE *value)
/* sigma��S�ēW�J���Ă��܂� */
{
	int i, j, s, t, min;
	unsigned int delta;
	struct STR_SUBSECTION *subsect;
	for (i = 0; i < MAXSIGMAS; i++) {
		if ((s = value->sigma[i].scale) == 0)
		//	continue;
			break;
		value->sigma[i].scale = 0;
		if ((t = value->sigma[i].terms) == 0)
			continue;
		value->sigma[i].terms = 0;
		subsect = &subsect0[value->sigma[i].subsect];
		value->sigma[i].subsect = 0;
		min = delta = 0;
		do {
			min += subsect->min;
			delta += subsect->delta;
			if (delta > INVALID_DELTA)
				delta = INVALID_DELTA;
			subsect++;
		} while (--t);
		if (delta >= INVALID_DELTA) {
			value->delta = INVALID_DELTA;
		}
		if (value->delta >= INVALID_DELTA) {
			value->flags |= VFLG_ERROR;
			continue;
		}
		if (s > 0) {
			value->min += s * min;
			value->delta += s * delta;
		} else {
			value->min += s * (min + delta);
			value->delta -= s * delta;
		}
	}
	return;
}

void addsigma(struct STR_VALUE *value, struct STR_SIGMA sigma)
{
	int i, j;
retry:
	for (i = 0; i < MAXSIGMAS && value->sigma[i].scale != 0; i++) {
		if (sigma.subsect + sigma.terms <= value->sigma[i].subsect)
			break;
		if (value->sigma[i].subsect + value->sigma[i].terms <= sigma.subsect)
			continue;
	add:
		if (value->sigma[i].subsect == sigma.subsect && value->sigma[i].terms <= sigma.terms) {
	add2:
			sigma.terms -= value->sigma[i].terms;
			sigma.subsect += value->sigma[i].terms;
			if ((value->sigma[i].scale += sigma.scale) == 0) {
	delete:
				for (j = i; j <= MAXSIGMAS - 2; j++)
					value->sigma[j] = value->sigma[j + 1];
				value->sigma[MAXSIGMAS - 1].scale = 0;
				value->sigma[MAXSIGMAS - 1].subsect = 0;
				value->sigma[MAXSIGMAS - 1].terms = 0;
			}
	check:
			do {
				if (value->sigma[i].scale != 0) {
					if (i > 0) {
						if (value->sigma[i - 1].scale == value->sigma[i].scale) {
							if (value->sigma[i - 1].subsect + value->sigma[i - 1].terms == value->sigma[i].subsect) {
								value->sigma[i - 1].terms += value->sigma[i].terms;
								goto delete;
							}
						}
					}
				}
			} while (++i < MAXSIGMAS);
			if (sigma.terms > 0)
				goto retry;
			return;
		}
		if (value->sigma[MAXSIGMAS - 1].scale != 0) {
			calcsigma(value);
			goto retry;
		}
		for (j = MAXSIGMAS - 2; i <= j; j--)
			value->sigma[j + 1] = value->sigma[j];
		if (value->sigma[i].subsect == sigma.subsect) {
			value->sigma[i].terms = sigma.terms;
			value->sigma[i + 1].subsect += sigma.terms;
			value->sigma[i + 1].terms -= sigma.terms;
			goto add2;
		}
#if 0
		if (sigma.subsect < value->sigma[i].subsect) {
			value->sigma[i].terms = j = value->sigma[i].subsect - sigma.subsect;
			value->sigma[i].subsect = sigma.subsect;
			value->sigma[i].scale = sigma.scale;
			sigma.terms -= j;
			sigma.subsect += j;
			i++;
			value->sigma[i].subsect = sigma.subsect;
			value->sigma[i].terms -= j;
			goto add;
		}
	//	if (value->sigma[i].subsect < sigma.subsect) {
			value->sigma[i].terms = j = sigma.subsect - value->sigma[i].subsect;
			i++;
			value->sigma[i].subsect = sigma.subsect;
			value->sigma[i].terms -= j;
			goto add;
	//	}
#endif
		if (sigma.subsect < value->sigma[i].subsect) {
			value->sigma[i].terms = j = value->sigma[i].subsect - sigma.subsect;
			value->sigma[i].subsect = sigma.subsect;
			value->sigma[i].scale = sigma.scale;
			sigma.terms -= j;
			sigma.subsect += j;
		} else {
			value->sigma[i].terms = j = sigma.subsect - value->sigma[i].subsect;
		}
		i++;
		value->sigma[i].subsect = sigma.subsect;
		value->sigma[i].terms -= j;
		goto add;
	}
	if (value->sigma[MAXSIGMAS - 1].scale != 0) {
		calcsigma(value);
		goto retry;
	}
	for (j = MAXSIGMAS - 2; i <= j; j--)
		value->sigma[j + 1] = value->sigma[j];
	value->sigma[i] = sigma;
	sigma.terms = 0;
	goto check;
}
