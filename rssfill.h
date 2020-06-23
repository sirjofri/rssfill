typedef struct Feed Feed;
struct Feed {
	char *title;
	char *link;
	char *desc;
	char *date;
	char *cont;
	int s;
	Feed *n;
	Feed *p;
};

enum {
	NONE = 0x00,
	ITEM,
	TITLE,
	LINK,
	DESC,
	CONTENT,
	DATE,
	END,
};
