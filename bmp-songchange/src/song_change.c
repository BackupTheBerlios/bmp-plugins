#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <signal.h>
#include <unistd.h>
#include <stdio.h>

#include <string.h>

#include <bmp/plugin.h>
#include <bmp/configdb.h>
#include <bmp/beepctrl.h>
#include <bmp/formatter.h>

static void about(void);
static void init(void);
static void cleanup(void);
static void configure(void);
static void show_format_info(void);
static gint timeout_func(gpointer);
static gint timeout_tag = 0;
static gint previous_song = -1;
static gchar *cmd_line = NULL;
static gboolean possible_pl_end;
static gchar *cmd_line_end;

static GtkWidget *configure_win = NULL, *configure_vbox;
static GtkWidget *cmd_entry, *cmd_end_entry;

GeneralPlugin sc_gp =
{
	NULL,			/* handle */
	NULL,			/* filename */
	-1,			/* xmms_session */
	NULL,			/* Description */
	init,
	about,
	configure,
	cleanup,
};

GeneralPlugin *get_gplugin_info(void)
{
	sc_gp.description = g_strdup_printf("Song Change %s", VERSION);
	return &sc_gp;
}


static void read_config(void)
{
	ConfigDb *cfg;

	g_free(cmd_line);
	g_free(cmd_line_end);
	cmd_line = NULL;
	cmd_line_end = NULL;

	if ((cfg = bmp_cfg_db_open()) != NULL)
	{
		bmp_cfg_db_get_string(cfg, "song_change", "cmd_line", &cmd_line);
		bmp_cfg_db_get_string(cfg, "song_change", "cmd_line_end", &cmd_line_end);
		bmp_cfg_db_close(cfg);
	}
}

static void about(void)
{
	static GtkWidget *aboutbox;
	gchar *text;

	if (aboutbox)
		return;

	text = g_strdup_printf("BMP Songchange Plugin %s\n\n"
			"Roman Bogorodskiy <bogorodskiy@inbox.ru>\n"
			"Based on XMMS song_change plugin\n",
		       VERSION);


	aboutbox = xmms_show_message("About BMP Songchange", text, "OK", 
			FALSE, NULL, NULL);

	g_free(text);
	gtk_signal_connect(GTK_OBJECT(aboutbox), "destroy",
			GTK_SIGNAL_FUNC(gtk_widget_destroyed), &aboutbox);
}

static void init(void)
{
	read_config();

	previous_song = -1;
	timeout_tag = gtk_timeout_add(100, timeout_func, NULL);
}


static void cleanup(void)
{
	if (timeout_tag)
		gtk_timeout_remove(timeout_tag);
	timeout_tag = 0;
	if (cmd_line)
		g_free(cmd_line);
	if (cmd_line_end)
		g_free(cmd_line_end);
	cmd_line = NULL;
	cmd_line_end = NULL;
	signal(SIGCHLD, SIG_DFL);
}


static void save_and_close(GtkWidget *w, gpointer data)
{
	char *cmd, *cmd_end;
	ConfigDb *cfg = bmp_cfg_db_open();

	cmd = gtk_entry_get_text(GTK_ENTRY(cmd_entry));
	cmd_end = gtk_entry_get_text(GTK_ENTRY(cmd_end_entry));

	bmp_cfg_db_set_string(cfg, "song_change", "cmd_line", cmd);
	bmp_cfg_db_set_string(cfg, "song_change", "cmd_line_end", cmd_end);
	bmp_cfg_db_close(cfg);

	if (timeout_tag)
	{
		g_free(cmd_line);
		cmd_line = g_strdup(cmd);
		g_free(cmd_line_end);
		cmd_line_end = g_strdup(cmd_end);
	}
	gtk_widget_destroy(configure_win);
}



static void warn_user(void)
{
	GtkWidget *warn_win, *warn_vbox, *warn_desc;
	GtkWidget *warn_bbox, *warn_yes, *warn_no;

	warn_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint(GTK_WINDOW(warn_win), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_title(GTK_WINDOW(warn_win), "Warning");
	gtk_window_set_transient_for(GTK_WINDOW(warn_win),
				     GTK_WINDOW(configure_win));
	gtk_window_set_modal(GTK_WINDOW(warn_win), TRUE);
	
	gtk_container_set_border_width(GTK_CONTAINER(warn_win), 10);

	warn_vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(warn_win), warn_vbox);

	warn_desc = gtk_label_new(
		"Filename and song title tags should be inside "
		"double quotes (\").  Not doing so might be a "
		"security risk.  Continue anyway?");
	gtk_label_set_justify(GTK_LABEL(warn_desc), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(warn_desc), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(warn_vbox), warn_desc, FALSE, FALSE, 0);
	gtk_label_set_line_wrap(GTK_LABEL(warn_desc), TRUE);
					  
	warn_bbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(warn_bbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(warn_bbox), 5);
	gtk_box_pack_start(GTK_BOX(warn_vbox), warn_bbox, FALSE, FALSE, 0);

	warn_yes = gtk_button_new_with_label("Yes");
	gtk_signal_connect(GTK_OBJECT(warn_yes), "clicked",
			   GTK_SIGNAL_FUNC(save_and_close), NULL);
	gtk_signal_connect_object(GTK_OBJECT(warn_yes), "clicked",
				  GTK_SIGNAL_FUNC(gtk_widget_destroy),
				  GTK_OBJECT(warn_win));
	GTK_WIDGET_SET_FLAGS(warn_yes, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(warn_bbox), warn_yes, TRUE, TRUE, 0);
	gtk_widget_grab_default(warn_yes);

	warn_no = gtk_button_new_with_label("No");
	gtk_signal_connect_object(GTK_OBJECT(warn_no), "clicked",
				  GTK_SIGNAL_FUNC(gtk_widget_destroy),
				  GTK_OBJECT(warn_win));
	GTK_WIDGET_SET_FLAGS(warn_no, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(warn_bbox), warn_no, TRUE, TRUE, 0);

	gtk_widget_show_all(warn_win);
}

static int check_command(char *command)
{
	const char *dangerous = "fns";
	char *c;
	int qu = 0;
	
	for (c = command; *c != '\0'; c++)
	{
		if (*c == '"' && (c == command || *(c - 1) != '\\'))
			qu = !qu;
		else if (*c == '%' && !qu && strchr(dangerous, *(c + 1)))
			return -1;
	}
	return 0;
}



static void configure_ok_cb(GtkWidget *w, gpointer data)
{
	char *cmd, *cmd_end;

	cmd = gtk_entry_get_text(GTK_ENTRY(cmd_entry));
	cmd_end = gtk_entry_get_text(GTK_ENTRY(cmd_end_entry));

#ifdef DEBUG
	(void)printf("cmd: %s\ncmd_end: %s\n", cmd, cmd_end);
#endif
	
	if (check_command(cmd) < 0 || check_command(cmd_end) < 0)
		warn_user();
	else
		save_and_close(NULL, NULL);
}


static void configure(void)
{
	GtkWidget *cmd_hbox, *cmd_label, *cmd_end_hbox, *cmd_end_label;
	GtkWidget *song_desc, *end_desc;
	GtkWidget *configure_bbox, *configure_ok, *configure_cancel;
	GtkWidget *song_frame, *song_vbox, *end_frame, *end_vbox;
	GtkWidget *cmd_help_button;
	gchar *temp;
	GtkTooltips *format_descr_tip;

	if (configure_win)
		return;

	read_config();

	format_descr_tip = gtk_tooltips_new();

	configure_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint(GTK_WINDOW(configure_win), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_signal_connect(GTK_OBJECT(configure_win), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &configure_win);
	gtk_window_set_title(GTK_WINDOW(configure_win), "Song Change Configuration");
	
	gtk_container_set_border_width(GTK_CONTAINER(configure_win), 10);

	configure_vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(configure_win), configure_vbox);

	song_frame = gtk_frame_new("Song change");
	gtk_box_pack_start(GTK_BOX(configure_vbox), song_frame, FALSE, FALSE, 0);
	song_vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_set_border_width(GTK_CONTAINER(song_vbox), 5);
	gtk_container_add(GTK_CONTAINER(song_frame), song_vbox);
	
	end_frame = gtk_frame_new("Playlist end");
	gtk_box_pack_start(GTK_BOX(configure_vbox), end_frame, FALSE, FALSE, 0);
	end_vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_set_border_width(GTK_CONTAINER(end_vbox), 5);
	gtk_container_add(GTK_CONTAINER(end_frame), end_vbox);
	
	temp = g_strdup_printf(
		"Shell-command to run when BMP changes song.  "
		"It can optionally include the string %%s which will be "
		"replaced by the new song title (press the \"?\" button for details).");
	song_desc = gtk_label_new(temp);
	g_free(temp);
	gtk_label_set_justify(GTK_LABEL(song_desc), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(song_desc), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(song_vbox), song_desc, FALSE, FALSE, 0);
	gtk_label_set_line_wrap(GTK_LABEL(song_desc), TRUE);
	
	cmd_hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(song_vbox), cmd_hbox, FALSE, FALSE, 0);

	cmd_label = gtk_label_new("Command:");
	gtk_box_pack_start(GTK_BOX(cmd_hbox), cmd_label, FALSE, FALSE, 0);

	cmd_entry = gtk_entry_new();
	if (cmd_line)
		gtk_entry_set_text(GTK_ENTRY(cmd_entry), cmd_line);
	gtk_widget_set_usize(cmd_entry, 200, -1);
	gtk_box_pack_start(GTK_BOX(cmd_hbox), cmd_entry, TRUE, TRUE, 0);

	cmd_help_button = gtk_button_new_with_label("?");
	gtk_box_pack_start(GTK_BOX(cmd_hbox), cmd_help_button, FALSE, FALSE, 0);
	
	end_desc = gtk_label_new(
		"Shell-command to run when BMP reaches the end "
		"of the playlist.");
	gtk_label_set_justify(GTK_LABEL(end_desc), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(end_desc), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(end_vbox), end_desc, FALSE, FALSE, 0);
	gtk_label_set_line_wrap(GTK_LABEL(end_desc), TRUE);
					  
	cmd_end_hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(end_vbox), cmd_end_hbox, FALSE, FALSE, 0);

	cmd_end_label = gtk_label_new("Command:");
	gtk_box_pack_start(GTK_BOX(cmd_end_hbox), cmd_end_label, FALSE, FALSE, 0);

	cmd_end_entry = gtk_entry_new();
	if (cmd_line_end)
		gtk_entry_set_text(GTK_ENTRY(cmd_end_entry), cmd_line_end);
	gtk_widget_set_usize(cmd_end_entry, 200, -1);
	gtk_box_pack_start(GTK_BOX(cmd_end_hbox), cmd_end_entry, TRUE, TRUE, 0);

	configure_bbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(configure_bbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(configure_bbox), 5);
	gtk_box_pack_start(GTK_BOX(configure_vbox), configure_bbox, FALSE, FALSE, 0);

	configure_ok = gtk_button_new_with_label("Ok");
	gtk_signal_connect(GTK_OBJECT(configure_ok), "clicked", GTK_SIGNAL_FUNC(configure_ok_cb), NULL);
	GTK_WIDGET_SET_FLAGS(configure_ok, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(configure_bbox), configure_ok, TRUE, TRUE, 0);
	gtk_widget_grab_default(configure_ok);

	configure_cancel = gtk_button_new_with_label("Cancel");
	gtk_signal_connect_object(GTK_OBJECT(configure_cancel), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(configure_win));
	GTK_WIDGET_SET_FLAGS(configure_cancel, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(configure_bbox), configure_cancel, TRUE, TRUE, 0);

	gtk_widget_show_all(configure_win);

	g_signal_connect((gpointer)cmd_help_button, "clicked",
			G_CALLBACK(show_format_info),
			NULL);
}

static void bury_child(int signal)
{
	waitpid(-1, NULL, WNOHANG);
}

static void execute_command(gchar *cmd)
{
	gchar *argv[4] = {"/bin/sh", "-c", NULL, NULL};
	gint i;
	argv[2] = cmd;
	signal(SIGCHLD, bury_child);
	if (fork() == 0)
	{
		/* We don't want this process to hog the audio device etc */
		for (i = 3; i < 255; i++)
			close(i);
		execv("/bin/sh", argv);
	}
}

/*
 * escape_shell_chars()
 *
 * Escapes characters that are special to the shell inside double quotes.
 */

static char * escape_shell_chars(char * string)
{
	const gchar *special = "$`\"\\"; /* Characters to escape */
	char *in = string, *out;
	char *escaped;
	int num = 0;

	while (*in != '\0')
		if (strchr(special, *in++))
			num++;

	escaped = g_malloc(strlen(string) + num + 1);

	in = string;
	out = escaped;

	while (*in != '\0')
	{
		if (strchr(special, *in))
			*out++ = '\\';
		*out++ = *in++;
	}
	*out = '\0';

	return escaped;
}

static gint timeout_func(gpointer data)
{
	int pos, length, rate, freq, nch;
	char *str, *shstring = NULL, *temp, numbuf[16];
	gboolean playing, run_end_cmd = FALSE;
	static char *previous_file;
	char *current_file;
	Formatter *formatter;

	GDK_THREADS_ENTER();

	playing = xmms_remote_is_playing(sc_gp.xmms_session);
	pos = xmms_remote_get_playlist_pos(sc_gp.xmms_session);
	current_file = xmms_remote_get_playlist_file(sc_gp.xmms_session, pos);
	
	/* Format codes:
	 *
	 *   F - frequency (in hertz)
	 *   c - number of channels
	 *   f - filename (full path)
	 *   l - length (in milliseconds)
	 *   n - name
	 *   r - rate (in bits per second)
	 *   s - name (since everyone's used to it)
	 *   t - playlist position (%02d)
	 */
	if (pos != previous_song ||
	    (!previous_file && current_file) ||
	    (previous_file && !current_file) ||
	    (previous_file && current_file &&
	     strcmp(previous_file, current_file)))
	{
		g_free(previous_file);
		previous_file = current_file;
		current_file = NULL;
		previous_song = pos;

		if (cmd_line && strlen(cmd_line) > 0)
		{
			formatter = xmms_formatter_new();
			str = xmms_remote_get_playlist_title(sc_gp.xmms_session, pos);
			if (str)
			{
				temp = escape_shell_chars(str);
				xmms_formatter_associate(formatter, 's', temp);
				xmms_formatter_associate(formatter, 'n', temp);
				g_free(str);
				g_free(temp);
			}
			else
			{
				xmms_formatter_associate(formatter, 's', "");
				xmms_formatter_associate(formatter, 'n', "");
			}                               

			if (previous_file)
			{
				temp = escape_shell_chars(previous_file);
				xmms_formatter_associate(formatter, 'f', temp);
				g_free(temp);
			}
			else
				xmms_formatter_associate(formatter, 'f', "");
			sprintf(numbuf, "%02d", pos + 1);
			xmms_formatter_associate(formatter, 't', numbuf);
			length = xmms_remote_get_playlist_time(sc_gp.xmms_session, pos);
			if (length != -1)
			{
				sprintf(numbuf, "%d", length);
				xmms_formatter_associate(formatter, 'l', numbuf);
			}
			else
				xmms_formatter_associate(formatter, 'l', "0");
			xmms_remote_get_info(sc_gp.xmms_session, &rate, &freq, &nch);
			sprintf(numbuf, "%d", rate);
			xmms_formatter_associate(formatter, 'r', numbuf);
			sprintf(numbuf, "%d", freq);
			xmms_formatter_associate(formatter, 'F', numbuf);
			sprintf(numbuf, "%d", nch);
			xmms_formatter_associate(formatter, 'c', numbuf);
			shstring = xmms_formatter_format(formatter, cmd_line);
			xmms_formatter_destroy(formatter);
		}
	}

	g_free(current_file);

	if (playing)
	{
		int playlist_length = xmms_remote_get_playlist_length(sc_gp.xmms_session);
		if (pos + 1 == playlist_length)
			possible_pl_end = TRUE;
		else
			possible_pl_end = FALSE;
	}
	else if (possible_pl_end)
	{
		if (pos == 0 && cmd_line_end && strlen(cmd_line_end) > 0)
			run_end_cmd = TRUE;
		possible_pl_end = FALSE;
	}

	if (shstring)
	{
		execute_command(shstring);
		g_free(shstring); /* FIXME: This can possibly be freed too early */
	}

	if (run_end_cmd)
		execute_command(cmd_line_end);

	GDK_THREADS_LEAVE();

	return TRUE;
}

static void show_format_info(void)
{
	static GtkWidget *format_info_dialog;
	gchar *text;

	if (format_info_dialog)
		return;

	text = g_strdup("Format codes:\n\n"
		"%F - frequency (in hertz)\n"
	 	"%c - number of channels\n"
		"%f - filename (full path)\n"
	 	"%l - length (in milliseconds)\n"
	 	"%n - name\n"
	 	"%r - rate (in bits per second)\n"
	 	"%s - name (an alias for %n)\n"
	 	"%t - playlist position (%02d)\n");

	format_info_dialog = xmms_show_message("Format info", text, "OK", 
			FALSE, NULL, NULL);

	g_free(text);
	gtk_signal_connect(GTK_OBJECT(format_info_dialog), "destroy",
			GTK_SIGNAL_FUNC(gtk_widget_destroyed), &format_info_dialog);
}
