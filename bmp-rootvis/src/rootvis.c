#include <string.h>
#include <math.h>
#include <pthread.h>

#include "rootvis.h"

extern Window ToonGetRootWindow(Display*, int, Window*);
extern void config_read(void);
extern void config_write(void);

// Forward declarations
static void rootvis_init(void);
static void rootvis_cleanup(void);
static void rootvis_about(void);
static void rootvis_configure(void);
static void rootvis_playback_start(void);
static void rootvis_playback_stop(void);
static void rootvis_render_freq(gint16 freq_data[2][256]);

// Callback functions
VisPlugin rootvis_vtable = {
	0, // Handle, filled in by xmms
	0, // Filename, filled in by xmms

	0,                     // Session ID
	"Root Spectrum Analyzer 0.0.7",  // description

	0, // # of PCM channels for render_pcm()
	2, // # of freq channels wanted for render_freq()

	rootvis_init,           // Called when plugin is enabled
	rootvis_cleanup,        // Called when plugin is disabled
	NULL,//rootvis_about,          // Show the about box
	rootvis_configure,      // Show the configure box
	0,                     // Called to disable plugin, filled in by xmms
	rootvis_playback_start, // Called when playback starts
	rootvis_playback_stop,  // Called when playback stops
	0,                     // Render the PCM data, must return quickly
	rootvis_render_freq     // Render the freq data, must return quickly
};

// XMMS entry point
VisPlugin *get_vplugin_info(void) {
	return &rootvis_vtable;
}

// X related
struct rootvis_x {
	int screen;
	Display *display;
	Window rootWin, Parent;
	GC gc;
};

// thread talk

struct rootvis_threads {
	gint16 freq_data[2][256];
	pthread_t worker[2];
	pthread_mutex_t mutex1;
	enum {GO, STOP} control;
	char dirty;
	/*** dirty flaglist ***
	  1: channel 1 geometry change
	  2: channel 1 color change
	  4: channel 2 geometry change
	  8: channel 2 color change
	 16: no data yet (don't do anything)
	*/
} threads;

// Some helper stuff

void clean_data(void) {
	pthread_mutex_lock(&threads.mutex1);
	memset(threads.freq_data, 0, sizeof(gint16) * 2 * 256);
	pthread_mutex_unlock(&threads.mutex1);
}

void print_status(char msg[]) {
	if (conf.debug == 1) printf(">> rootvis >> %s\n", msg); // for debug purposes, but doesn't tell much anyway
}

void error_exit(char msg[]) {
	printf("*** ERROR (rootvis): %s\n", msg);
	rootvis_vtable.disable_plugin(&rootvis_vtable);
}

void initialize_X(struct rootvis_x* drw, char* display) {
	print_status("Opening X Display");
	drw->display = XOpenDisplay(display);
	if (drw->display == NULL) {
		fprintf(stderr, "cannot connect to X server %s\n",
			getenv("DISPLAY") ? getenv("DISPLAY") : "(default)");
		error_exit("Connecting to X server failed");
		pthread_exit(NULL);
	}
	print_status("Getting screen and window");
	drw->screen = DefaultScreen(drw->display);
	drw->rootWin = ToonGetRootWindow(drw->display, drw->screen, &drw->Parent);

	print_status("Creating Graphical Context");
	drw->gc = XCreateGC(drw->display, drw->rootWin, 0, NULL);
	if (drw->gc < 0) {
		error_exit("Creating Graphical Context failed");
		pthread_exit(NULL);
	}
	print_status("Setting Line Attributes");
	XSetLineAttributes(drw->display, drw->gc, 1, LineSolid, CapButt, JoinBevel);
}

long get_color(struct rootvis_x* drw, char col[3]) {
	Colormap def_colormap;
 	XColor dacolor;
	dacolor.red = col[RED] * 256;
	dacolor.green = col[GREEN] * 256;
	dacolor.blue = col[BLUE] * 256;
	print_status("Getting Colormap");
	def_colormap = DefaultColormap(drw->display, DefaultScreen(drw->display));
	print_status("Allocationg color");
	if (XAllocColor(drw->display, def_colormap, &dacolor) == 0) {
		error_exit("Color allocation failed");
		pthread_exit(NULL);
	}
	return dacolor.pixel;
}

void damage_clear(struct rootvis_x* drw, unsigned short damage_coords[4]) {
	XClearArea(drw->display, drw->rootWin, damage_coords[0], damage_coords[1], damage_coords[2], damage_coords[3], True);
}

void draw_bar(struct rootvis_x* drw, int t, int i, int color, int peakcolor, int shcolor, unsigned short level, unsigned short oldlevel, unsigned short peak, unsigned short oldpeak) {
	
	/* to make following cleaner, we work with redundant helper variables
	   this also avoids some calculations */
	register int a, b, c, d;
	int drawpeak = 0, shheight, shoffset = 0;
	
	if (level < conf.bar[t].shadow) { shheight = level; shoffset = conf.bar[t].shadow - level; }
	else shheight = conf.bar[t].shadow;
	
	if ((conf.peak[t].enabled)&&(peak != oldpeak)) drawpeak = 1;
	
	if (conf.geo[t].orientation < 2) {
		a = conf.geo[t].posx + i*(conf.bar[t].width+conf.bar[t].shadow+conf.geo[t].space);
		c = conf.bar[t].width;
	} else {
		b = conf.geo[t].posy + (conf.data[t].cutoff/conf.data[t].div - i - 1)
			*(conf.bar[t].width+conf.bar[t].shadow+conf.geo[t].space);
		d = conf.bar[t].width;
	}
	
	if (level > oldlevel) {
		if (conf.geo[t].orientation == 0) {	b = conf.geo[t].posy + conf.geo[t].height - level; d = level; }
		else if (conf.geo[t].orientation == 1) { b = conf.geo[t].posy; d = level; }
		else if (conf.geo[t].orientation == 2) { a = conf.geo[t].posx; c = level; }
		else	{ a = conf.geo[t].posx + conf.geo[t].height - level; c = level; }
		XSetForeground(drw->display, drw->gc, color);
		XFillRectangle(drw->display, drw->rootWin, drw->gc, a, b, c, d);
		if (conf.bar[t].shadow) {
			XSetForeground(drw->display, drw->gc, shcolor);
			if (conf.geo[t].orientation < 2) {
				XFillRectangle(drw->display, drw->rootWin, drw->gc,
					a+conf.bar[t].width, b+conf.bar[t].shadow, conf.bar[t].shadow, d);
				XFillRectangle(drw->display, drw->rootWin, drw->gc,
					a+conf.bar[t].shadow, b + level + shoffset, conf.bar[t].width, shheight);
			} else {
				XFillRectangle(drw->display, drw->rootWin, drw->gc,
					a+conf.bar[t].shadow, b+conf.bar[t].width, c, conf.bar[t].shadow);
				XFillRectangle(drw->display, drw->rootWin, drw->gc,
					a + level + shoffset, b+conf.bar[t].shadow, shheight, conf.bar[t].width);
			}
		}
	}
	if ((level < oldlevel)||((drawpeak)&&(peak < oldpeak))) {
		if (conf.geo[t].orientation == 0) { b = conf.geo[t].posy; d = conf.geo[t].height - level; }
		else if (conf.geo[t].orientation == 1) { b = conf.geo[t].posy + level;
							d = conf.geo[t].height - level + conf.bar[t].shadow; }
		else if (conf.geo[t].orientation == 2) { a = conf.geo[t].posx + level;
							c = conf.geo[t].height - level + conf.bar[t].shadow; }
		else	{ a = conf.geo[t].posx; c = conf.geo[t].height - level; }
		XClearArea(drw->display, drw->rootWin, a, b, c, d, False);

		if (conf.bar[t].shadow) {
			if (conf.geo[t].orientation == 0) {
				XClearArea(drw->display, drw->rootWin,
					a + conf.bar[t].width, b + conf.bar[t].shadow, conf.bar[t].shadow, d, False);
				if (level < conf.bar[t].shadow) {
					XClearArea(drw->display, drw->rootWin,
						a + conf.bar[t].shadow, b + conf.geo[t].height,
						conf.bar[t].width, conf.bar[t].shadow - level, False);
				}
			}
			if (conf.geo[t].orientation == 1) {
				if (level > conf.bar[t].shadow) {
					XClearArea(drw->display, drw->rootWin,
						a + conf.bar[t].width, b + conf.bar[t].shadow, conf.bar[t].shadow, d, False);
					b = conf.geo[t].posy + level;
					d = conf.bar[t].shadow;
				}
				else {
					XClearArea(drw->display, drw->rootWin,
						a + conf.bar[t].width, conf.geo[t].posy,
						conf.bar[t].shadow, conf.geo[t].height, False);
					b = conf.geo[t].posy + conf.bar[t].shadow - 1;
					d = level;
				}
				XSetForeground(drw->display, drw->gc, shcolor);
				XFillRectangle(drw->display, drw->rootWin, drw->gc, a+conf.bar[t].shadow, b, c, d);
			}
			if (conf.geo[t].orientation == 2) {
				if (level > conf.bar[t].shadow) {
					XClearArea(drw->display, drw->rootWin,
						a + conf.bar[t].shadow, b + conf.bar[t].width, c, conf.bar[t].shadow, False);
					a = conf.geo[t].posx + level;
					c = conf.bar[t].shadow;
				}
				else {
					XClearArea(drw->display, drw->rootWin,
						conf.geo[t].posx, b + conf.bar[t].width,
						conf.geo[t].height, conf.bar[t].shadow, False);
					a = conf.geo[t].posx + conf.bar[t].shadow - 1;
					c = level;
				}
				XSetForeground(drw->display, drw->gc, shcolor);
				XFillRectangle(drw->display, drw->rootWin, drw->gc, a, b+conf.bar[t].shadow, c, d);
			}
			if (conf.geo[t].orientation == 3) {
				XClearArea(drw->display, drw->rootWin,
					a + conf.bar[t].shadow, b + conf.bar[t].width, c, conf.bar[t].shadow, False);
				if (level < conf.bar[t].shadow) {
					XClearArea(drw->display, drw->rootWin,
						a + conf.geo[t].height, b + conf.bar[t].shadow,
						conf.bar[t].shadow - level, conf.bar[t].width, False);
				}
			}
		}
	}
	if ((peak > 0)&&((conf.geo[t].orientation == 1)||(conf.geo[t].orientation == 2)||(drawpeak))) {
		XSetForeground(drw->display, drw->gc, peakcolor);
		if (conf.geo[t].orientation == 0) {	b = conf.geo[t].posy + conf.geo[t].height - peak; d = 1; }
		else if (conf.geo[t].orientation == 1) { b = conf.geo[t].posy + peak - 1; d = 1; }
		else if (conf.geo[t].orientation == 2) { a = conf.geo[t].posx + peak - 1; c = 1; }
		else	{ a = conf.geo[t].posx + conf.geo[t].height - peak; c = 1; }
		XFillRectangle(drw->display, drw->rootWin, drw->gc, a, b, c, d);
	}
}

// Our worker thread

void* worker_func(void* threadnump) {
	struct rootvis_x draw;
	long pixel[6];
	gint16 freq_data[256];
	double scale, x00, y00;
	unsigned int threadnum, i, j, level;
	unsigned short damage_coords[4];
	unsigned short *level1 = NULL, *level2 = NULL, *levelsw, *peak1 = NULL, *peak2 = NULL, *peakstep;

	if (threadnump == NULL) threadnum = 0; else threadnum = 1;
	
	print_status("Memory allocations");
	level1 = (unsigned short*)calloc(256, sizeof(short)); // need to be zeroed out
	level2 = (unsigned short*)malloc(256*sizeof(short));
	peak1 = (unsigned short*)calloc(256, sizeof(short)); // need to be zeroed out
	peak2 = (unsigned short*)calloc(256, sizeof(short)); // need to be zeroed out for disabled peaks
	peakstep = (unsigned short*)calloc(256, sizeof(short)); // need to be zeroed out
	if ((level1 == NULL)||(level2 == NULL)||(peak1 == NULL)||(peak2 == NULL)||(peakstep == NULL)) {
		error_exit("Allocation of memory failed");
		pthread_exit(NULL);
	}
	print_status("Allocations done");

	draw.display = NULL;
	
	while (threads.control != STOP) {

	  /* we will unset our own dirty flags after receiving them */
	  pthread_mutex_lock(&threads.mutex1);
	  memcpy(&freq_data, &threads.freq_data[threadnum], sizeof(gint16)*256);
	  i = threads.dirty;
	  if ((i & 16) == 0) threads.dirty = i & (~(3 + threadnum*9));
	  pthread_mutex_unlock(&threads.mutex1);

	  if ((i & 16) == 0) { // we've gotten data
		if (draw.display == NULL)	initialize_X(&draw, conf.geo[threadnum].display);
		else if (i & (1 + threadnum*3)) damage_clear(&draw, damage_coords);
				
		if (i & (1 + threadnum*3)) {	// geometry has changed
			damage_coords[0] = conf.geo[threadnum].posx;
			damage_coords[1] = conf.geo[threadnum].posy;
			if (conf.geo[threadnum].orientation < 2) {
				damage_coords[2] = conf.data[threadnum].cutoff/conf.data[threadnum].div
					*(conf.bar[threadnum].width + conf.bar[threadnum].shadow + conf.geo[threadnum].space);
				damage_coords[3] = conf.geo[threadnum].height + conf.bar[threadnum].shadow;
			} else {
				damage_coords[2] = conf.geo[threadnum].height + conf.bar[threadnum].shadow;
				damage_coords[3] = conf.data[threadnum].cutoff/conf.data[threadnum].div
					*(conf.bar[threadnum].width + conf.bar[threadnum].shadow + conf.geo[threadnum].space);
			}
			print_status("Geometry recalculations");
			scale = conf.geo[threadnum].height /
				(log((1 - conf.data[threadnum].linearity) / conf.data[threadnum].linearity) * 4);
			x00 = conf.data[threadnum].linearity*conf.data[threadnum].linearity*32768.0 /
				(2*conf.data[threadnum].linearity - 1);
			y00 = -log(-x00) * scale;
			memset(level1, 0, 256*sizeof(short));
			memset(peak1, 0, 256*sizeof(short));
			memset(peak2, 0, 256*sizeof(short));
		}
		if (i & (2 + threadnum*6)) {	// colors have changed
			pixel[0] = get_color(&draw, conf.bar[threadnum].color[0]);
			pixel[1] = get_color(&draw, conf.bar[threadnum].color[1]);
			pixel[2] = get_color(&draw, conf.bar[threadnum].color[2]);
			pixel[3] = get_color(&draw, conf.bar[threadnum].color[3]);
			pixel[4] = get_color(&draw, conf.peak[threadnum].color);
			pixel[5] = get_color(&draw, conf.bar[threadnum].shadow_color);
		}

		/* instead of copying the old level array to the second array, 
		we just tell the first is now the second one */
		levelsw = level1;
		level1 = level2;
		level2 = levelsw;
		levelsw = peak1;
		peak1 = peak2;
		peak2 = levelsw;
		
		for (i = 0; i < conf.data[threadnum].cutoff/conf.data[threadnum].div; i++) {
			level = 0;
			for (j = i*conf.data[threadnum].div; j < (i+1)*conf.data[threadnum].div; j++)
				if (level < freq_data[j])
					level = freq_data[j];
			level = level * (i*conf.data[threadnum].div + 1);
			level = floor(abs(log(level - x00)*scale + y00));
			if (level < conf.geo[threadnum].height) {
				if ((level2[i] > conf.bar[threadnum].falloff)&&(level < level2[i] - conf.bar[threadnum].falloff))
					level1[i] = level2[i] - conf.bar[threadnum].falloff;
				else	level1[i] = level;
			} else level1[i] = conf.geo[threadnum].height;
			if (conf.peak[threadnum].enabled) {
				if (level1[i] > peak2[i] - conf.peak[threadnum].falloff) {
					peak1[i] = level1[i];
					peakstep[i] = 0;
				} else if (peakstep[i] == conf.peak[threadnum].step)
					if (peak2[i] > conf.peak[threadnum].falloff)
						peak1[i] = peak2[i] - conf.peak[threadnum].falloff;
					else peak1[i] = 0;
				else {
					peak1[i] = peak2[i];
					peakstep[i]++;
				}
			}
			draw_bar(&draw, threadnum, i, pixel[(int)ceil((float)level1[i] / conf.geo[threadnum].height * 4) - 1], pixel[4], pixel[5], level1[i], level2[i], peak1[i], peak2[i]);
		}
		XFlush(draw.display);
	  }
	  xmms_usleep(1000000 / conf.data[threadnum].fps);
	}
	print_status("Worker thread: Exiting");
	if (draw.display != NULL) {
		damage_clear(&draw, damage_coords);
		XCloseDisplay(draw.display); 
	}
	free(level1);	free(level2);	free(peak1);	free(peak2);	free(peakstep);
	return NULL;
}


// da xmms functions

static void rootvis_init(void) {
	int rc1;
	print_status("Initializing");
	pthread_mutex_init(&threads.mutex1, NULL);
	threads.control = GO;
	clean_data();
	conf.geo[0].display = malloc(256);
	conf.geo[1].display = malloc(256);
	config_read();
	threads.dirty = 31;	// this means simply everything has changed and there was no data
	if ((rc1 = pthread_create(&threads.worker[0], NULL, worker_func, NULL))) {
		fprintf(stderr, "Thread creation failed: %d\n", rc1);
		error_exit("Thread creation failed");
	}
	if ((conf.stereo)&&(rc1 = pthread_create(&threads.worker[1], NULL, worker_func, &rc1))) {
		fprintf(stderr, "Thread creation failed: %d\n", rc1);
		error_exit("Thread creation failed");
	}
}

static void rootvis_cleanup(void) {
	print_status("Cleanup... ");
	threads.control = STOP;
	pthread_join(threads.worker[0], NULL);
	if (conf.stereo)	pthread_join(threads.worker[1], NULL);
	print_status("Clean Exit");
}

static void rootvis_about(void)
{
	print_status("About");
}

static void rootvis_configure(void)
{
	print_status("Configuration trigger");
	pthread_mutex_lock(&threads.mutex1);
	/* as the configs aren't saved in a thread save way, we have to lock while we read them
	 * in future versions we will have to make config reading/writing thread save (buffering configs probably)
	 */
	config_read();
	threads.dirty = 15;	// this means simply everything has changed
	pthread_mutex_unlock(&threads.mutex1);
}

static void rootvis_playback_start(void)
{
	print_status("Playback starting");
}

static void rootvis_playback_stop(void)
{
	clean_data();
}

static void rootvis_render_freq(gint16 freq_data[2][256]) {
	int channel, bucket;
	pthread_mutex_lock(&threads.mutex1);
	threads.dirty = threads.dirty & (~(16)); // unset no data yet flag
	for (channel = 0; channel < 2; channel++) {
	 for (bucket = 0; bucket < 256; bucket++) {
		if (conf.stereo) threads.freq_data[channel][bucket] = freq_data[channel][bucket];
		else if (channel == 0) threads.freq_data[0][bucket] = freq_data[channel][bucket] / 2;
			else	threads.freq_data[0][bucket] += freq_data[channel][bucket] / 2;
	 }
	}
	pthread_mutex_unlock(&threads.mutex1);
}
