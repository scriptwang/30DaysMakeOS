/* for stdc */

int GOLD_write_t(const UCHAR *name, int len, const UCHAR *p0)
/* �e�L�X�g���[�h�Ńt�@�C���ɏo�́B����name��NULL�Ȃ�A�W���o�͂֏o�� */
{
	int ll = 0;
	FILE *fp = stdout;
	if (name) {
		fp = fopen(name, "w");
		if (fp == NULL)
			goto err;
	}
	if (len)
		ll = fwrite(p0, 1, len, fp);
	if (name)
		fclose(fp);
	if (ll != len)
		goto err;
	return 0;
err:
	return 1;
}

