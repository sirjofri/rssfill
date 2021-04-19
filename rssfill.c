#include <u.h>
#include <libc.h>
#include <bio.h>
#include <String.h>
#include "xmlpull.h"
#include "rssfill.h"

char  *directory = "/lib/news";
char  *prefix = "";

int chatty = 0;
int dry = 0;

int dohtml;
int typehtml;

void
usage(void)
{
	fprint(2, "usage: %s "
		"[ -ct ] "
		"[ -p prefix ] "
		"[ -d directory ]\n", argv0);
	exits("usage");
}

char*
html(char *text)
{
	char *s, buf[8192];
	String *str;
	int n, m, written;
	int p[2];

	if (!dohtml)
		return strdup(text);
	dohtml = 0;

	if (pipe(p) < 0)
		sysfatal("pipe: %r");

	s = nil;
	switch (fork()){
	case -1:
		close(p[0]);
		close(p[1]);
		return strdup(text);
		break;
	case 0:
		dup(p[1], 0);
		dup(p[1], 1);
		close(p[1]);
		close(p[0]);
		execl("/bin/htmlfmt", "htmlfmt", "-cutf-8", nil);
		exits(nil);
	default:
		close(p[1]);
		str = s_new();
		written = 0;
		while (written < strlen(text) && (n = write(p[0], &text[written], strlen(&text[written]))) > 0){
			written += n;
			write(p[0], "", 0); // htmlfmt needs double flush, idk why
			write(p[0], "", 0);
			m = read(p[0], buf, 8191);
			buf[m] = 0;
			str = s_append(str, buf);
		}
		close(p[0]);
		while (waitpid() > 0)
			;
		s = strdup(s_to_c(str));
		s_free(str);
	}

	if (s)
		return s;
	return strdup(text);
}

void
writefeedfiles(Feed *f)
{
	int fd;
	char *file = nil;
	long d;
	Tm t;
	Dir dir;

	if(f != nil){
		while(f->n != nil)
			f = f->n;

		while(f != nil){
			if(f->s == 2){
				if(tmparse(&t, "W[,] ?D MMM YYYY hh:mm:ss ?Z", f->date, nil, nil) == nil)
					if(tmparse(&t, "YYYY-MM-DD[T]hh:mm:ss?Z", f->date, nil, nil) == nil)
						if(tmparse(&t, nil, f->date, nil, nil) == nil)
							sysfatal("tmparse: %r");
						else
							fprint(2, "tmparse: auto parsed date\n");

				d = tmnorm(&t);

				if(file)
					free(file);
				file = smprint("%s/%s%ld", directory, prefix, d);

				fd = create(file, OWRITE, 0666);
				if(!fd)
					sysfatal("error creating file %s: %r", file);

				if(chatty)
					fprint(2, "writing file %s\n", file);

				if(dry){
					f = f->p;
					continue;
				}
				if(f->title != nil)
					fprint(fd, "title:   %s\n", f->title);
				if(f->date != nil)
					fprint(fd, "pubDate: %s (%ld)\n", f->date, d);
				if(f->link != nil)
					fprint(fd, "link:    %s\n", f->link);
				if(f->desc != nil)
					fprint(fd, "\n%s\n", f->desc);
				if(f->cont != nil)
					fprint(fd, "\n%s\n", f->cont);

				nulldir(&dir);
				dir.mtime = d;
				dirfwstat(fd, &dir);

				close(fd);
			}
			f = f->p;
		}
	}
}

void
freefeed(Feed *f)
{
	if(f != nil){
		if(f->title != nil)
			free(f->title);
		if(f->link != nil)
			free(f->link);
		if(f->desc != nil)
			free(f->desc);
		if(f->date != nil)
			free(f->date);
		if(f->cont != nil)
			free(f->cont);
		free(f);
	}
	return;
}

void
freefeedt(Feed *r)
{
	while(r != nil){
		if(r->n != nil){
			r = r->n;
			freefeed(r->p);
		} else {
			freefeed(r);
			r = nil;
		}
	}
}

Feed *
searchfeed(Feed *r, char *title, char *link, char *desc, char *date)
{
	while(r != nil){
		if(r->title != nil && title != nil){
			if(!strcmp(r->title, title)){
				r->s = 1;
				return r;
			}
		}
		if(r->link != nil && link != nil){
			if(!strcmp(r->link, link)){
				r->s = 1;
				return r;
			}
		}
		if(r->desc != nil && desc != nil){
			if(!strcmp(r->desc, desc)){
				r->s = 1;
				return r;
			}
		}
		if(r->date != nil && date != nil){
			if(!strcmp(r->date, date)){
				r->s = 1;
				return r;
			}
		}
		r = r->n;
	}
	return nil;
}

Feed *
addfeed(Feed *r, Feed *f)
{
	Feed *ret;

	ret = r;
	f->s = 2;
	if(r != nil) {
		while(r->n != nil)
			r = r->n;
	} else
		return f;
	r->n = f;
	f->p = r;

	return ret;
}

Feed *
removefeed(Feed *r, Feed *f)
{
	if(f->n != nil && f->p != nil){
		f->n->p = f->p;
		f->p->n = f->n;
	} else {
		if(f->n != nil){
			f->n->p = nil;
			r = f->n;
		}
		if(f->p != nil)
			f->p->n = nil;
	}
	freefeed(f);

	return r;
}

Feed *
checkfeed(Feed *r)
{
	Feed *a;

	a = r;

	while(a != nil){
		if(a->s == 0)
			r = removefeed(r, a);
		else
			a->s = 0;
		a = a->n;
	}

	return r;
}

void
main(int argc, char **argv)
{
	xmlpull *x, *a;
	char st;
	Feed *f, *r;

	ARGBEGIN {
	case 'd':
		directory = EARGF(usage());
		break;
	case 'p':
		prefix = EARGF(usage());
		break;
	case 't':
		dry = 1;
		break;
	case 'c':
		chatty = 1;
		break;
	} ARGEND;

	if(dry)
		chatty = 1;

	st = NONE;
	f = nil;
	r = nil;

	x = openxmlpull(0);
	while((a = nextxmlpull(x)) != nil && st != END){
		switch(a->ev){
		case START_DOCUMENT:
			break;
		case START_TAG:
			if(!strcmp(x->na, "item") || !strcmp(x->na, "entry")){
				if(f != nil)
					freefeed(f);
				f = mallocz(sizeof(Feed), 2);
				st = ITEM;
				break;
			}
			if(!strcmp(x->na, "title") && st == ITEM){
				st = TITLE;
				break;
			}
			if(!strcmp(x->na, "description") && st == ITEM){
				st = DESC;
				break;
			}
			if(!strcmp(x->na, "summary") && st == ITEM){
				st = DESC;
				break;
			}
			if(!strcmp(x->na, "content") && st == ITEM){
				st = CONTENT;
			}
			if(!strcmp(x->na, "link") && st == ITEM){
				st = LINK;
				break;
			}
			if(!strcmp(x->na, "pubDate") && st == ITEM){
				st = DATE;
				break;
			}
			if(!strcmp(x->na, "updated") && st == ITEM){
				st = DATE;
				break;
			}
			break;
		case START_END_TAG:
			break;
		case ATTR:
			if(!strcmp(x->na, "href") && st == LINK)
				f->link = strdup(x->va);
			if(!strcmp(x->na, "type") && !cistrcmp(x->va, "html"))
				typehtml = 1;
			break;
		case CDATA:
			/* if typehtml AND cdata, do html */
			dohtml = typehtml;
		case TEXT:
			switch(st){
			case TITLE:
				if (!f->title || strlen(f->title) == 0)
					f->title = html(x->na);
				break;
			case LINK:
				if (!f->link || strlen(f->link) == 0)
					f->link = strdup(x->na);
				break;
			case DESC:
				if (!f->desc || strlen(f->desc) == 0)
					f->desc = html(x->na);
				break;
			case CONTENT:
				if (!f->cont || strlen(f->cont) == 0)
					f->cont = html(x->na);
				break;
			case DATE:
				if (!f->date || strlen(f->date) == 0)
					f->date = strdup(x->na);
				break;
			default:
				break;
			}
			break;
		case END_TAG:
			if((!strcmp(x->na, "item") || !strcmp(x->na, "entry")) && st == ITEM){
			//	if(searchfeed(r, f->title, f->link, f->desc, f->date) == nil){
					r = addfeed(r, f);
					f = nil;
			//	} else {
			//		freefeed(f);
			//		f = nil;
			//	}
							
				st = NONE;
				break;
			}
			if(!strcmp(x->na, "title") && st == TITLE){
				st = ITEM;
				break;
			}
			if(!strcmp(x->na, "link") && st == LINK){
				st = ITEM;
				break;
			}
			if(!strcmp(x->na, "description") && st == DESC){
				st = ITEM;
				break;
			}
			if(!strcmp(x->na, "summary") && st == DESC){
				st = ITEM;
				break;
			}
			if(!strcmp(x->na, "content") && st == CONTENT){
				st = ITEM;
				break;
			}
			if(!strcmp(x->na, "pubDate") && st == DATE){
				st = ITEM;
				break;
			}
			if(!strcmp(x->na, "updated") && st == DATE){
				st = ITEM;
				break;
			}
			if(!strcmp(x->na, "rdf:RDF") || !strcmp(x->na, "items")
					|| !strcmp(x->na, "rss") || !strcmp(x->na, "feed")){
				writefeedfiles(r);
				r = checkfeed(r);
				break;
			}
			break;
		case END_DOCUMENT:
			st = END;
			break;
		default:
			sysfatal("Error, should never happen: %x", x->ev);
			break;
		}
	}
	freexmlpull(x);
//	freefeedt(r);
	exits(nil);
}
