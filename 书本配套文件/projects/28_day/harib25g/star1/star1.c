#include "apilib.h"

void HariMain(void)
{
	char *buf;
	int win;
	api_initmalloc();
	buf = api_malloc(150 * 100);
	win = api_openwin(buf, 150, 100, -1, "star1");
	api_boxfilwin(win,  6, 26, 143, 93, 0 /* �� */);
	api_point(win, 75, 59, 3 /* �� */);
	for (;;) {
		if (api_getkey(1) == 0x0a) {
			break; /* Enter�Ȃ�break; */
		}
	}
	api_end();
}
