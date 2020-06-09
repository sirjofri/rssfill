#include <u.h>
#include <libc.h>
#include "date.h"

int
str2mon(char *s)
{
	if(!strcmp(s, "Jan")) return 0;
	if(!strcmp(s, "Feb")) return 1;
	if(!strcmp(s, "Mar")) return 2;
	if(!strcmp(s, "Apr")) return 3;
	if(!strcmp(s, "May")) return 4;
	if(!strcmp(s, "Jun")) return 5;
	if(!strcmp(s, "Jul")) return 6;
	if(!strcmp(s, "Aug")) return 7;
	if(!strcmp(s, "Sep")) return 8;
	if(!strcmp(s, "Oct")) return 9;
	if(!strcmp(s, "Nov")) return 10;
	if(!strcmp(s, "Dec")) return 11;
	return 0;
}

int
str2wday(char *s)
{
	if(!strcmp(s, "Sun")) return 0;
	if(!strcmp(s, "Mon")) return 1;
	if(!strcmp(s, "Tue")) return 2;
	if(!strcmp(s, "Wed")) return 3;
	if(!strcmp(s, "Thu")) return 4;
	if(!strcmp(s, "Fri")) return 5;
	if(!strcmp(s, "Sat")) return 6;
	return 0;
}

int
doty(int day, int month, int year)
{
	int n, i;
	
	n = 0;
	for(i = 0; i < month; i++){
		if(i == 0) n += 31;
		if(i == 1){
			if(year%4 == 0 && year%100 == 0 && year%400 != 0)
				n += 29;
			else
				n += 28;
		}
		if(i == 2) n += 31;
		if(i == 3) n += 30;
		if(i == 4) n += 31;
		if(i == 5) n += 30;
		if(i == 6) n += 31;
		if(i == 7) n += 31;
		if(i == 8) n += 30;
		if(i == 9) n += 31;
		if(i == 10) n += 30;
		if(i == 11) n += 31;
	}
	
	n += day;
	
	return n;
}

long
parseatomdate(char *input)
{
	/* YYYY-MM-DDTHH:MM:SSZ */
	/* YYYY                          year                    */
	/*      MM                       month                   */
	/*         DD                    day of month            */
	/*           T                   or space                */
	/*            HH                 hour                    */
	/*               MM              minute                  */
	/*                  SS           seconds                 */
	/*                    .SS        seconds (sub, optional) */
	/*                       Z       zulu (UTC+0)            */
	/*                       +HH:MM  tz offset               */
	
	/* examples:                           length */
	/*   2020-06-05( |T)13:20:35.44+04:00      28 */
	/*   2020-06-05( |T)13:20:35+04:00         25 */
	/*   2020-06-05( |T)13:20:35.44Z           23 */
	/*   2020-06-05( |T)13:20:35Z              20 */
	
	Tm ret;
	char *args[9];
	char s[32];
	int n;
	
	strcpy(s, input);
	
	switch(strlen(s)){
	case 28:
		n = getfields(s, args, 9, 1, "- T:.+Z");
		if(n != 9)
			sysfatal("error parsing atom date: %d", n);
		ret.year = atoi(args[0]) - 1900;
		ret.mon = atoi(args[1]) - 1;
		ret.mday = atoi(args[2]);
		ret.tzoff = atoi(args[7]);
		ret.hour = atoi(args[3]);
		ret.min = atoi(args[4]);
		ret.sec = atoi(args[5]);
		break;
	case 25:
		n = getfields(s, args, 8, 1, "- T:.+Z");
		if(n != 8)
			sysfatal("error parsing atom date: %d", n);
		ret.year = atoi(args[0]) - 1900;
		ret.mon = atoi(args[1]) - 1;
		ret.mday = atoi(args[2]);
		ret.tzoff = atoi(args[6]);
		ret.hour = atoi(args[3]);
		ret.min = atoi(args[4]);
		ret.sec = atoi(args[5]);
		break;
	case 23:
		n = getfields(s, args, 7, 1, "- T:.+Z");
		if(n != 7)
			sysfatal("error parsing atom date: %d", n);
		ret.year = atoi(args[0]) - 1900;
		ret.mon = atoi(args[1]);
		ret.mday = atoi(args[2]);
		ret.tzoff = 0;
		ret.hour = atoi(args[3]);
		ret.min = atoi(args[4]);
		ret.sec = atoi(args[5]);
		break;
	case 20:
		n = getfields(s, args, 6, 1, "- T:.+Z");
		if(n != 6)
			sysfatal("error parsing atom date: %d", n);
		ret.year = atoi(args[0]) - 1900;
		ret.mon = atoi(args[1]) - 1;
		ret.mday = atoi(args[2]);
		ret.tzoff = 0;
		ret.hour = atoi(args[3]);
		ret.min = atoi(args[4]);
		ret.sec = atoi(args[5]);
		break;
	default:
		sysfatal("error parsing atom date format: %s", s);
	}
	ret.yday = doty(ret.mday, ret.mon, ret.year);
	
	return tm2sec(&ret) - ret.tzoff*60*60;
}

long
parsedate(char *s)
{
	Tm ret;
	char input[64];
	char *args[8];
	int n, i;
	
	strcpy(input, s);
	n = getfields(input, args, 8, 1, ", :");
	
	if(n < 8){
		/* try parsing atom date */
		return parseatomdate(s);
	}
	
	for(i = 0; i < n; i++){
		if(!args[i])
			sysfatal("error parsing pubDate: %s", s);
		switch(i){
		case 0: /* day of the week */
			ret.wday = str2wday(args[i]);
			break;
		case 1: /* day of the month */
			ret.mday = atoi(args[i]);
			break;
		case 2: /* month of the year */
			ret.mon = str2mon(args[i]);
			break;
		case 3: /* year */
			ret.year = atoi(args[i]) - 1900;
			break;
		case 4: /* hour */
			ret.hour = atoi(args[i]);
			break;
		case 5: /* minute */
			ret.min = atoi(args[i]);
			break;
		case 6: /* second */
			ret.sec = atoi(args[i]);
			break;
		case 7: /* timezone offset */
			ret.tzoff = atoi(args[i])/100;
			break;
		}
	}
	ret.yday = doty(ret.mday, ret.mon, ret.year);
	
	return tm2sec(&ret) - ret.tzoff*60*60;
}