#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <bmp/beepctrl.h>

#define FMT_PLAIN 0
#define FMT_HTML 1
#define FMT_XML 2

static void html_header(void);
static char *format_time(int time);

static void html_header()
{
	(void)printf("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n");
	(void)printf("<html>\n\t<head>\n");
	(void)printf("\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n");
	(void)printf("\t</head>\n<body>\n");
}

static char *format_time(int time)
{
	int days,
	    hours,
	    minutes,
	    seconds;
	div_t dv;
	char *string;
	
	string = (char *)malloc(256);
	
	seconds = time;
	
	dv = div(seconds, 86400);

	if (dv.quot > 0) {
		days = dv.quot;
		seconds -= dv.quot * 86400;
	} else
		days = 0;

	dv = div(seconds, 3600);

	if (dv.quot > 0) {
		hours = dv.quot;
		seconds -= dv.quot * 3600;
	} else 
		hours = 0;


	dv = div(seconds, 60);
	
	if (dv.quot > 0) {
		minutes = dv.quot;
		seconds -= dv.quot * 60;
	} else
		minutes = 0;

	if (days == 0 && hours == 0) 
		(void)snprintf(string, 255, "%-.2d:%-.2d:%-.2d", hours, minutes, seconds);
	else
		(void)snprintf(string, 255, "%d days %d hours %d minutes %d seconds", 
			       days, hours, minutes, seconds);
	return string;
}

int main(int argc, char **argv) 
{
	int length,
	    i,
	    session,
	    time;
	time_t time_total;
	char *song;
		
	time_total = 0;	
	/* XXX should ask user about that */
	session = 0;
	
	length = xmms_remote_get_playlist_length(session);
	
	if (length == 0)
		return 0;

	html_header();
	
	for (i = 0; i < length; i++) {
		song = xmms_remote_get_playlist_title(session, i);
		
		
		if (!song)
			return 0;
		
		time = xmms_remote_get_playlist_time(session, i)/1000;
		time_total += time;
		
		(void)printf("\t%s (%s)<br />\n", song, format_time(time));
	}

	(void)printf("\n\t<hr size=1 width=100%%><br />\n");
	(void)printf("\n\t<b>Total:</b><br />\n\tTracks: <b>%d</b><br />\n", length);
	(void)printf("\tTime: %s<br />\n", format_time(time_total));
	(void)printf("\n\t</body>\n</html>\n\n");

	return 0;
}
	
