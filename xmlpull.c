/*
 * Copy me if you can.
 * by 20h
 */

#ifndef PLAN9
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#endif
#ifdef PLAN9
#include <u.h>
#include <libc.h>
#endif
#include "xmlpull.h"

void *
reallocp(void *p, int s, short d)
{
	p = realloc(p, s);
	if(p == nil){
		perror("realloc");
		exits("realloc");
	}

	if(d != 0)
		memset(p, 0, s);

	return (void *)p;
}

void
freexmlpull(xmlpull *x)
{
	if(x != nil){
		if(x->na != nil)
			free(x->na);
		if(x->va != nil)
			free(x->va);
		free(x);
	}

	return;
}

xmlpull *
openxmlpull(int fd)
{
	xmlpull *ret;

	ret = reallocp(nil, sizeof(xmlpull), 2);
	ret->na = nil;
	ret->va = nil;
	ret->lm = nil;
	ret->ln = 0;
	ret->lv = 0;
	ret->la = 0;
	ret->ev = START_DOCUMENT;
	ret->nev = START_DOCUMENT;
	ret->fd = fd;

	return ret;
}

char
getchara(xmlpull *x)
{
	char g;

	if(read(x->fd, &g, 1) <= 0){
		x->ev = END_DOCUMENT;
		return (char)0;
	}

	return g;
}

char *
addchara(char *b, int *l, char c)
{
	b = reallocp(b, ++(*l) + 1, 0);
	b[(*l) - 1] = c;
	b[*l] = '\0';

	return b;
}

char *
readuntil(xmlpull *x, char *b, int *l, char w, char t)
{
	char g;
	
	while((g = getchara(x)) != 0){
		//print("||%c>%c||", g, w);
		if(g == w){
			b = addchara(b, l, '\0');
			return b;
		}

		switch(g){
		case '/':
		case '>':
			if(t != 0){
				addchara(b, l, g);
				return nil;
			}
		case '\t':
		case '\r':
		case '\n':
		case ' ':
			if(t != 0)
				return b;
			b = addchara(b, l, g);
			break;
		case '\\':
			g = getchara(x);
			//print("%c", g);
			if(g == 0)
				return nil;
			b = addchara(b, l, g);
			break;
		default:
			b = addchara(b, l, g);
			break;
		}
	}

	return nil;
}


char *
parseattrib(xmlpull *x)
{
	char g, *b;

	while((g = getchara(x)) != 0){
		//print("%c", g);
		switch(g){
		case '\t':
		case '\r':
		case '\n':
		case ' ':
			continue;
		case '/':
		case '>':
			x->na = addchara(x->na, &x->ln, g);
			return nil;
		default:
			x->na = addchara(x->na, &x->ln, g);
			g = (char)0;
		}
		if(g == (char)0)
			break;
	}

	if((b = readuntil(x, x->na, &x->ln, '=', 2)) == nil)
		return nil;
	x->na = b;
	
	if((g = getchara(x)) == 0)
		return nil;

	//print("magic char: %c\n", g);
	switch(g){
	case '"':
	case '\'':
		if((b = readuntil(x, x->va, &x->lv, g, 0)) == nil)
			return nil;
		x->va = b;
		return x->va;
	default:
		if((b = readuntil(x, x->va, &x->lv, '>', 2)) == nil)
			return nil;
		x->va = b;
		return x->na;
	}
	
	return x->na;
}

char *
readname(xmlpull *x)
{
	char g;

	while((g = getchara(x)) != 0){
		//print("%c", g);
		switch(g){
		case '\n':
		case '\t':
		case '\r':
		case ' ':
		case '>':
		case '/':
			x->na = addchara(x->na, &x->ln, g);
			return x->na;
		default:
			x->na = addchara(x->na, &x->ln, g);
		}
	}

	return nil;
}

char *
readcdata(xmlpull *x)
{
	char g;
	while((g = getchara(x)) != 0){
		x->na = addchara(x->na, &x->ln, g);
		if(strncmp("]]>", &x->na[x->ln-3], 3) == 0) {
			x->na[x->ln-3] = '\0';
			break;
		}
	}

	x->na[x->ln-1] = '\0'; /* if while breaks */
	//print("X: '%s'\n", x->na);
	return x->na;
}

int
checkcdata(xmlpull *x)
{
	char name[7];
	int i = 7;
	while(i) {
		name[7-i] = getchara(x);
		i--;
	}
	if(strncmp("[CDATA[", name, 7) != 0) {
		return 0;
	}
	x->ev = CDATA;
	x->na = nil;
	x->nev = TEXT;
	
	/* read cdata contents in na */
	x->na = readcdata(x);
	return 1;
}

xmlpull *
nextxmlpull(xmlpull *x)
{
	char g;

	if(x->va != nil)
		free(x->va);

	if(x->ev == START_TAG){
		if(x->lm != nil)
			free(x->lm);
		x->lm = x->na;
		x->la = x->ln;
	} else
		if(x->na != nil)
			free(x->na);

	x->na = nil;
	x->va = nil;
	x->ln = 0;
	x->lv = 0;
	g = '\0';

	switch(x->nev){
	case START_DOCUMENT:
		if((x->na = readuntil(x, x->na, &x->ln, '<', 0)) == nil)
			x->nev = END_DOCUMENT;
		else
			x->nev = START_TAG;
		x->ev = START_DOCUMENT;
		break;
	case START_TAG:
		g = getchara(x);
		//print("%c", g);
		if(g == '/')
			x->ev = END_TAG;
		else if(g == '!' && checkcdata(x))
			break;
		else {
			x->na = addchara(x->na, &x->ln, g);
			x->ev = START_TAG;
		}

		if(readname(x) == nil)
			x->nev = END_DOCUMENT;
		else {
			if(!strncmp(x->na, "!--", 3)){
				x->na[x->ln - 1] = '\0';
				x->nev = TEXT_C;
				return x;
			}
			if(x->ev == END_TAG){
				x->na[x->ln - 1] = '\0';
				x->nev = TEXT;
			} else {
				switch(x->na[x->ln - 1]){
				case '/':
					getchara(x);
					x->ev = START_END_TAG;
					x->nev = TEXT;
					x->na[x->ln - 1] = '\0';
					break;
				case '>':
					x->nev = TEXT;
					x->na[x->ln - 1] = '\0';
					break;
				default:
					x->na[x->ln - 1] = '\0';
					x->nev = ATTR;
				
				}
			}
		}
		break;
	case TEXT_C:
		g = '>';
	case TEXT:
		if(g != '>')
			g = '<';

		if((x->na = readuntil(x, x->na, &x->ln, g, 0)) == nil){
			x->ev = END_DOCUMENT;
			x->nev = END_DOCUMENT + 1;
		} else {
			if(x->nev == TEXT_C)
				x->nev = TEXT;
			else
				x->nev = START_TAG;
			x->ev = TEXT;
		}
		break;
	case ATTR:
		if(parseattrib(x) == nil){
			//print("%c\n", x->na[x->ln - 1]);
			switch(x->na[x->ln - 1]){
			case '/':
				free(x->na);
				x->na = x->lm;
				x->ln = x->la;
				x->lm = nil;
				x->la = 0;

				getchara(x);
				x->ev = END_TAG;
				x->nev = TEXT;
				return x;
			case '>':
			default:
				x->na[x->ln - 1] = '\0';
			}
			x->ev = ATTR;
			x->nev = TEXT;
			return nextxmlpull(x);
		} else
			x->nev = ATTR;
		x->ev = ATTR;
		break;
	case END_DOCUMENT:
		x->ev = END_DOCUMENT;
		x->nev = END_DOCUMENT + 1;
		break;
	default:
		return nil;
	}

	return x;
}

xmlpull *
writexmlpull(xmlpull *x)
{
	char *b;

	b = nil;

	switch(x->nev){
	case START_DOCUMENT:
		if(write(x->fd, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n", 39) < 0)
			return nil;
		return x;
	case START_TAG:
		if(x->na == nil)
			return nil;

		b = reallocp(b, x->ln + 3, 2);
		snprint(b, x->ln + 3, "<%s ", x->na);
		if(write(x->fd, b, strlen(b)) < 0){
			free(b);
			return nil;
		}
		free(b);
		return x;
	case START_END_TAG:
		if(x->na == nil)
			return nil;

		b = reallocp(b, x->ln + 4, 2);
		snprint(b, x->ln + 4, "<%s/>", x->na);
		if(write(x->fd, b, strlen(b)) < 0){
			free(b);
			return nil;
		}
		free(b);
		return x;
	case TEXT:
		if(x->na == nil)
			return nil;
		if(write(x->fd, x->na, x->ln) < 0)
			return nil;
		return x;
	case TEXT_C:
		if(x->na == nil)
			return nil;

		b = reallocp(b, x->ln + 5, 2);
		snprint(b, x->ln + 5, "%s -->", x->na);
		if(write(x->fd, b, strlen(b)) < 0){
			free(b);
			return nil;
		}
		free(b);
		return x;
	case ATTR:
		if(x->na == nil)
			return nil;

		b = reallocp(b, x->ln + x->lv + 5, 2);
		snprint(b, x->ln + x->lv + 5, "%s=\"%s\" ", x->na, (x->va == nil) ? "" : x->va);
		if(write(x->fd, b, strlen(b)) < 0){
			free(b);
			return nil;
		}
		free(b);
		return x;
	case END_TAG:
		if(x->na == nil)
			return nil;

		b = reallocp(b, x->ln + 4, 2);
		snprint(b, x->ln + 4, "</%s>", x->na);
		if(write(x->fd, b, strlen(b)) < 0){
			free(b);
			return nil;
		}
		free(b);
		return x;
	case END_TAG_S:
		if(write(x->fd, "/>", 2) < 0)
			return nil;
		return x;
	case END_TAG_N:
		if(write(x->fd, ">", 1) < 0)
			return nil;
		return x;
	case END_DOCUMENT:
		close(x->fd);
		return nil;	
	default:
		break;
	}

	return nil;
}

