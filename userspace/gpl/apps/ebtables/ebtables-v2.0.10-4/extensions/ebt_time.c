/*
  Description: EBTables time extension module for userspace.
  Authors:  Song Wang <songw@broadcom.com>, ported from netfilter/iptables
            The following is the original disclaimer.

 Shared library add-on to iptables to add TIME matching support. 
*/
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>

#include "../include/ebtables_u.h"
#include "../include/linux/netfilter_bridge/ebt_time.h"
#include <time.h>
static int globaldays;

/* Function which prints out usage message. */
static void
help(void)
{
	printf(
"time options:\n"
" --timestart value --timestop value --days listofdays\n"
#if defined(GPL_CODE_SUPPORT_EBTABLES_DATE_MATCH)
" --datestart value --datestop value\n"
#endif
"          timestart value : HH:MM\n"
"          timestop  value : HH:MM\n"
#if defined(GPL_CODE_SUPPORT_EBTABLES_DATE_MATCH)
"          datestart value : YYYY-MM-DD\n"
"          datestop  value : YYYY-MM-DD\n"
#endif
"          listofdays value: a list of days to apply -> ie. Mon,Tue,Wed,Thu,Fri. Case sensitive\n");
}

static struct option opts[] = {
	{ "timestart", required_argument, 0, '1' },
	{ "timestop", required_argument, 0, '2' },
	{ "days", required_argument, 0, '3'},
#if defined(GPL_CODE_SUPPORT_EBTABLES_DATE_MATCH)
    { "datestart", required_argument, 0, '4' },
    { "datestop", required_argument, 0, '5' },
#endif

	{0}
};

/* Initialize the match. */
static void
init(struct ebt_entry_match *m)
{
#if defined(GPL_CODE_SUPPORT_EBTABLES_DATE_MATCH)
        struct ebt_time_info *timeinfo = (struct ebt_time_info *) m->data;
        timeinfo->date_start=0;
        timeinfo->date_stop=0;
#endif
	globaldays = 0;
}

static int
string_to_number(const char *s, unsigned int min, unsigned int max,
                 unsigned int *ret)
{
        long number;
        char *end;

        /* Handle hex, octal, etc. */
        errno = 0;
        number = strtol(s, &end, 0);
        if (*end == '\0' && end != s) {
                /* we parsed a number, let's see if we want this */
                if (errno != ERANGE && min <= number && number <= max) {
                        *ret = number;
                        return 0;
                }
        }
        return -1;
}

/**
 * param: part1, a pointer on a string 2 chars maximum long string, that will contain the hours.
 * param: part2, a pointer on a string 2 chars maximum long string, that will contain the minutes.
 * param: str_2_parse, the string to parse.
 * return: 1 if ok, 0 if error.
 */
static int
split_time(char **part1, char **part2, const char *str_2_parse)
{
	unsigned short int i,j=0;
	char *rpart1 = *part1;
	char *rpart2 = *part2;
	unsigned char found_column = 0;

	/* Check the length of the string */
	if (strlen(str_2_parse) > 5)
		return 0;
	/* parse the first part until the ':' */
	for (i=0; i<2; i++)
	{
		if (str_2_parse[i] == ':')
			found_column = 1;
		else
			rpart1[i] = str_2_parse[i];
	}
	if (!found_column)
		i++;
	j=i;
	/* parse the second part */
	for (; i<strlen(str_2_parse); i++)
	{
		rpart2[i-j] = str_2_parse[i];
	}
	/* if we are here, format should be ok. */
	return 1;
}
#if defined(GPL_CODE_SUPPORT_EBTABLES_DATE_MATCH)
static time_t time_parse_date(const char *s, __u8 end)
{
	unsigned int month = 1, day = 1, hour = 0, minute = 0, second = 0;
	unsigned int year  = end ? 2038 : 1970;
	const char *os = s;
	struct tm tm;
	time_t ret;
	char *e;

	year = strtoul(s, &e, 10);
	if ((*e != '-' && *e != '\0') || year < 1970 || year > 2038)
		goto out;
	if (*e == '\0')
		goto eval;

	s = e + 1;
	month = strtoul(s, &e, 10);
	if ((*e != '-' && *e != '\0') || month > 12)
		goto out;
	if (*e == '\0')
		goto eval;

	s = e + 1;
	day = strtoul(s, &e, 10);
	if ((*e != 'T' && *e != '\0') || day > 31)
		goto out;
	if (*e == '\0')
		goto eval;

	s = e + 1;
	hour = strtoul(s, &e, 10);
	if ((*e != ':' && *e != '\0') || hour > 23)
		goto out;
	if (*e == '\0')
		goto eval;

	s = e + 1;
	minute = strtoul(s, &e, 10);
	if ((*e != ':' && *e != '\0') || minute > 59)
		goto out;
	if (*e == '\0')
		goto eval;

	s = e + 1;
	second = strtoul(s, &e, 10);
	if (*e != '\0' || second > 59)
		goto out;

 eval:
	tm.tm_year = year - 1900;
	tm.tm_mon  = month - 1;
	tm.tm_mday = day;
	tm.tm_hour = hour;
	tm.tm_min  = minute;
	tm.tm_sec  = second;
	tm.tm_isdst = 0;
	/*
	 * Offsetting, if any, is done by xt_time.ko,
	 * so we have to disable it here in userspace.
	 */
	setenv("TZ", "UTC", 1);
	tzset();
	ret = mktime(&tm);
	if (ret >= 0)
		return ret;
	perror("mktime");
	ebt_print_error("mktime returned an error");

 out:
	ebt_print_error("Invalid date \"%s\" specified. Should "
	           "be YYYY[-MM[-DD[Thh[:mm[:ss]]]]]", os);
	return -1;
}


#endif

static void
parse_time_string(unsigned int *hour, unsigned int *minute, const char *time)
{
	char *hours;
	char *minutes;

	hours = (char *)malloc(3);
	minutes = (char *)malloc(3);
	bzero((void *)hours, 3);
	bzero((void *)minutes, 3);

	if (split_time(&hours, &minutes, time) == 1)
	{
                /* if the number starts with 0, replace it with a space else
                   this string_to_number will interpret it as octal !! */
                if ((hours[0] == '0') && (hours[1] != '\0'))
			hours[0] = ' ';
		if ((minutes[0] == '0') && (minutes[1] != '\0'))
			minutes[0] = ' ';

		if((string_to_number(hours, 0, 23, hour) == -1) ||
			(string_to_number(minutes, 0, 59, minute) == -1)) {
			*hour = *minute = (-1);
		}
	}
	if ((*hour != (-1)) && (*minute != (-1))) {
		free(hours);
		free(minutes);
		return;
	}

	/* If we are here, there was a problem ..*/
	ebt_print_error("invalid time %s specified, should be HH:MM format", time);
}

/* return 1->ok, return 0->error */
static int
parse_day(int *days, int from, int to, const char *string)
{
	char *dayread;
	char *days_str[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	unsigned short int days_of_week[7] = {64, 32, 16, 8, 4, 2, 1};
	unsigned int i;

	dayread = (char *)malloc(4);
	bzero(dayread, 4);
	if ((to-from) != 3) {
		free(dayread);
		return 0;
	}
	for (i=from; i<to; i++)
		dayread[i-from] = string[i];
	for (i=0; i<7; i++)
		if (strcmp(dayread, days_str[i]) == 0)
		{
			*days |= days_of_week[i];
			free(dayread);
			return 1;
		}
	/* if we are here, we didn't read a valid day */
	free(dayread);
	return 0;
}

static void
parse_days_string(int *days, const char *daystring)
{
	int len;
	int i=0;
	//char *err = "invalid days specified, should be Sun,Mon,Tue... format";

	len = strlen(daystring);
	if (len < 3)
		ebt_print_error("invalid days specified, should be Sun,Mon,Tue... format");	
	while(i<len)
	{
		if (parse_day(days, i, i+3, daystring) == 0)
			ebt_print_error("invalid days specified, should be Sun,Mon,Tue... format");
		i += 4;
	}
}

#define EBT_TIME_START 0x01
#define EBT_TIME_STOP  0x02
#define EBT_TIME_DAYS  0x04
#if defined(GPL_CODE_SUPPORT_EBTABLES_DATE_MATCH)
#define EBT_DATE_START 0x08
#define EBT_DATE_STOP  0x10
#endif

/* Function which parses command options; returns true if it
   ate an option */
static int
parse(int c, char **argv, int argc,
      const struct ebt_u_entry *entry,
      unsigned int *flags, struct ebt_entry_match **match)
{
	struct ebt_time_info *timeinfo = (struct ebt_time_info *)(*match)->data;
	unsigned int hours, minutes;

	switch (c) /* c is the return value of getopt_long */
	{
		/* timestart */
	case '1':
		if (*flags & EBT_TIME_START)
                        ebt_print_error("Can't specify --timestart twice");
		parse_time_string(&hours, &minutes, optarg);
		timeinfo->time_start = (hours * 60) + minutes;
		*flags |= EBT_TIME_START;
		break;
		/* timestop */
	case '2':
		if (*flags & EBT_TIME_STOP)
                        ebt_print_error("Can't specify --timestop twice");
		parse_time_string(&hours, &minutes, optarg);
		timeinfo->time_stop = (hours * 60) + minutes;
		*flags |= EBT_TIME_STOP;
		break;

		/* days */
	case '3':
		if (*flags & EBT_TIME_DAYS)
                        ebt_print_error("Can't specify --days twice");
		parse_days_string(&globaldays, optarg);
		timeinfo->days_match = globaldays;
		*flags |= EBT_TIME_DAYS;
		break;
#if defined(GPL_CODE_SUPPORT_EBTABLES_DATE_MATCH)
       case '4':
                if (*flags & EBT_DATE_START)
                        ebt_print_error("Can't specify --datestart twice");
                timeinfo->date_start = time_parse_date(optarg,0);
                *flags |= EBT_DATE_START;
                break;
       case '5':
                if (*flags & EBT_DATE_STOP)
                        ebt_print_error("Can't specify --datestop twice");
                timeinfo->date_stop = time_parse_date(optarg,1);
                *flags |= EBT_DATE_STOP;
                break;
#endif
	default:
		return 0;
	}
	/* default value if not specified */
	if (!(*flags & EBT_TIME_START))
		timeinfo->time_start = 0;
	if (!(*flags & EBT_TIME_STOP))
		timeinfo->time_stop = 1439; /* 23*60+59 = 1439*/
	if (!(*flags & EBT_TIME_DAYS))
		timeinfo->days_match = 0;

	return 1;
}

/* Final check; must have specified --timestart --timestop --days. */
static void
final_check(const struct ebt_u_entry *entry,
   const struct ebt_entry_match *match, const char *name,
   unsigned int hookmask, unsigned int time)
{
        struct ebt_time_info *timeinfo = (struct ebt_time_info *)match->data;

	/*
	printf("start=%d,stop=%d,days=%d\n",
		timeinfo->time_start,timeinfo->time_stop,timeinfo->days_match);
	*/
#if !defined(GPL_CODE_SUPPORT_EBTABLES_TIME_OVERNIGHT_MATCH)
	if (timeinfo->time_stop < timeinfo->time_start)
		ebt_print_error("stop time can't be smaller than start time");
#endif
#if defined(GPL_CODE_SUPPORT_EBTABLES_DATE_MATCH)
        if (timeinfo->date_stop < timeinfo->date_start)
		ebt_print_error("stop date can't be smaller than start date");
#endif
}


static void
print_days(int daynum)
{
	char *days[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	unsigned short int days_of_week[7] = {64, 32, 16, 8, 4, 2, 1};
	unsigned short int i, nbdays=0;

	for (i=0; i<7; i++) {
		if ((days_of_week[i] & daynum) == days_of_week[i])
		{
			if (nbdays>0)
				printf(",%s", days[i]);
			else
				printf("%s", days[i]);
			++nbdays;
		}
	}
}

static void
divide_time(int fulltime, int *hours, int *minutes)
{
	*hours = fulltime / 60;
	*minutes = fulltime % 60;
}

/* Prints out the matchinfo. */
static void
print(const struct ebt_u_entry *entry,
      const struct ebt_entry_match *match)
{
	struct ebt_time_info *time = ((struct ebt_time_info *)match->data);
	int hour_start, hour_stop, minute_start, minute_stop;
#if defined(GPL_CODE_SUPPORT_EBTABLES_DATE_MATCH)
        if(time->date_start !=0 && time->date_stop !=0)
        {
            char buff_start[20];
            char buff_stop[20];
            time_t dd = (time_t) time->date_start;
            strftime(buff_start, 20, "%Y-%m-%d", localtime(&dd));
            dd = (time_t) time->date_stop;
            strftime(buff_stop, 20, "%Y-%m-%d", localtime(&dd));
            printf(" Date from %s to %s ",buff_start,buff_stop);
        }
#endif
	divide_time(time->time_start, &hour_start, &minute_start);
	divide_time(time->time_stop, &hour_stop, &minute_stop);
	printf(" TIME from %d:%d to %d:%d on ",
	       hour_start, minute_start,
	       hour_stop, minute_stop);
	print_days(time->days_match);
	printf(" ");
}

#if 0
/* Saves the data in parsable form to stdout. */
static void
save(const struct ipt_ip *ip, const struct ebt_entry_match *match)
{
	struct ebt_time_info *time = ((struct ebt_time_info *)match->data);
	int hour_start, hour_stop, minute_start, minute_stop;

	divide_time(time->time_start, &hour_start, &minute_start);
	divide_time(time->time_stop, &hour_stop, &minute_stop);
	printf(" --timestart %.2d:%.2d --timestop %.2d:%.2d --days ",
	       hour_start, minute_start,
	       hour_stop, minute_stop);
	print_days(time->days_match);
	printf(" ");
}
#endif

static int 
compare(const struct ebt_entry_match *m1, const struct ebt_entry_match *m2)
{
        struct ebt_time_info *timeinfo1 = (struct ebt_time_info *)m1->data;
        struct ebt_time_info *timeinfo2 = (struct ebt_time_info *)m2->data;

        if (timeinfo1->days_match != timeinfo2->days_match)
                return 0;
        if (timeinfo1->time_start != timeinfo2->time_start)
                return 0;
        if (timeinfo1->time_stop != timeinfo2->time_stop)
                return 0;
#if defined(GPL_CODE_SUPPORT_EBTABLES_DATE_MATCH)
        if (timeinfo1->date_start != timeinfo2->date_start)
                return 0;
        if (timeinfo1->date_stop != timeinfo2->date_stop)
                return 0;
#endif
        return 1;
}

static struct ebt_u_match time_match =
{
   .name          = "time",
   .size          = sizeof(struct ebt_time_info),
   .help          = help,
   .init          = init,
   .parse         = parse,
   .final_check   = final_check,
   .print         = print,
   .compare       = compare,
   .extra_ops     = opts,
};

void _init(void)
{
	ebt_register_match(&time_match);
}
