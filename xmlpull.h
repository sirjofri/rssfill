/*
 * Copy me if you can.
 * by 20h
 */

/*
#ifdef nil
#pragma lib "libxmlpull.a"
#endif
*/

#ifndef XMLPULL_H
#define XMLPULL_H

#ifndef nil
#define nil NULL
#define print printf
#define snprint snprintf
#define exits return
#endif

enum { 
	START_DOCUMENT = 0x0,
	START_TAG,
	START_END_TAG,
	CDATA,
	TEXT,
	TEXT_C,
	ATTR,
	END_TAG,
	END_TAG_S,
	END_TAG_N,
	END_DOCUMENT,
};

typedef struct xmlpull xmlpull;
struct xmlpull {
	int fd;
	char ev;
	char nev;
	char *lm;
	char *na;
	char *va;
	int la;
	int lv;
	int ln;
};

void freexmlpull(xmlpull *x);
xmlpull *openxmlpull(int fd);
xmlpull *nextxmlpull(xmlpull *x);
xmlpull *writexmlpull(xmlpull *x);

#endif
