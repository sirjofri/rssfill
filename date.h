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
parsedate(char *s)
{
	Tm ret;
	char input[64];
	char *args[8];
	int n, i;
	
	strcpy(input, s);
	n = getfields(input, args, 8, 1, ", :");
	
	if(n < 8)
		sysfatal("error parsing pubDate: %s", s);
	
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
	/*
	ret.zone[0] = 'C';
	ret.zone[1] = 'E';
	ret.zone[2] = 'S';
	ret.zone[3] = 'T';
	*/
	ret.yday = doty(ret.mday, ret.mon, ret.year);
	
	return tm2sec(&ret) - ret.tzoff*60*60;
}