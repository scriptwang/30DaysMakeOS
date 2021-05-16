/* "sartol.c" */
/* copyright (C) 2004 H.Kawai & I.Tak. (KL-01) */

#define SAR_MODE_WIN32		1
/* Linux�Ȃǂł́����R�����g�A�E�g���� */

//#define SAR_MODE_POSIX	1
/* Windows�Ȃǂł́����R�����g�A�E�g���� */

#define SAR_MODE_SJIS		1

#define ERROUT		stdout

typedef unsigned char UCHAR;

/* �t�H�[�}�b�g��sartol0e��0f�ł��������ς���� */

/* �A���C����1�Ȃ�O��14�o�C�g��ǉ����邾���ŁAKHBIOS�Ή��ɂł��� */
/* 16�o�C�g�ǉ��ł����Ȃ�A1/2/4/8/16�̃A���C�������Ȃ� */
/* �����炱���̃A���C���ł́A-���w�肷��K�R���͒Ⴂ���낤 */

struct sar_dirattr0 {
	int flags, unitnamelen, align;
	int baseattr_f, baseperm_f, baseattr_d, baseperm_d;
	int time_unit, subsec_mode;
	int time0[6];
	UCHAR *alignbase;
};

struct sar_attrtime {
	int attr, permission;
	int subsec;
	UCHAR sec, min, hour, day, mon, _dummy[3];
	unsigned int year, year_h;
};

struct sar_fileinfo0 {
	UCHAR *p, *p0, *s;
	int namelen;
	UCHAR *name;
	unsigned int size, size_h;
	UCHAR *content, *content1;
	struct sar_attrtime at;
#if 0
	int attr;
		/* attr�̈Ӗ���sartol0e��0f�ő啝�ɕύX */
		/* bit0-3:0000=normal, 0001=extend, 0010=vfile, 0011=vdir */
		/*		01xx=dir(bit0:0=inline, bit1:1=extend) */
			/* vfile, vdir��xfile, xdir���w���Bxfile, xdir�̓f�t�H���g��hidden�ŁA�����N�J�E���g������ */
			/* �����Ƃ���xfile, xdir�̓��[�g�f�B���N�g���̖����ɒu�� */
			/* xfile��xdir�̓��[�g�f�B���N�g�����̓��ʂȃf�B���N�g���ɂ������ */
			/* ���̃f�B���N�g���ł̓t�@�C���l�[���͂Ȃ��S��ID�inamelen�����ŋL�q�j�ŊǗ������ */
			/* extend�f�B���N�g���͒����w�b�_�������ăf�B���N�g���t�H�[�}�b�g�p�����[�^��ύX�ł���f�B���N�g�� */
		/* bit4:1=deleted */
		/* bit5:1=time�t�B�[���h�Ȃ� */
		/* bit6:1=�p�[�~�b�V�����t�B�[���h���� */
		/* bit7:1=read-only */
		/* bit8:1=hidden */
		/* bit9:1=system */
		/* bit10:archive-flag(DOS-compatible) */
	int permission;
		/* bit0-2:others */
		/* bit3-5:group */
		/* bit6-8:owner */
	int subsec;
	UCHAR sec, min, hour, day, mon, _dummy;
	unsigned int year, year_h;
#endif

	int ext_len;
	int *ext_p;
	struct sar_dirattr0 da0;
};

struct sar_archandle0 {
	int version;
	int lang;
	struct sar_dirattr0 da0;
	int ext_len, ext_id, ext_header_len;
	UCHAR *ext_header;
	UCHAR *p0, *p1;
};

struct sar_archandle1 {
	int version;
	int lang;
	struct sar_dirattr0 da0;
	int ext_len, ext_id, ext_header_len;
	UCHAR *ext_header;
	UCHAR *p, *p1, *s, *s0;
	UCHAR lastdir[4096];
	struct sar_archw_subdir {
		UCHAR *p0; /* �Ƃ肠�����݂��4�o�C�g�m�ۂ��A���Ƃł߂� */
		UCHAR *pl; /* lastdir���̃|�C���^ */
		UCHAR *s0;
		UCHAR *ps; /* �f�B���N�g����size�t�B�[���h�|�C���^ */
	} dir[16];
	int dirlev, reservelen;
};

#if 0

�t�H�[�}�b�g���

�V�O�l�`���F"sar\0KHB0"  73 61 72 00 4b 48 42 30
	�V�O�l�`�����t�@�C���擪�Ɍ�����Ȃ��ꍇ�A����ь㑱�̃t�H�[�}�b�g�o�[�W���������������ꍇ�A
	�t�@�C���擪�̑O�ɗ����ł��Ȃ�14�o�C�g���������Ă��邩������Ȃ��Ɣ��肵��
	�I�t�Z�b�g0x0e����̃V�O�l�`����������邱�ƁB
	���̃��[���ɂ��AKHBIOS�p�f�B�X�N�C���[�W�ɑΉ�����B
	�擪512�A�擪1k�A�擪2k�A�擪4k�A�E�E�E�ȉ�64k�܂ŁA�ł��悢�B16���璲�ׂ邩�B

s7s�Ńt�H�[�}�b�g�o�[�W����(0)
s7s�Ńt���O�t�B�[���h
	���[�g�̃t�@�C�����ΊJ�n�ʒu�ȗ��ibit0��0�Ɂj�B
	�t�@�C�����]���́A�㑱�̃t�@�C������s7s��0x00��A�˂邱�ƂŃJ�o�[�ł���B
	���ꂪ�s���̏ꍇ�́Abit1��1�ɂ��āA�t�@�C�����t�B�[���h���Ɏ��t�@�C������������iextend�j�B
	bit2��1���ƃt�@�C�������t�B�[���h�ł�s7s�̕ς���t8s���g�p�Bt8s�͌��ߑł�����ɂ͈����₷���B
	bit3��1���ƃx�[�X�A�g���r���[�g����
	bit4��1���ƃ^�C���]�[���t�B�[���h����
	bit5��1���ƃx�[�X�����Ȃ�
	bit6��1���ƃx�[�X�A�N�Z�X���Ȃ�
	bit7��1���Ɗg����������B
	bit8��1���Ɗg���w�b�_����B
	bit9��1���ƕ��̎����������B
�����Q�[�W�R�[�h(0:�s��)�B1����IBM�B
�x�[�X�A�g���r���[�g�A7bit+7bit�B
�����P�ʁE�x�[�X�P��
	�����P�ʂ����̏ꍇ�A�b�����ׂ����P�ʂ��w�肷�邱�ƂɂȂ邪�A���̏ꍇ��LSB������ɑ����ɂȂ�A
	LSB��0����10�i�n�ALSB��1����2�i�n�ɂȂ�B
�x�[�X�����E�x�[�X�A�N�Z�X���i���ꂪXOR�����j
	�A�N�Z�X���t�B�[���h�̓r�b�g���t���ɂȂ��Ă���i�܂������A�O���[�v�A���̑��A�g���j�B
	�������������Axwr�̏��ir�����ʁAx����ʁj
�A���C���R�[�h�i���ꂪ�Ȃ��Ă�����ʒu�������΃A���C���͂ł����j�B
�g���������i0���Ɖϒ��j
�g�������R�[�h�i0�͖��\���j
�g���w�b�_��

s7s�Ńw�b�_��

�t�@�C���l�[�����is7s - �A���C���̂��߁j
�t�@�C���l�[��
�t�@�C������
�t�@�C�������i�����ts7s�j
�i�p�[�~�b�V�����j
�t�@�C�����ΊJ�n�ʒu�i�����ts7s�j
�t�@�C���T�C�Y�is7s�j/�X�L�b�v���i�o�C�g���F�z���̌��͐����Ȃ����Ƃɂ����j/��ΊJ�n�ʒu

�T�u�f�B���N�g���ł́A�ŏ��̃t�@�C���̓w�b�_�I�[����̑��΂ŊJ�n�ʒu���L�^�B

�����́A
�b��6bit�A����6bit�A����5bit�A����5bit�A����4bit�A�N�ɂ͂�����ł��B
�b���猎�܂ł�26�r�b�g�B
�����͑S�Đ��E�W�����Ŋi�[�B����31�ɂ���ƁA���������B

sar�͊�{�I�Ƀ��[�h�I�����[�����l���ĂȂ��B���[�h�I�����[�ł����ɒZ���������ł���B

#endif

#if 0

�������S�����������B
������700
�x�[�X�p�X�ƁA�t�@�C�����񋓂Ő����Be��d�B
e�̏ꍇ�Aargv[1]=="e"
	argv[2]==�x�[�X�p�X("."�Ȃ�)
	argv[3]==�i�[�t�@�C����

�t�@�C���l�[����127�܂Łi�t�@�C���l�[����0�ŏI�[�j
�����͂��C�̂Ȃ�0�B
�t�@�C�������͂��C�Ȃ���0�B

�����N�J�E���g�͂Ȃ����A����f�B���N�g���͉\�B
	�� ��{�I��deleted�ŏ����Ă����āA���܂ɐ����Ńf�B���N�g���\���S�̂�ǂݎ���ăf�b�h�����N����������΂������낤�B
		�폜�����Ǘ��������ꍇ�́Adeleted�f�B���N�g��������āA�����ɍD���ȏ������߂�΂�����������Ȃ��B
		�����������ʂȃf�B���N�g���́A���O�̍ŏ���0x00�ɂ���Ƃ��ŕ\�����Ă��������낤
			�i��ʃt�@�C����0x00�Ŏn�܂�Ȃ�A0x00-0x00�ŕ\��������Ƃ��j�B

#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

UCHAR **ini2arg(int *pargc, UCHAR *ini);
int decode(int argc, UCHAR **argv);
int autorun(UCHAR *tmp, UCHAR *cmd, UCHAR *base);
void fixpath(UCHAR *src, UCHAR *dst);
void fixbasepath(UCHAR *base, UCHAR *arc);
int decode_main(int asiz, int bsiz, UCHAR *arc, UCHAR *tmp, UCHAR *basepath, int argc, UCHAR **argv);
int decode_sub(struct sar_fileinfo0 *dir, UCHAR *name, void *prm);
int access_all_r(struct sar_fileinfo0 *dir, UCHAR *name, UCHAR *name1, int argc, UCHAR **argv, int flags,
	int (*func)(struct sar_fileinfo0 *, UCHAR *, void *), void *prm);
int match(UCHAR *name, int argc, UCHAR **argv);
int list0(int argc, UCHAR **argv);
int list_main(int asiz, int bsiz, UCHAR *fbuf, UCHAR *tmp, int argc, UCHAR **argv, int bias, int flags);
int list_sub(struct sar_fileinfo0 *dir, UCHAR *name, void *prm);
int encode(int argc, UCHAR **argv);
int restore(UCHAR **argv);
int fmkdir(UCHAR *path, UCHAR *tmp);
FILE *ffopen(UCHAR *path, UCHAR *tmp);
void getattrtime(struct sar_attrtime *s, UCHAR *path);
void setattrtime(struct sar_attrtime *s, UCHAR *path);

#define SIZ_FILEBUF		32 * 1024 * 1024

void sar_openarchive_r(struct sar_archandle0 *handle, int siz, UCHAR *buf);
unsigned int sar_getnum_s7s(UCHAR **pp);
void sar_opendirectory_r(struct sar_fileinfo0 *dir, struct sar_dirattr0 *da0, UCHAR *p);
void sar_getfile_r(struct sar_fileinfo0 *dir);

UCHAR *sar_puts7s(UCHAR *p, unsigned int i);
void sar_openarchive_w(struct sar_archandle1 *handle, int siz, UCHAR *buf, int flags);
UCHAR *sar_putfile_w(struct sar_fileinfo0 *file, struct sar_archandle1 *arc, int flags);
UCHAR *sar_closedir_w(struct sar_archandle1 *arc, UCHAR *p);
int sar_closearchive_w(struct sar_archandle1 *arc);
int sar_permconv_u2s(int unx);
#define sar_permconv_s2u(x)	sar_permconv_u2s(x)
void sar_time2uc(struct sar_attrtime *at, UCHAR *uc24);
void sar_uc2time(UCHAR *uc24, struct sar_attrtime *at);
UCHAR *sar_puttime(UCHAR *p, struct sar_attrtime *at, struct sar_dirattr0 *da0);
UCHAR *sar_gettime(UCHAR *p, struct sar_attrtime *at, struct sar_dirattr0 *da0);
void sar_shifttime(struct sar_attrtime *at, int min, void *opt);

static struct sar_attrtime sar_atinv = {
	0, 0, -1,
	0, 0, 0, 0, 0x1f, { 0, 0, 0 },
	0, 0
};

extern int autodecomp(int siz0, UCHAR *p0, int siz);

int main(int argc, UCHAR **argv)
{
	int i = 1;
	UCHAR *p;

	if (argc >= 2 && argv[i][0] == '@') {
		argv = ini2arg(&argc, p = &argv[i][1]);
		if (argv == NULL) {
			fprintf(ERROUT, "can't open file : %s\n", p);
			return 1;
		}
		i = 0;
	}

	if (argc - i >= 3 && argv[i][0] == 'd')
		return decode(argc - 1 - i, argv + 1 + i);
	if (argc - i >= 5 && argv[i][0] == 'e')
		return encode(argc - 1 - i, argv + 1 + i);
	if (argc - i >= 2 && argv[i][0] == 'l')
		return list0(argc - 1 - i, argv + 1 + i);
	if (argc - i == 3 && argv[i][0] == 'r')
		return restore(argv + 1 + i);

	fprintf(stdout,
		"usage>sartol e arcfile basepath align file1 file2 ...\n"
		"usage>sartol d arcfile basepath[/] [autorun]\n"
		"usage>sartol l [:bias] arcfile\n"
		"usage>sartol r tek-file outfile\n"
		"usage>sartol @params\n");
	return 1;
}

UCHAR **ini2arg(int *pargc, UCHAR *ini)
{
	FILE *fp;
	int argc = 0, i;
	UCHAR *p = malloc(1024 * 1024), *q, *q0, **argv, mode = 0;

	fp = fopen(ini, "rb");
	if (fp == NULL)
		return NULL;
	i = fread(p, 1, 1024 * 1024 - 1, fp);
	fclose(fp);
	p[i] = '\0';

	q = q0 = malloc(strlen((char *) p) + 1);
	do {
		do {
			if (*p == '\0')
				break; /* �R�}���h���C���o�O����̂��� */
			if (*p == 0x22)
				mode ^= 1;
			else
				*q++ = *p;
			p++;
		} while (*p > ' ' || mode != 0);
		argc++;
		*q++ = '\0';
		while ('\0' < *p && *p <= ' ')
			p++;
	} while (*p);
	argv = malloc((argc + 1) * sizeof (char *));
	argv[0] = q = q0;
	i = 1;
	while (i < argc) {
		while (*q++);
		argv[i++] = q;
	}
	argv[i] = NULL;
	*pargc = argc;
	return argv;
}

int decode(int argc, UCHAR **argv)
{
	struct str_dec0_work {
		UCHAR fbuf[SIZ_FILEBUF], tmp[4096 * 4], basepath[4096], arcname[4096];
	} *work = malloc(sizeof (struct str_dec0_work));

	FILE *fp;
	int i;

	/* '\'��'/'�ɒ����Ĉȍ~�̍�Ƃ����₷������ */
	fixpath(argv[0], work->arcname);
	fixpath(argv[1], work->basepath);

	/* basepath�̉��H�i/�ŏI����Ă���Ƃ��̏����Ȃǁj */
	fixbasepath(work->basepath, work->arcname);

	/* �A�[�J�C�u�ǂݍ��� */
	fp = fopen(argv[0], "rb");
	if (fp == NULL) {
		fprintf(ERROUT, "can't open file : %s\n", argv[0]);
err:
		free(work);
		return 1;
	}
	i = fread(work->fbuf, 1, SIZ_FILEBUF, fp);
	fclose(fp);

	if (decode_main(i, SIZ_FILEBUF, work->fbuf, work->tmp, work->basepath, argc - 3, argv + 3))
		goto err;

	/* autorun */
	i &= 0;
	if (argc >= 3 && !(argv[2][0] == '.' && argv[2][1] == '\0'))
		i = autorun(work->tmp, argv[2], work->basepath);
	return i;
}

struct str_dec_subwork {
	UCHAR *bp0, *tmp;
};

int decode_main(int asiz, int bsiz, UCHAR *arc, UCHAR *tmp, UCHAR *basepath, int argc, UCHAR **argv)
{
	struct str_dec_work {
		struct sar_fileinfo0 dir;
		struct sar_archandle0 arc;
		struct str_dec_subwork sub;
	} *work = malloc(sizeof (struct str_dec_work));

	UCHAR *bp1;
	int i;

	/* tek���k���|�����Ă���΁A������Ƃ� */
	if (autodecomp(bsiz, arc, asiz) < 0)
		goto tekerr;

	/* �ǂݍ��ݐ�p���[�h�ŃA�[�J�C�u���J�� */
	sar_openarchive_r(&work->arc, bsiz, arc);
	if (work->arc.p0 == NULL) {
tekerr:
		fprintf(ERROUT, "arcfile error\n");
		free(work);
		return 1;
	}
	
	/* basepath�̖��������o */
	for (bp1 = basepath; *bp1 != 0; bp1++);
	work->sub.bp0 = basepath;
	work->sub.tmp = tmp;

	sar_opendirectory_r(&work->dir, &work->arc.da0, work->arc.p0);
	i = access_all_r(&work->dir, bp1, bp1, argc, argv, 0x02, decode_sub, &work->sub);
	free(work);
	*bp1 = '\0';
	return i;
}

int decode_sub(struct sar_fileinfo0 *dir, UCHAR *name, void *prm)
{
	struct str_dec_subwork *work = prm;
	int i = dir->at.attr & 0x0f;
	FILE *fp;

	if (0x03 <= i && i <= 0x07) {
		/* �f�B���N�g���n */
		i = fmkdir(work->bp0, work->tmp);
		if (i == 0)
			setattrtime(&dir->at, work->bp0);
		return i;
	}
	if (i) { /* �����m�F�F�܂��ʏ�t�@�C���ȊO�͈����Ȃ� */
		fprintf(ERROUT, "unsupported file type : %s\n", name);
		return 1;
	}

	/* ��ʃt�@�C�� */
	fp = ffopen(work->bp0, work->tmp);
	if (fp == NULL) {
		fprintf(ERROUT, "can't open file : %s\n", work->bp0);
		return 1;
	}
	if (dir->size) {
		if (fwrite(dir->content, 1, dir->size, fp) != dir->size) {
			fclose(fp);
			fprintf(ERROUT, "output error : %s\n", work->bp0);
			return 1;
		}
	}
	fclose(fp);
	setattrtime(&dir->at, work->bp0);
	return 0;
}

int access_all_r(struct sar_fileinfo0 *dir, UCHAR *name, UCHAR *name1, int argc, UCHAR **argv, int flags,
	int (*func)(struct sar_fileinfo0 *, UCHAR *, void *), void *prm)
/* �����𖞂����f�B���N�g�����̑S�Ẵt�@�C���E�f�B���N�g���ɑ΂���func��K�p */
{
	UCHAR *p, *q;
	int i, r = 0;
	do {
		sar_getfile_r(dir);
		if ((i = dir->namelen) == 0)
			break;

		if ((flags & 1) == 0 && (dir->at.attr & 0x10) != 0)
			continue; /* deleted�̓X�L�b�v */

		/* �t�@�C�����̘A�� */
		p = dir->name;
		q = name1;
		do {
			*q++ = *p++;
		} while (--i);
		*q = 0;

		i = dir->at.attr & 0x0f;
		if (i == 0x00) {
			/* �ʏ�t�@�C�� */
			/* �t�@�C�������w�肵�Ă���ꍇ�A��v������̂����邩�ǂ������`�F�b�N���� */
			if (match(name, argc, argv) >= 0)
				r = (*func)(dir, name, prm);
		} else if (i == 0x04) {
			/* �C�����C���f�B���N�g�� */
			q[0] = '/';
			q[1] = '\0';
			/* �t�@�C�������w�肵�Ă���ꍇ�A��v������̂����邩�ǂ������`�F�b�N���� */
			if (match(name, argc, argv) >= 0)
				r = (*func)(dir, name, prm);
			if (flags & 2) {
				struct sar_fileinfo0 sub;
				sub.da0 = dir->da0;
				sub.p = dir->content1;
				sub.s = dir->content;
				sar_getnum_s7s(&sub.p);
				r = access_all_r(&sub, name, q + 1, argc, argv, flags, func, prm);
			}
		}
	} while (r == 0);
	return r;
}

int autorun(UCHAR *tmp, UCHAR *cmd, UCHAR *base)
{
	UCHAR flag = 0, *p, *q, *r;

	*tmp++ = 0x22;
	p = cmd;
	for (q = tmp; *p != '\0'; p++, q++) {
		*q = *p;
		if (*p <= ' ')
			flag = 1;
	}
	*q++ = ' ';
	if (flag) {
		q[-1] = 0x22;
		tmp--;
		*q++ = ' ';
	}
	r = q;
	*q++ = ' ';
	flag = 0;
	p = base;
	for (; *p != '\0'; p++, q++) {
		*q = *p;
		#if (defined(SAR_MODE_WIN32))
			if (*q == '/')
				*q = '\\';
		#endif
		if (*q <= ' ')
			flag = 1;
	}
	*q++ = '\0';
	if (flag) {
		*r = 0x22;
		q[-1] = 0x22;
		*q++ = '\0';
	}

	return system(tmp);

	/* #_���X�y�[�X�u���ɂ���Ό݊����͕ۂĂ� */
	/* #W���K�v�� */
	/* #�����o�����玩��#w�̓I�t�ɂȂ�A�� */
	/* �Ƃ���������Ȃ�#_�͂Ȃ��Ă������ȁE�E�E����K�v�B
		""�͎g�������Ȃ����A�X�y�[�X����ꂽ���ꍇ�͂��肤��B */
}

void fixpath(UCHAR *src, UCHAR *dst)
{
	UCHAR *p, *q;
	p = src;
	for (q = dst; *p != '\0'; *q++ = *p++);
	*q = '\0';

	#if (defined(SAR_MODE_WIN32))
		#if (!defined(SAR_MODE_SJIS))
			for (q = dst; *q != '\0'; q++) {
				if (*q == '\\')
					*q = '/';
			}
		#else
			for (q = dst; *q != '\0'; q++) {
				if (0x81 <= *q && *q <= 0x9f) {
					q++;
					continue;
				}
				if (0xe0 <= *q && *q <= 0xfc) {
					q++;
					continue;
				}
				if (*q == '\\')
					*q = '/';
			}
		#endif
	#endif
	return;
}

void fixbasepath(UCHAR *base, UCHAR *arc)
{
	UCHAR *p, *q, *r, flags = 0;
	int j;
	/* basepath�̌���'/'�̐��𐔂��� */
	for (q = base; *q != '\0'; q++);
	while (base <= &q[-1] && q[-1] == '/') {
		*--q = '\0';
		flags++;
	}

	/* basepath�̖�����"."�ł���΂�����O�� */
	/* "a:/." -> "a:/", "a:." -> "a:" */
	/* �����łȂ����"/"������ */
	if (base <= &q[-1] && q[-1] == '.' &&
		(base == &q[-1] || (base <= &q[-2] && (q[-2] == '/' || q[-2] == ':'))))
		q--;
	else
		*q++ = '/';
	*q = '\0';

	/* "..@arcpath"�̃`�F�b�N */
	if (q - base == 11) {
		UCHAR c = 0;
		for (j = 0; j < 11; j++)
			c |= base[j] ^ "..@arcpath/"[j];
		if (c == 0) {
			p = arc;
			for (r = q = base; *p != '\0'; p++, q++) {
				*q = *p;
				if (*q == '/')
					r = q + 1;
			}
			*r = '\0';
			q = r;
		}
	}

	/* basepath�ɃA�[�J�C�u����t������ */
	if (flags >= 1) {
		for (r = p = arc; *p != '\0'; p++) {
			if (*p == '/')
				r = p + 1;
		}
		for (p = NULL; *r != '\0'; r++, q++) {
			*q = *r;
			if (*q == '.')
				p = q;
		}
		if (flags == 1) {
			if (p)
				q = p;
		}
		*q++ = '/';
		*q = '\0';
	}

	return;
}

int match(UCHAR *name, int argc, UCHAR **argv)
/* 0:���A-1:�s����, 1�ȏ�:��֕\�������݂��� */
{
	int i, j = 0, flags = 0, len = strlen(name);
	UCHAR *p;
	if (argc <= 0)
		return 0;
	for (i = 0; i < argc; i++) {
		p = argv[i];
		if (*p == ':') {
		//	if (p[1] == '=')
		//		continue;
			if (p[1] == '!') {
				flags |= 0x05;
				p += 2;
			} else
				continue;
		} else
			flags |= 0x0a; /* �ʏ���������݂��� */
		if (strncmp(name, p, len) == 0 && p[len] == '\0') {
			if (flags & 1)
				return -1;
			else {
				flags |= 0x10; /* �ʏ��������v���� */
				j = i + 1;
			}
		}
		flags &= ~0x3;
	}
	if (flags == 0)
		return 0; /* �����͈��������all */
	if (flags & 0x10) { /* ��v���聕�ے�ɂ͊Y������ */
		if (j < argc && argv[j][0] == ':' && argv[j][1] == '=')
			return j;
		return 0;
	}
	if (flags == 0x04) /* �ے���������Ȃ����ǂ�ɂ��|����Ȃ����� */
		return 0;
	return -1;
}

int list0(int argc, UCHAR **argv)
{
	struct str_dec0_work {
		UCHAR fbuf[SIZ_FILEBUF], tmp[4096 * 4], arcname[4096];
	} *work = malloc(sizeof (struct str_dec0_work));

	FILE *fp;
	int i, bias = 0;
	UCHAR *p = argv[0];

	if (*p == ':') {
		p++;
		i = 1;
		if (*p == '+')
			p++;
		if (*p == '-') {
			p++;
			i |= -1;
		}
		while ('0' <= *p && *p <= '9')
			bias = bias * 10 + (*p++ - '0');
		bias *= i;
		argc--;
		argv++;
	}

	/* �A�[�J�C�u�ǂݍ��� */
	fp = fopen(argv[0], "rb");
	if (fp == NULL) {
		fprintf(ERROUT, "can't open file : %s\n", argv[0]);
err:
		free(work);
		return 1;
	}
	i = fread(work->fbuf, 1, SIZ_FILEBUF, fp);
	fclose(fp);

	if (list_main(i, SIZ_FILEBUF, work->fbuf, work->tmp, argc - 1, argv + 1, bias * 60, 0x02))
		goto err;

	free(work);
	return 0;
}

int list_main(int asiz, int bsiz, UCHAR *fbuf, UCHAR *tmp, int argc, UCHAR **argv, int bias, int flags)
{
	struct sar_archandle0 arc;
	struct sar_fileinfo0 dir;

	/* tek���k���|�����Ă���΁A������Ƃ� */
	if (autodecomp(bsiz, fbuf, asiz) < 0)
		goto tekerr;

	/* �ǂݍ��ݐ�p���[�h�ŃA�[�J�C�u���J�� */
	sar_openarchive_r(&arc, bsiz, fbuf);
	if (arc.p0 == NULL) {
tekerr:
		fprintf(ERROUT, "arcfile error\n");
		return 1;
	}
	sar_opendirectory_r(&dir, &arc.da0, arc.p0);
	return access_all_r(&dir, tmp + 50, tmp + 50, argc, argv, flags, list_sub, &bias);
}

int list_sub(struct sar_fileinfo0 *dir, UCHAR *name, void *prm)
{
	UCHAR *tmp = name - 50, *q;
	int i, j, bias = *(int *) prm;

	/* ----/--/--  --:--:--.---  RHSAD  666  1234567890  filename/ */
	/* ----/--/--  --:--:--.---  RHSAD  666     <dir>    filename/ */

	for (j = 0; j < 50; j++)
		tmp[j] = "----/--/--  --:--:--.---  _____                   "[j];

	if (dir->at.subsec != -1) {
		/* �����␳ */
		sar_shifttime(&dir->at, bias, NULL);
		sprintf(tmp, "%04d/%02d/%02d  %02d:%02d:%02d.%03d  _____                  ",
			dir->at.year, dir->at.mon, dir->at.day,
			dir->at.hour, dir->at.min, dir->at.sec, dir->at.subsec >> 20
		);
		tmp[49] = ' ';
	}
	tmp[33] = '0' | ((dir->at.permission >> 6) & 7);
	tmp[34] = '0' | ((dir->at.permission >> 3) & 7);
	tmp[35] = '0' | ((dir->at.permission     ) & 7);

	i = dir->at.attr >> 4;
	if (i & 0x08)
		tmp[26] = 'R';
	if (i & 0x10)
		tmp[27] = 'H';
	if (i & 0x20)
		tmp[28] = 'S';
	if (i & 0x40)
		tmp[29] = 'A';
	if (i & 0x01)
		tmp[30] = 'D';

	i = dir->at.attr & 0x0f;
	if (i == 0x00) {
		/* �ʏ�t�@�C�� */
		j = dir->size;
		q = &tmp[47];
		do {
			*q-- = '0' + (j % 10);
		} while (j /= 10);
	} else if (i == 0x04) {
		/* �C�����C���f�B���N�g�� */
		for (j = 0; j < 5; j++)
			tmp[41 + j] = "<dir>"[j];
	}
	puts(tmp); /* �o�� */
	return 0;
}

int encode(int argc, UCHAR **argv)
{
	int align, i, j, k, flags;
	UCHAR *p, *q, *r, *fnb, *fbuf, *fnb0, *tmp;
	FILE *fp;
	struct sar_archandle1 *arc;
	struct sar_fileinfo0 *finfo;

	fnb0 = malloc(4096);
	tmp = malloc(4096 * 4);
	fbuf = malloc(SIZ_FILEBUF);
	arc = malloc(sizeof (struct sar_archandle1));
	finfo = malloc(sizeof (struct sar_fileinfo0));

	flags &= 0;
	align &= 0;
	p = argv[2];
	k &= 0;
prefix:
	if (*p == '!') {
		p++;
		flags |= 2; /* �t�@�C�������A�����i�[ */
		goto prefix;
	}
	if (*p == '@') {
		p++;
		flags |= 1;
		goto prefix;
	}
	if (*p == '-') {
		p++;
		k |= 14;
		goto prefix;
	}
	while ('0' <= *p && *p <= '9')
		align = align * 10 + (*p++ - '0');
	if (*p == 'k' || *p == 'K')
		align *= 1024;
	if ((align & (align - 1)) != 0 || align == 0) {
		fprintf(ERROUT, "align error\n");
		return 1;
	}

	arc->da0.align = align - 1;
	arc->reservelen = k;
	sar_openarchive_w(arc, SIZ_FILEBUF, fbuf, flags);

	fnb = fnb0;
	for (q = argv[1]; *q != 0; *fnb++ = *q++);
	*fnb++ = '/';
	for (i = 3; i < argc; i++) {
		finfo->name = r = fnb;
		for (q = argv[i]; *q != 0; *r++ = *q++);
		*r = '\0';
		finfo->namelen = r - fnb;

		if (r[-1] == '/') {
			if (flags & 2)
				getattrtime(&finfo->at, fnb0);
			sar_putfile_w(finfo, arc, flags);
			continue;
		}
		fp = fopen(fnb0, "rb");
		if (fp == NULL) {
open_err:
			fprintf(ERROUT, "can't open file : %s\n", fnb0);
			return 1;
		}
		r = arc->s;
			/* ���̃��C�u�����̓C�����C���f�B���N�g�������T�|�[�g���ĂȂ��̂ŁAarc->s�͗��p�ł��� */
		j = fread(r, 1, fbuf + SIZ_FILEBUF - r, fp);
			/* fread�݂͂͂����悤�ȗv�����ǂ������킴�킴�`�F�b�N���Ă���炵�� */
		fclose(fp);
		if (flags & 1)
			j = autodecomp(fbuf + SIZ_FILEBUF - r, r, j); /* �G���[�`�F�b�N���ڂ� */

		finfo->size = j;
		r += j;
		if (flags & 2)
			getattrtime(&finfo->at, fnb0);
		sar_putfile_w(finfo, arc, flags); /* finfo�̒��̖��O�ɂ̓f�B���N�g�������܂܂�Ă��Ă悢 */
		while (r < arc->s)
			*r++ = 0x00;
	}

	j = sar_closearchive_w(arc);

	fp = ffopen(argv[0], tmp);
	if (fp == NULL) {
		fnb0 = argv[0];
		goto open_err;
	}
	fwrite(fbuf, 1, j, fp);
	fclose(fp);
	return 0;
}

int restore(UCHAR **argv)
/* ������tek�f�R�[�_ */
{
	UCHAR *buf = malloc(SIZ_FILEBUF);
	FILE *fp;
	int csiz, dsiz;
	if (argv[0][0] == ':' && argv[0][1] == 's' && argv[0][2] == 't' && argv[0][3] == 'd' && argv[0][4] == '\0')
		fp = stdin; /* ���Linux�Ŋ���i�o�C�i�����[�h�ł���K�v������̂Łj */
	else {
		fp = fopen(argv[0], "rb");
		if (fp == NULL) {
			fprintf(ERROUT, "can't open file : %s\n", argv[0]);
err:
			free(buf);
			return 1;
		}
	}
	csiz = fread(buf, 1, SIZ_FILEBUF, fp);
	fclose(fp);
	dsiz = autodecomp(SIZ_FILEBUF, buf, csiz);
	if (dsiz < 0) {
		fprintf(ERROUT, "decode error\n");
		free(buf);
		return 1;
	}
	if (argv[1][0] == ':' && argv[1][1] == 's' && argv[1][2] == 't' && argv[1][3] == 'd' && argv[1][4] == '\0')
		fp = stdout; /* ���Linux�Ŋ���i�o�C�i�����[�h�ł���K�v������̂Łj */
	else {
		fp = fopen(argv[1], "wb");
		if (fp == NULL) {
			fprintf(ERROUT, "can't open file : %s\n", argv[1]);
			goto err;
		}
	}
	csiz = fwrite(buf, 1, dsiz, fp);
	fclose(fp);
	free(buf);
	if (csiz != dsiz) {
		fprintf(ERROUT, "output error : %s\n", argv[1]);
		return 1;
	}
	return 0;
}

/* �t�@�C���֌W�֐��Q */

int chkdir(UCHAR *path, UCHAR *tmp)
{
	UCHAR *t = tmp, *p;
	FILE *fp;
	if (*path == '\0')
		return 0;
	for (; *path != '\0'; *t++ = *path++);
	if (t[-1] != '/')
		*t++ = '/';
	for (p = "_chkdir_.prb"; *p != '\0'; *t++ = *p++);
	*t = '\0';

	fp = fopen(tmp, "rb");
	if (fp != NULL) {
		fclose(fp);
		return 0;
	}
	fp = fopen(tmp, "wb");
	if (fp != NULL) {
		fclose(fp);
		remove(tmp);
		return 0;
	}
	return 1;
}

int makdir(UCHAR *path, UCHAR *tmp)
{
	UCHAR flag = 0;
	UCHAR *p = "mkdir ", *t = tmp;
	int i;
	for (; *p != '\0'; *t++ = *p++);
	if (*path != 0x22) {
		for (p = path; *p != '\0'; p++) {
			if (*p <= ' ')
				flag = 1;
		}
	}
	if (flag)
		*t++ = 0x22;
	for (p = path; *p != '\0'; *t++ = *p++);
	if (flag)
		*t++ = 0x22;
	*t = '\0';
	#if (defined(SAR_MODE_WIN32))
		for (t = tmp; *t != '\0'; t++) {
			if (*t == '/')
				*t = '\\';
		}
	#endif
	i = system(tmp);
	#if (defined(SAR_MODE_WIN32) && 0)
		/* ��������Ȃ��Ă��悭�Ȃ����̂ŃR�����g�A�E�g */
		if (i == 0) {
			/* Windows��mkdir�͂Ȃ�ƃf�B���N�g���̐����Ɏ��s���Ă��G���[�ɂȂ�Ȃ� */
			/* �����炿���Əo�������ǂ������m�F */
			i = chkdir(path, tmp);
		}
	#endif

	return i;
}

int fmkdir(UCHAR *path, UCHAR *tmp)
{
	UCHAR *t, *t1;
	if (chkdir(path, tmp) == 0)
		return 0; /* ���ɑ��݂��� */
	for (t = tmp; *path != '\0'; *t++ = *path++);
	if (t[-1] == '/')
		t--;
	*t = '\0';

	t1 = t;
	#if 0
		/* Win95�ŗ]�v�ȃ��b�Z�[�W���o��̂ł���̓X�L�b�v */
		if (makdir(tmp, t1 + 1) == 0)
			return 0; /* �����ɐ����ł��� */
	#endif
	for (;;) {
		while (t > tmp && *t != '/' && *t != ':')
			t--;
		if (t <= tmp)
			break;
		#if (defined(SAR_MODE_WIN32))
			if (*t == ':')
				goto colon;
		#endif
		*t = '\0';
		if (chkdir(tmp, t1 + 1) == 0)
			break;
	}
	/* ��O�i���� */
	do {
		if (t > tmp)
			*t = '/';
#if (defined(SAR_MODE_WIN32))
colon:
#endif
		do {
			t++;
		} while (*t);
		if (makdir(tmp, t1 + 1))
			return 1; /* �Ȃ����G���[ */
	} while (t < t1);
	return 0; /* �S������ */
}

FILE *ffopen(UCHAR *path, UCHAR *tmp)
/* wb�ł̃I�[�v���Ɍ��� */
{
	UCHAR *t = tmp, *p = NULL, *q, *s;
	FILE *fp;
	s = tmp;
	for (q = path; *q != '\0'; q++, s++) {
		*s = *q;
		#if (defined(SAR_MODE_WIN32))
			if (*s == '/')
				*s = '\\';
		#endif
	}
	*s = '\0';
	fp = fopen(tmp, "wb");
	if (fp)
		return fp;
	for (q = path; *q != '\0'; t++, q++) {
		*t = *q;
		if (*t == '/')
			p = t;
	}
	*t++ = '\0';
	if (p) {
		*p = '\0';
		if (fmkdir(tmp, t) == 0) {
			s = t;
			for (q = path; *q != '\0'; q++, s++) {
				*s = *q;
				#if (defined(SAR_MODE_WIN32))
					if (*s == '/')
						*s = '\\';
				#endif
			}
			*s = '\0';
			fp = fopen(t, "wb");
			if (fp)
				return fp;
		}
	}
	return NULL;
}

#if (defined(SAR_MODE_WIN32))

	#if (!defined(__stdcall))
		#define	__stdcall	__attribute__((__stdcall__))
	#endif

	#define FILE_ATTRIBUTE_READONLY		0x01
	#define FILE_ATTRIBUTE_HIDDEN		0x02
	#define FILE_ATTRIBUTE_SYSTEM		0x04
	#define FILE_ATTRIBUTE_DIRECTORY	0x10
	#define FILE_ATTRIBUTE_ARCHIVE		0x20
	#define FILE_ATTRIBUTE_NORMAL		0x80
	#define FILE_FLAG_BACKUP_SEMANTICS	0x02000000
	#define FILE_SHARE_READ				1
	#define FILE_SHARE_WRITE			2
	#define FILE_SHARE_DELETE			4
	#define GENERIC_READ				0x80000000
	#define GENERIC_WRITE				0x40000000
	#define OPEN_EXISTING				3

	struct FILETIME {
		unsigned int dwLowDateTime, dwHighDateTime;
	};

	struct SYSTEMTIME {
		short wYear, wMonth, wDayOfWeek, wDay;
		short wHour, wMinute, wSecond, wMilliseconds;
	};

	struct TIME_ZONE_INFORMATION {
		int Bias;
		char StandardName[64];
		struct SYSTEMTIME StandardDate;
		int StandardBias;
		char DaylightName[64];
		struct SYSTEMTIME DaylightDate;
		int DaylightBias;
	};

	int __stdcall GetFileAttributesA(const char *lpFileName);
	int __stdcall SetFileAttributesA(const char *lpFileName, int dwFileAttributes);
	int __stdcall GetFileTime(int hFile, struct FILETIME *lpCreationTime,
		struct FILETIME *lpLastAccessTime, struct FILETIME *lpLastWriteTime);
	int __stdcall SetFileTime(int hFile, const struct FILETIME *lpCreationTime,
		const struct FILETIME *lpLastAccessTime, const struct FILETIME *lpLastWriteTime);
	int __stdcall SystemTimeToFileTime(const struct SYSTEMTIME *lpSystemTime, struct FILETIME *lpFileTime);
	int __stdcall FileTimeToSystemTime(const struct FILETIME *lpFileTime, struct SYSTEMTIME *lpSystemTime);
	int __stdcall GetTimeZoneInformation(struct TIME_ZONE_INFORMATION *lpTimeZoneInformation);
	int __stdcall CreateFileA(const char *name, int access, int share, void *security, int option, int flags, int template);
	int __stdcall CloseHandle(int handle);

#elif (defined(SAR_MODE_POSIX))
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <unistd.h>
	#include <utime.h>
	#include <time.h>		/* ANSI-C, time_t�Ɠ����̕ϊ��Ɏg�� */
#endif


void getattrtime(struct sar_attrtime *s, UCHAR *path)
/* s->attr��bit0-3,5-6�͕ۑ����邱�� */
{
	#if (defined(SAR_MODE_WIN32))
		int i;
		s->attr &= 0x06f;
		i = GetFileAttributesA(path);
		if (i & FILE_ATTRIBUTE_READONLY)
			s->attr |= 0x080;
		if (i & FILE_ATTRIBUTE_HIDDEN)
			s->attr |= 0x100;
		if (i & FILE_ATTRIBUTE_SYSTEM)
			s->attr |= 0x200;
		if (i & FILE_ATTRIBUTE_ARCHIVE)
			s->attr |= 0x400;
		s->subsec |= -1;
		i = CreateFileA(path, GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
		if (i != -1) {
			struct FILETIME ft;
			struct SYSTEMTIME st;
			GetFileTime(i, NULL, NULL, &ft);
			CloseHandle(i);
			FileTimeToSystemTime(&ft, &st);
			s->subsec = st.wMilliseconds << 20;
			s->sec = st.wSecond;
			s->min = st.wMinute;
			s->hour = st.wHour;
			s->day = st.wDay;
			s->mon = st.wMonth;
			s->year = st.wYear;
			s->year_h = 0;
		}
		s->permission = 0700;
	#elif (defined(SAR_MODE_POSIX))
		/* POSIX�ɂ��Ă݂�e�X�g */
		struct stat buf;
		s->attr &= 0x6f;
		s->subsec = -1;
		if (stat(path, &buf) == 0) {
			struct tm gm;
			s->permission = buf.st_mode & 0777; /* SUID�Ƃ��ꉞ���� */
			/* ANSI-C */
			if (gmtime_r(&buf.st_mtime, &gm)) {
				s->subsec = 0;
				s->sec  = gm.tm_sec;
				s->min  = gm.tm_min;
				s->hour = gm.tm_hour;
				s->day  = gm.tm_mday;
				s->mon  = gm.tm_mon + 1;
				s->year = gm.tm_year + 1900;
				s->year_h = 0;
			}
		}
	#else
		s->attr &= 0x6f;
		s->permission = 0700;
		s->subsec |= -1; /* subsec == -1 : �X�V�������Ȃ� */
	#endif
	return;
}

void setattrtime(struct sar_attrtime *s, UCHAR *path)
{
	#if (defined(SAR_MODE_WIN32))
		int i;
		i = GetFileAttributesA(path);
		i &= ~FILE_ATTRIBUTE_READONLY & ~FILE_ATTRIBUTE_HIDDEN &
			~FILE_ATTRIBUTE_SYSTEM & ~FILE_ATTRIBUTE_ARCHIVE & ~FILE_ATTRIBUTE_NORMAL;
		if (s->attr & 0x080)
			i |= FILE_ATTRIBUTE_READONLY;
		if (s->attr & 0x100)
			i |= FILE_ATTRIBUTE_HIDDEN;
		if (s->attr & 0x200)
			i |= FILE_ATTRIBUTE_SYSTEM;
		if (s->attr & 0x400)
			i |= FILE_ATTRIBUTE_ARCHIVE;
		SetFileAttributesA(path, i);
		if (s->subsec != -1) {
			struct SYSTEMTIME st;
			struct FILETIME ft;
			int h;
			st.wMilliseconds = s->subsec >> 20;
			st.wSecond = s->sec;
			st.wMinute = s->min;
			st.wHour = s->hour;
			st.wDay = s->day;
			st.wMonth = s->mon;
			st.wYear = s->year;
			SystemTimeToFileTime(&st, &ft);
			h = CreateFileA(path, GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
				NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
			if (h != -1) {
				SetFileTime(h, &ft, &ft, &ft); /* �Ƃ肠�������ׂĂ��̓����ɂ��Ă݂� */
				CloseHandle(h);
			}
		}
	#elif (defined(SAR_MODE_POSIX))
		struct tm gm;
		struct utimbuf buf;
		char *tz;

		gm.tm_sec  = s->sec;
		gm.tm_min  = s->min;
		gm.tm_hour = s->hour;
		gm.tm_mday = s->day;
		gm.tm_mon  = s->mon - 1;
		gm.tm_year = s->year - 1900;

		/* GNU�g��timegm�̑��� */
		tz = getenv("TZ"); /* TZ��ۑ� */
		setenv("TZ", "", 1); /* TZ������ */
		tzset();
		buf.modtime = buf.actime = mktime(&gm);
		if (tz)
			setenv("TZ", tz, 1);
		else
			unsetenv("TZ");
		tzset();	/* TZ���� */

		if (buf.modtime != -1)
			utime(path, &buf);
		chmod(path, s->permission); /* ���s���Ă��C�ɂ��Ȃ� */
	#else
		/* �������Ȃ� */
	#endif
	return;
}

/* sar�T�|�[�g�֐��Q�i�W�J�p�j */

unsigned int sar_getnum_s7s(UCHAR **pp)
/* ����͕K��big-endian */
/* ���ʂ��Ȃ��̂Œ��g��������₷�����Apad������₷���B */
{
	unsigned int s = 0;
	UCHAR *p = *pp;
	do {
		s = s << 7 | *p++;
	} while ((s & 1) == 0);
	s >>= 1;
	*pp = p;
	return s;
}

void sar_openarchive_r(struct sar_archandle0 *handle, int siz, UCHAR *buf)
/* �ǂݍ��ݐ�p���[�h�ŃA�[�J�C�u���J�� */
{
	int i, j;
	UCHAR *p, *q;
	handle->da0.alignbase = buf;
	handle->p1 = buf + siz;
	handle->p0 = NULL;
	handle->da0.baseattr_f = 0;
	handle->da0.baseattr_d = 0;
	handle->da0.baseperm_f = 0x0007;
	handle->da0.baseperm_d = 0x0007; /* �w�b�_�ł͉��ʂ�f�A��ʂ�d�B������d������f�ɑ΂���xor�ŋL�q */
	handle->da0.flags = 0;
	handle->da0.unitnamelen = 0;

	for (i = 0; i < 6; i++)
		handle->da0.time0[i] = 0;

	j = 0;
retry:
	i = 0;
	p = buf + j;
	for (q = "sar\0KHB0\1\1\1\1\x2d\x3f\x0f\2"; *q != 1; i |= *p++ ^ *q++); /* 8�o�C�g */
	if (i) {
		if (j == 0) {
			j += 14;
			goto retry;
		}
		if (j < 16) {
			j = 16;
			goto retry;
		}
		if (j < 64 * 1024) {
			j += j;
			goto retry;
		}
arc_err:
		return;
	}
	if (sar_getnum_s7s(&p) != 0) {
		if (j == 0) {
			j += 14;
			goto retry;
		}
		goto arc_err; /* �t�H�[�}�b�g�o�[�W�����`�F�b�N */
	}
	handle->da0.flags = sar_getnum_s7s(&p);

	if (handle->da0.flags & ~0x0029)
		goto arc_err; /* �t���O�`�F�b�N */

	sar_getnum_s7s(&p); /* �����Q�[�W�͖��� */
	if (handle->da0.flags & 0x0008) {
		/* �x�[�X�A�g���r���[�g */
		i = sar_getnum_s7s(&p);
		handle->da0.baseattr_f = i & 0x7f;
		handle->da0.baseattr_d = (i >> 7) & 0x7f;
	}
	sar_getnum_s7s(&p); /* �����P�ʂ͖��� */
	if ((handle->da0.flags & 0x0020) == 0) {
		/* �x�[�X������΁A����͖��������ł���ƌ��ߕt���� */
		sar_getnum_s7s(&p); /* �x�[�X�P�ʂ͖��� */
		sar_getnum_s7s(&p); /* �x�[�X���������� */
		sar_time2uc(&sar_atinv, (UCHAR *) handle->da0.time0);
	}
	i = sar_getnum_s7s(&p); /* �x�[�X�A�N�Z�X�� */
	handle->da0.baseperm_f = i;
	handle->da0.baseperm_d = i; /* ��ʂ�0�Ȃ炱��ł��� */

	handle->da0.align = (1 << sar_getnum_s7s(&p)) - 1; /* align�̓}�X�N�p�^�[���ɂȂ��Ă���̂ɒ��� */
	handle->p0 = p;
	return;
}

void sar_opendirectory_r(struct sar_fileinfo0 *dir, struct sar_dirattr0 *da0, UCHAR *p)
{
	int i, align = da0->align;
	UCHAR *alignbase = da0->alignbase;

	i = sar_getnum_s7s(&p); /* �f�B���N�g���� */
	dir->p = p;
	dir->s = alignbase + (((p - alignbase) + align) & ~align) + i * (align + 1);
	dir->da0 = *da0;

	return;
}

#if 0

	/* ���܂�ɒP���Ȃ̂Ŋ֐��ɂ���܂ł��Ȃ� */

void sar_openildirectory_r(struct sar_fileinfo0 *dir, struct sar_archandle0 *arc, UCHAR *p, UCHAR *s)
{
	dir->p = p;
	dir->s = s;

	return;
}

#endif

void sar_getfile_r(struct sar_fileinfo0 *dir)
{
	int i;
	UCHAR *p = dir->p;
	dir->p0 = p;
	dir->namelen = sar_getnum_s7s(&p);
	if (dir->namelen == 0)
		return;
	dir->name = p;
	if (dir->da0.unitnamelen == 0)
		p += dir->namelen;
	else if (dir->da0.unitnamelen > 0) {
		p += dir->namelen * dir->da0.unitnamelen;
		dir->namelen = sar_getnum_s7s(&dir->name);
	} else if (dir->da0.unitnamelen < -1)
		p -= dir->da0.unitnamelen;
	/* dir->da0.unitnamelen == -1 �́A�t�@�C�����Ȃ����[�h(ID) */
	/* ID�ԍ���00-0f�̓��U�[�u�Ŏg���Ȃ� */

	dir->at.attr = sar_getnum_s7s(&p);
	i = dir->at.attr & 0x0f;
	if (0x03 <= i && i <= 0x07) {
		/* �f�B���N�g���n */
		dir->at.attr ^= dir->da0.baseattr_d << 5;
		dir->at.permission = dir->da0.baseperm_d;
	} else {
		/* �t�@�C���n */
		dir->at.attr ^= dir->da0.baseattr_f << 5;
		dir->at.permission = dir->da0.baseperm_f;
	}
	if (dir->at.attr & 0x40)
		dir->at.permission ^= sar_getnum_s7s(&p); /* �p�[�~�b�V�������� */
	dir->at.permission = sar_permconv_s2u(dir->at.permission);

	if ((dir->at.attr & 0x20) == 0)
		p = sar_gettime(p, &dir->at, &dir->da0); /* �������� */
	else
		sar_uc2time((UCHAR *) dir->da0.time0, &dir->at);
	if (dir->da0.flags & 1) {
		i = sar_getnum_s7s(&p);
		if (i & 1)
			i ^= -1;
		i >>= 1;
		dir->s += i * (dir->da0.align + 1);
	}
	dir->size = sar_getnum_s7s(&p);
	i = dir->at.attr & 0x0f;
	dir->content = dir->s;
	if (0x04 <= i && i <= 0x05) {
		/* �f�B���N�g�� */
		if (i == 0x04) {
			/* �C�����C���f�B���N�g�� */
			dir->content1 = p;
			i = sar_getnum_s7s(&p); /* size��0���ƃt�@�C���Ȃ� */
			p += i;
		}
		dir->s += dir->size * (dir->da0.align + 1); /* size��0���ƒ��g�Ȃ� */
	} else
		dir->s += ((dir->size + dir->da0.align) & ~dir->da0.align);
	dir->p = p;
	return;
}

/* sar�T�|�[�g�֐��Q�i�����p�j */

UCHAR *sar_puts7s(UCHAR *p, unsigned int i)
{
	if (i < 0x80)
		goto len1;
	if (i < 0x4000)
		goto len2;
	if (i < 0x200000)
		goto len3;
	if (i < 0x10000000)
		goto len4;
	*p++ = ((i >> 28) & 0x7f) << 1;
len4:
	*p++ = ((i >> 21) & 0x7f) << 1;
len3:
	*p++ = ((i >> 14) & 0x7f) << 1;
len2:
	*p++ = ((i >>  7) & 0x7f) << 1;
len1:
	*p++ = (i & 0x7f) << 1 | 1;
	return p;
}

void sar_openarchive_w(struct sar_archandle1 *handle, int siz, UCHAR *buf, int flags)
/* �������ݐ�p���[�h�ŃA�[�J�C�u���J�� */
{
	int i, j;
	UCHAR *p, *q;
	handle->da0.alignbase = buf;
	handle->p1 = buf + siz;

	j = handle->da0.align + 1;
	for (i = 0; (1 << i) < j; i++);

	p = buf;
	if ((j = handle->reservelen) != 0) {
		do {
			*p++ = 0x00;
		} while (--j);
	}
	q = "sar\0KHB0\1\1\1\1\x2d\x3f\x0f\2"; /* 15�o�C�g */
	if (flags & 2)
		q = "sar\0KHB0\1\x51\1\x41\x13\x0f\2"; /* 14�o�C�g */
	for (; *q != 2; *p++ = *q++);

	*p++ = i + i + 1; /* �A���C�������� */

	handle->s = handle->s0 = buf + 1024 * 1024;
	handle->p = p + 3;
	handle->dirlev = 0;
	handle->lastdir[0] = '\0'; /* ���[�g�ȊO�ł͖�����/ */
	handle->dir[0].p0 = p;
	handle->dir[0].pl = &handle->lastdir[0];

	for (i = 0; i < 6; i++)
		handle->da0.time0[i] = 0;
	handle->da0.baseattr_f = 0x20; /* archive-flag(DOS-compatible) */
	handle->da0.baseattr_d = 0;
	handle->da0.baseperm_f = 0x007;
	handle->da0.baseperm_d = 0x007;

	if ((flags & 2) == 0)
		sar_time2uc(&sar_atinv, (UCHAR *) handle->da0.time0);
	return;
}

UCHAR *sar_putfile_w(struct sar_fileinfo0 *file, struct sar_archandle1 *arc, int flags)
{
	UCHAR c, *p = arc->p, *s, *q;
	int i, j, perm;
	file->name[file->namelen] = '\0';
recheck:
	c = 0;
	for (i = 0; arc->lastdir[i] != '\0'; i++) {
		if (i >= file->namelen) {
			c = 1;
			break;
		}
		c |= arc->lastdir[i] ^ file->name[i];
	}
	if (c) {
		/* �vclosedir */
		p = sar_closedir_w(arc, p);
		goto recheck;
	}
	j = i;
	while (i < file->namelen) {
		if (file->name[i] == '/') {
			c = 1;
			break;
		}
		i++;
	}
	s = &file->name[j];
	if (c) {
		/* �vmakedir */
		/* file->name[j]����[i-1]�܂łɐ�������ׂ��f�B���N�g���������� */
		arc->dirlev++;
		arc->dir[arc->dirlev].pl = q = &arc->lastdir[j];
		arc->dir[arc->dirlev].s0 = arc->s;
		*p++ = (i - j) << 1 | 1;
		for (; *s != '/'; *p++ = *q++ = *s, s++);
		q[0] = '/';
		q[1] = '\0';
		if (flags & 2) {
			if (s[1] == '\0') {
				/* �蔲���ŁA����time�͏o�� */
				perm = sar_permconv_u2s(file->at.permission);
				i = (file->at.attr & ~0x7f) | 0x04;
				if (arc->da0.baseperm_d != perm)
					i |= 0x40; /* perm */
				p = sar_puts7s(p, i ^ (arc->da0.baseattr_d << 5));
				if (arc->da0.baseperm_d != perm)
					p = sar_puts7s(p, perm ^ arc->da0.baseperm_d);
				p = sar_puttime(p, &file->at, &arc->da0);
			} else {
				*p++ = 0x09 ^ (arc->da0.baseattr_d << 6); /* �����i�C�����C���f�B���N�g���j, �������� */
				p = sar_puttime(p, &sar_atinv, &arc->da0);
			}
		} else
			*p++ = 0x49 ^ (arc->da0.baseattr_d << 6); /* �����i�C�����C���f�B���N�g���j, �����Ȃ� */
		arc->dir[arc->dirlev].ps = p;
		p += 3;
		arc->dir[arc->dirlev].p0 = p;
		p += 3;
		goto recheck;
	}
	if (*s == '\0') {
		/* �f�B���N�g���������Ȃ� */
		s = NULL;
		goto fin;
	}
	*p++ = (file->namelen - j) << 1 | 1;
	for (; *s != 0; *p++ = *s++);
	if (flags & 2) {
		/* �蔲���ŁA����time�͏o�� */
		perm = sar_permconv_u2s(file->at.permission);
		i = file->at.attr & ~0x7f;
		if (arc->da0.baseperm_f != perm)
			i |= 0x40; /* perm */
		p = sar_puts7s(p, i ^ (arc->da0.baseattr_f << 5));
		if (arc->da0.baseperm_f != perm)
			p = sar_puts7s(p, perm ^ arc->da0.baseperm_f);
		p = sar_puttime(p, &file->at, &arc->da0);
	} else
		*p++ = 0x41 ^ (arc->da0.baseattr_f << 6); /* ���� */
	s = arc->s;
	p = sar_puts7s(p, file->size);
	arc->s += file->size;
	i = (arc->s - arc->da0.alignbase) & arc->da0.align;
	if (i > 0)
		arc->s += arc->da0.align - i + 1;
fin:
	arc->p = p;
	return s; /* ����s����arc->s�܂ł�fill���邱�� */
}

UCHAR *sar_closedir_w(struct sar_archandle1 *arc, UCHAR *p)
/* �C�����C���f�B���N�g������� */
{
	UCHAR *q, *r;
	int i;
	*p++ = 0x01;
	i = (arc->s - arc->dir[arc->dirlev].s0) / (arc->da0.align + 1);
	r = sar_puts7s(arc->dir[arc->dirlev].ps, i);
	q = arc->dir[arc->dirlev].p0 + 3;
	r = sar_puts7s(r, p - q);
	do {
		*r++ = *q++;
	} while (q < p);
	q = arc->dir[arc->dirlev].pl;
	*q = '\0';
	arc->dirlev--;
	return r;
}

int sar_closearchive_w(struct sar_archandle1 *arc)
{
	UCHAR *p = arc->p, *q, *s0;
	int i;

	while (arc->dirlev)
		p = sar_closedir_w(arc, p);

	*p++ = 0x01;

	/* s0�̌��� */
	s0 = arc->da0.alignbase + (((p - arc->da0.alignbase) + arc->da0.align) & ~arc->da0.align);

	if (arc->da0.align == 0) {
		/* ��������߂� */
		q = arc->dir[0].p0 + 3;
		p = sar_puts7s(arc->dir[0].p0, s0 - q);
		if (p < q) {
			do {
				*p++ = *q++;
			} while (q < s0);
			s0 = p;
		}
	} else {
		/* ����������ɂ߂�i�蔲���j */
		while (p < s0)
			*p++ = 0x00; /* �A���C�� */
		p = arc->dir[0].p0;
		i = ((p + 3 - arc->da0.alignbase) + arc->da0.align) & ~arc->da0.align;
		i = (((s0   - arc->da0.alignbase) + arc->da0.align) & ~arc->da0.align) - i;
		i /= arc->da0.align + 1;
		p[0] =  (i >> 13) & 0xfe;
		p[1] =  (i >>  6) & 0xfe;
		p[2] = ((i <<  1) & 0xfe) | 1;
	}

	/* �R���e���c�]�� */
	p = arc->da0.alignbase + 1024 * 1024;
	while (p < arc->s)
		*s0++ = *p++;

	return s0 - arc->da0.alignbase;
}

int sar_permconv_u2s(int unx)
{
	int sar = unx & ~0x1ff, i;
#if 0
	sar |= ((unx >> 8) & 1);		/* 0400 -> 0001 */
	sar |= ((unx >> 7) & 1) << 1;	/* 0200 -> 0002 */
	sar |= ((unx >> 6) & 1) << 2;	/* 0100 -> 0004 */
	sar |= ((unx >> 5) & 1) << 3;	/* 0040 -> 0010 */
	sar |= ((unx >> 4) & 1) << 4;	/* 0020 -> 0020 */
	sar |= ((unx >> 3) & 1) << 5;	/* 0010 -> 0040 */
	sar |= ((unx >> 2) & 1) << 6;	/* 0004 -> 0100 */
	sar |= ((unx >> 1) & 1) << 7;	/* 0002 -> 0200 */
	sar |= ((unx     ) & 1) << 8;	/* 0001 -> 0400 */
#endif
	for (i = 0; i < 9; i++)
		sar |= (unx >> (8 - i) & 1) << i;
	return sar;
}

void sar_time2uc(struct sar_attrtime *at, UCHAR *uc24)
{
	int i;
	*(int *) &uc24[0] = at->subsec;
		/* uc24�ł�subsec��1000-1000-1000�`�� */
	*(int *) &uc24[4] = at->sec | at->min << 6 | at->hour << 12
		| at->day << 17 | at->mon << 22 | (at->year & 0x1f) << 27;
		/* yyyyymmm_mmdddddh_hhhhmmmmm_mmssssss */
	*(int *) &uc24[8] = at->year >> 5;
	for (i = 12; i < 24; i++)
		uc24[i] = 0;
	if (at->subsec == -1) {
		*(int *) &uc24[0] = 0;
		*(int *) &uc24[4] = 0x1f << 22; /* 31���ɂ��� */
		*(int *) &uc24[8] = 0;
	}
	return;
}

void sar_uc2time(UCHAR *uc24, struct sar_attrtime *at)
{
	unsigned int i = *(int *) &uc24[4];
	at->subsec = *(int *) &uc24[0];
	at->sec = i & 0x3f;
	at->min = (i >> 6) & 0x3f;
	at->hour = (i >> 12) & 0x1f;
	at->day = (i >> 17) & 0x1f;
	at->mon = (i >> 22) & 0x1f;
	at->year = i >> 27 | (*(int *) &uc24[8]) << 5;
	if (at->mon == 0x1f)
		at->subsec |= -1;
	return;
}

void sar_neguc24(UCHAR *uc24)
{
	int i;
	for (i = 0; i < 24; i++)
		uc24[i] ^= -1;
	for (i = 0; i < 24; i++) {
		uc24[i]++;
		if (uc24[i])
			break;
	}
	return;
}

void sar_adduc24(UCHAR *dst, UCHAR *src)
{
	int i, c = 0;
	for (i = 0; i < 24; i++) {
		c = dst[i] + src[i] + c;
		dst[i] = c & 0xff;
		c >>= 8;
	}
	return;
}

void sar_shiftluc24(UCHAR *uc24, int shft)
/* 1�`31 */
{
	int i;
	for (i = 6; i >= 0; i--) {
		((unsigned int *) uc24)[i] <<= shft;
		if (i)
			((unsigned int *) uc24)[i] |= ((unsigned int *) uc24)[i - 1] >> (32 - shft);
	}
	return;
}

void sar_shiftruc24(UCHAR *uc24, int shft)
/* 1�`31 */
{
	int i;
	for (i = 0; i < 6; i++) {
		((unsigned int *) uc24)[i] >>= shft;
		if (i < 5)
			((unsigned int *) uc24)[i] |= ((unsigned int *) uc24)[i + 1] << (32 - shft);
	}
	return;
}

UCHAR *sar_puttime(UCHAR *p, struct sar_attrtime *at, struct sar_dirattr0 *da0)
{
	int t0[6], t1[6];
	int i;
	for (i = 0; i < 6; i++)
		t1[i] = da0->time0[i];
	sar_time2uc(at, (UCHAR *) t0);
	sar_neguc24((UCHAR *) t1);
	sar_adduc24((UCHAR *) t0, (UCHAR *) t1);
	/* �S���� 24 * 8 = 192 bit ���� */
	/* ���Ԃ̒P�ʂ�9(-3,dec)�Œ� ... �蔲�� */
	sar_shiftruc24((UCHAR *) t0, 20); /* �����172bit�B����� 24 * 7 = 168 bit �ɕ��� */
	sar_shiftluc24((UCHAR *) t0, 23);

	i = 24;
	for (;;) {
		if (((UCHAR *) t0)[23])
			break;
		if (i <= 1)
			break;
		sar_shiftluc24((UCHAR *) t0,  7);
		i--;
	}
	do {
		*p++ = ((UCHAR *) t0)[23] << 1;
		sar_shiftluc24((UCHAR *) t0,  7);
	} while (--i);
	p[-1] |= 1;
	return p;
}

UCHAR *sar_gettime(UCHAR *p, struct sar_attrtime *at, struct sar_dirattr0 *da0)
{
	int t0[6], i;
	for (i = 0; i < 6; i++)
		t0[i] = 0;
	do {
		sar_shiftluc24((UCHAR *) t0,  7);
		*(UCHAR *) t0 |= *p++ >> 1;
	} while ((p[-1] & 1) == 0);
	sar_shiftluc24((UCHAR *) t0, 20);
	sar_adduc24((UCHAR *) t0, (UCHAR *) da0->time0);
	sar_uc2time((UCHAR *) t0, at);
	return p;
}

void sar_shifttime(struct sar_attrtime *at, int min, void *opt)
/* �������������A���������Ƃ����P���Ȃ��� */
/* �Ď��ԂȂ�Ă���܂��� */
/* �����Ƃ�����������������Ă������� */
{
	static UCHAR days[] = {
		31, 28, 31, 30, 31, 60, 31, 31, 30, 31, 30, 31
	};
	UCHAR max;
	if (min >= 0) {
		min += at->min;
		at->min = min % 60;
		if (min /= 60) {
			min += at->hour;
			at->hour = min % 24;
			at->day += min / 24; /* �����܂����蔲���A32���ɂȂ��Ă������� */
			max = days[at->mon - 1];
			if (at->mon == 2 && (at->year & 3) == 0) {
				if ((at->year % 100) != 0 || (at->year % 400) == 0)
					max++;
			}
			if (at->day > max) {
				at->day -= max;
				at->mon++;
				if (at->mon > 12) {
					at->mon = 1;
					at->year++;
				}
			}
		}
	} else {
		int tmp;
		min += at->min;
		tmp = min % 60;
		min /= 60;
		while (tmp < 0) {
			tmp += 60;
			min--;
		}
		at->min = tmp;
		if (min < 0) {
			min += at->hour;
			tmp = min % 24;
			min /= 24;
			while (tmp < 0) {
				tmp += 24;
				min--;
			}
			at->hour = tmp;
			at->day += min; /* �����܂����蔲���A0���ɂȂ��Ă������� */
			if (at->day <= 0) {
				at->mon--;
				if (at->mon <= 0) {
					at->mon += 12;
					at->year--;
				}
				max = days[at->mon - 1];
				if (at->mon == 2 && (at->year & 3) == 0) {
					if ((at->year % 100) != 0 || (at->year % 400) == 0)
						max++;
				}
				at->day += max;
			}
		}
	}
	return;
}
