/*
 * Copyright (c) 2004, 2005 Roman Bogorodskiy (bogorodskiy@inbox.ru)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software. 
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 *
 * $Id: bmp-htmlplaylist.c,v 1.10 2005/03/02 19:23:39 bogorodskiy Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <bmp/beepctrl.h>

#define HOMEPAGE "http://bmp-plugins.berlios.de/misc/bmp-htmlplaylist/"

short int xml_output;
static void html_header(void);
static char *format_time(int time);
static void show_current_song(int session);
static void version(void);
static void usage(void);
static void help(void);

static void version()
{
	
	(void)printf("bmp-htmlplaylist %s (c) 2004 Roman Bogorodskiy\n%s\n\n", 
		     VERSION, COMPILEINFO);
	exit(0);
}

static void usage()
{
	
	(void)printf("usage: [-ax] [ -c ] [-s session]\n");
	exit(1);
}

static void help()
{
 
	(void)printf("bmp-htmlplaylist %s (c) 2004 Roman Bogorodskiy\n\n", VERSION);
	(void)printf("arguments:\n");
	(void)printf(" -a\tdon't show banner\n");
	(void)printf(" -c\tshow current song and exit\n");
	(void)printf(" -h\tshow this message\n");
	(void)printf(" -s\tselect bmp session (default: 0)\n");
	(void)printf(" -x\tgenerate xml output\n");
	(void)printf(" -v\tshow version and exit\n\n");
	exit(0);
}

static void html_header()
{
	
	if (!xml_output) {
		(void)printf("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n");
		(void)printf("<html>\n\t<head>\n");
		(void)printf("\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n");
		(void)printf("\t</head>\n<body>\n");
	} else {
		(void)printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n");
		(void)printf("<playlist>\n");
	}
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

static void show_current_song(int session)
{
	int pos;
	char *title;

	pos = xmms_remote_get_playlist_pos(session);
	if (!(title = xmms_remote_get_playlist_title(session, pos))) {
		(void)fprintf(stderr, "Error: BMP doesn't seem to be running\n");
		exit(1);
	} else
		(void)printf("%s\n", title);
			
	exit(0);	
}

int main(int argc, char **argv) 
{
	int length,
	    i,
	    session,
	    time;
	int ch;
	int show_banner;
	time_t time_total;
	char *song;
		
	time_total = 0;	
	session = 0;
	show_banner = 1;
	xml_output = 0;
	
	while ((ch = getopt(argc, argv, "achs:vx")) != -1) {
		switch (ch) {
			case 'a':
				show_banner = 0;
				break;
			case 'c':
				show_current_song(session);
				break;
			case 'h':
				(void)help();
				break;
			case 's':
				session = atoi(optarg);
				break;
			case 'v':
				(void)version();
				break;
			case 'x':
				xml_output = 1;
				break;
			default:
				(void)usage();	
		}
	}

     	argc -= optind;
	argv += optind;
		
	length = xmms_remote_get_playlist_length(session);
	
	if (length == 0) {
		(void)fprintf(stderr, "Session %d: BMP is not running or playlist is empty\n", session);
		return 1;
	}

	html_header();
	
	for (i = 0; i < length; i++) {
		song = xmms_remote_get_playlist_title(session, i);
		
		if (!song)
			return 0;
		
		time = xmms_remote_get_playlist_time(session, i)/1000;
		time_total += time;
	
		if (!xml_output) {	
			(void)printf("\t%s (%s)<br />\n", song, format_time(time));
		} else {
			(void)printf("\t<song>\n\t\t<title>%s</title>\n\t\t<time>%s</time>\n\t</song>\n",
				     song, format_time(time));
		}
	}

	if (!xml_output) {
		(void)printf("\n\t<hr size=1 width=100%%><br />\n");
		(void)printf("\n\t<b>Total:</b><br />\n\tTracks: <b>%d</b> (mean track length: %s)<br />\n", 
			     length, format_time((int)(time_total/length)));
		(void)printf("\tTime: %s<br />\n", format_time(time_total));

		if (show_banner) {
			(void)printf("\t<br />\n\tCreated with \
<a href=\"%s\">bmp-htmlplaylist %s</a>\n", HOMEPAGE, VERSION);
		}
		(void)printf("\n\t</body>\n</html>\n\n");
	} else {
		(void)printf("</playlist>\n");
	}

	return 0;
}
	
