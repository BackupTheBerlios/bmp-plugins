#include <rootvis.h>
#include <bmp/configdb.h>

void color_triplet2arr(char* res, char* triplet) {
	if (sscanf(triplet, "#%2hhx%2hhx%2hhx", &res[0], &res[1], &res[2]) != 3)
		fprintf(stderr, "Color value %s could not be recognized as #rrggbb, ranging from #000000 to #FFFFFF\n", triplet);
/*	if (sscanf(triplet, "(%hhd,%hhd,%hhd)", &res[0], &res[1], &res[2]) != 3)
		fprintf(stderr, "Color value %s could not be recognized in the value (x,y,z) whereby x,y,z are between 0 and 255\n", triplet); */
}

char* color_arr2triplet(unsigned char* src, char* triplet) {
	sprintf(triplet, "#%2.2hhx%2.2hhx%2.2hhx", src[0], src[1], src[2]);
	return triplet;
}

void config_write(void) {
	ConfigDb *cfg;
	char colortmp[7];
	print_status("Writing configuration");
	
	cfg = bmp_cfg_db_open();
	if (cfg) {
		bmp_cfg_db_set_int(cfg, "rootvis", "stereo", conf.stereo);
		bmp_cfg_db_set_string(cfg, "rootvis", "geometry_display", conf.geo[0].display);
		bmp_cfg_db_set_int(cfg, "rootvis", "geometry_posx", conf.geo[0].posx);
		bmp_cfg_db_set_int(cfg, "rootvis", "geometry_posy", conf.geo[0].posy);
		bmp_cfg_db_set_int(cfg, "rootvis", "geometry_orientation", conf.geo[0].orientation);
		bmp_cfg_db_set_int(cfg, "rootvis", "geometry_height", conf.geo[0].height);
		bmp_cfg_db_set_int(cfg, "rootvis", "geometry_space", conf.geo[0].space);
		bmp_cfg_db_set_int(cfg, "rootvis", "bar_width", conf.bar[0].width);
		bmp_cfg_db_set_int(cfg, "rootvis", "bar_shadow", conf.bar[0].shadow);
		bmp_cfg_db_set_int(cfg, "rootvis", "bar_falloff", conf.bar[0].falloff);
		bmp_cfg_db_set_int(cfg, "rootvis", "peak_enabled", conf.peak[0].enabled);
		bmp_cfg_db_set_int(cfg, "rootvis", "peak_falloff", conf.peak[0].falloff);
		bmp_cfg_db_set_int(cfg, "rootvis", "peak_step", conf.peak[0].step);
		bmp_cfg_db_set_int(cfg, "rootvis", "data_cutoff", conf.data[0].cutoff);
		bmp_cfg_db_set_int(cfg, "rootvis", "data_div", conf.data[0].div);
		bmp_cfg_db_set_float(cfg, "rootvis", "data_linearity", conf.data[0].linearity);
		bmp_cfg_db_set_int(cfg, "rootvis", "data_fps", conf.data[0].fps);
		bmp_cfg_db_set_string(cfg, "rootvis2", "geometry_display", conf.geo[1].display);
		bmp_cfg_db_set_int(cfg, "rootvis2", "geometry_posx", conf.geo[1].posx);
		bmp_cfg_db_set_int(cfg, "rootvis2", "geometry_posy", conf.geo[1].posy);
		bmp_cfg_db_set_int(cfg, "rootvis2", "geometry_orientation", conf.geo[1].orientation);
		bmp_cfg_db_set_int(cfg, "rootvis2", "geometry_height", conf.geo[1].height);
		bmp_cfg_db_set_int(cfg, "rootvis2", "geometry_space", conf.geo[1].space);
		bmp_cfg_db_set_int(cfg, "rootvis2", "bar_width", conf.bar[1].width);
		bmp_cfg_db_set_int(cfg, "rootvis2", "bar_shadow", conf.bar[1].shadow);
		bmp_cfg_db_set_int(cfg, "rootvis2", "bar_falloff", conf.bar[1].falloff);
		bmp_cfg_db_set_int(cfg, "rootvis2", "peak_enabled", conf.peak[1].enabled);
		bmp_cfg_db_set_int(cfg, "rootvis2", "peak_falloff", conf.peak[1].falloff);
		bmp_cfg_db_set_int(cfg, "rootvis2", "peak_step", conf.peak[1].step);
		bmp_cfg_db_set_int(cfg, "rootvis2", "data_cutoff", conf.data[1].cutoff);
		bmp_cfg_db_set_int(cfg, "rootvis2", "data_div", conf.data[1].div);
		bmp_cfg_db_set_float(cfg, "rootvis2", "data_linearity", conf.data[1].linearity);
		bmp_cfg_db_set_int(cfg, "rootvis2", "data_fps", conf.data[1].fps);
		bmp_cfg_db_set_string(cfg, "rootvis", "bar_color_1", color_arr2triplet(conf.bar[0].color[0], colortmp));
		bmp_cfg_db_set_string(cfg, "rootvis", "bar_color_2", color_arr2triplet(conf.bar[0].color[1], colortmp));
		bmp_cfg_db_set_string(cfg, "rootvis", "bar_color_3", color_arr2triplet(conf.bar[0].color[2], colortmp));
		bmp_cfg_db_set_string(cfg, "rootvis", "bar_color_4", color_arr2triplet(conf.bar[0].color[3], colortmp));
		bmp_cfg_db_set_string(cfg, "rootvis", "bar_shadow_color", color_arr2triplet(conf.bar[0].shadow_color, colortmp));
		bmp_cfg_db_set_string(cfg, "rootvis", "peak_color", color_arr2triplet(conf.peak[0].color, colortmp));
		bmp_cfg_db_set_string(cfg, "rootvis2", "bar_color_1", color_arr2triplet(conf.bar[1].color[0], colortmp));
		bmp_cfg_db_set_string(cfg, "rootvis2", "bar_color_2", color_arr2triplet(conf.bar[1].color[1], colortmp));
		bmp_cfg_db_set_string(cfg, "rootvis2", "bar_color_3", color_arr2triplet(conf.bar[1].color[2], colortmp));
		bmp_cfg_db_set_string(cfg, "rootvis2", "bar_color_4", color_arr2triplet(conf.bar[1].color[3], colortmp));
		bmp_cfg_db_set_string(cfg, "rootvis2", "bar_shadow_color", color_arr2triplet(conf.bar[1].shadow_color, colortmp));
		bmp_cfg_db_set_string(cfg, "rootvis2", "peak_color", color_arr2triplet(conf.peak[1].color, colortmp));
		bmp_cfg_db_set_int(cfg, "rootvis", "debug", conf.debug);
		bmp_cfg_db_close(cfg);
	}
}

void config_read(void) {
	int dirty;
	ConfigDb *cfg;
	gchar *colortmp;

	conf.stereo = 0;
	for (dirty = 1; dirty >= 0; dirty--) {
		conf.geo[dirty].display = "\0";
		conf.geo[dirty].posx = DEFAULT_geometry_posx;
		conf.geo[dirty].posy = DEFAULT_geometry_posy + dirty * (DEFAULT_geometry_height + 2); // second channel below first
		conf.geo[dirty].orientation = DEFAULT_geometry_orientation;
		conf.geo[dirty].height = DEFAULT_geometry_height;
		conf.geo[dirty].space = DEFAULT_geometry_space;
		conf.bar[dirty].width = DEFAULT_bar_width;
		conf.bar[dirty].shadow = DEFAULT_bar_shadow;
		conf.bar[dirty].falloff = DEFAULT_bar_falloff;
		color_triplet2arr(conf.bar[dirty].color[0], DEFAULT_bar_color_1);
		color_triplet2arr(conf.bar[dirty].color[1], DEFAULT_bar_color_2);
		color_triplet2arr(conf.bar[dirty].color[2], DEFAULT_bar_color_3);
		color_triplet2arr(conf.bar[dirty].color[3], DEFAULT_bar_color_4);
		color_triplet2arr(conf.bar[dirty].shadow_color, DEFAULT_bar_shadow_color);
		conf.peak[dirty].enabled = DEFAULT_peak_enabled;
		conf.peak[dirty].falloff = DEFAULT_peak_falloff;
		conf.peak[dirty].step = DEFAULT_peak_step;
		color_triplet2arr(conf.peak[dirty].color, DEFAULT_peak_color);
		conf.data[dirty].cutoff = DEFAULT_cutoff;
		conf.data[dirty].div = DEFAULT_div;
		conf.data[dirty].linearity = DEFAULT_linearity;
		conf.data[dirty].fps = DEFAULT_fps;
	} // dirty is now set to 0
	conf.debug = DEFAULT_debug;
	
	print_status("Reading configuration");
	
	/* without gtk it would be
	   filename = strcat(getenv("HOME"), "/.xmms/config");
	   after determining the length */
	cfg =  bmp_cfg_db_open();

	if (cfg) {
		if (!(bmp_cfg_db_get_int(cfg, "rootvis", "stereo", &conf.stereo))) dirty++;
		if (!(bmp_cfg_db_get_string(cfg, "rootvis", "geometry_display", &conf.geo[0].display))) dirty++;
		if (!(bmp_cfg_db_get_int(cfg, "rootvis", "geometry_posx", &conf.geo[0].posx))) dirty++;
		if (!(bmp_cfg_db_get_int(cfg, "rootvis", "geometry_posy", &conf.geo[0].posy))) dirty++;
		if (!(bmp_cfg_db_get_int(cfg, "rootvis", "geometry_orientation", &conf.geo[0].orientation))) dirty++;
		if (!(bmp_cfg_db_get_int(cfg, "rootvis", "geometry_height", &conf.geo[0].height))) dirty++;
		if (!(bmp_cfg_db_get_int(cfg, "rootvis", "geometry_space", &conf.geo[0].space))) dirty++;
		if (!(bmp_cfg_db_get_int(cfg, "rootvis", "bar_width", &conf.bar[0].width))) dirty++;
		if (!(bmp_cfg_db_get_int(cfg, "rootvis", "bar_shadow", &conf.bar[0].shadow))) dirty++;
		if (!(bmp_cfg_db_get_int(cfg, "rootvis", "bar_falloff", &conf.bar[0].falloff))) dirty++;
		if (!(bmp_cfg_db_get_int(cfg, "rootvis", "peak_enabled", &conf.peak[0].enabled))) dirty++;
		if (!(bmp_cfg_db_get_int(cfg, "rootvis", "peak_falloff", &conf.peak[0].falloff))) dirty++;
		if (!(bmp_cfg_db_get_int(cfg, "rootvis", "peak_step", &conf.peak[0].step))) dirty++;
		if (!(bmp_cfg_db_get_int(cfg, "rootvis", "data_cutoff", &conf.data[0].cutoff))) dirty++;
		if (!(bmp_cfg_db_get_int(cfg, "rootvis", "data_div", &conf.data[0].div))) dirty++;
		if (!(bmp_cfg_db_get_float(cfg, "rootvis", "data_linearity", &conf.data[0].linearity))) dirty++;
		if (!(bmp_cfg_db_get_int(cfg, "rootvis", "data_fps", &conf.data[0].fps))) dirty++;
		if (conf.stereo) {
			if (!(bmp_cfg_db_get_string(cfg, "rootvis2", "geometry_display", &conf.geo[1].display))) dirty++;
			if (!(bmp_cfg_db_get_int(cfg, "rootvis2", "geometry_posx", &conf.geo[1].posx))) dirty++;
			if (!(bmp_cfg_db_get_int(cfg, "rootvis2", "geometry_posy", &conf.geo[1].posy))) dirty++;
			if (!(bmp_cfg_db_get_int(cfg, "rootvis2", "geometry_orientation", &conf.geo[1].orientation))) dirty++;
			if (!(bmp_cfg_db_get_int(cfg, "rootvis2", "geometry_height", &conf.geo[1].height))) dirty++;
			if (!(bmp_cfg_db_get_int(cfg, "rootvis2", "geometry_space", &conf.geo[1].space))) dirty++;
			if (!(bmp_cfg_db_get_int(cfg, "rootvis2", "bar_width", &conf.bar[1].width))) dirty++;
			if (!(bmp_cfg_db_get_int(cfg, "rootvis2", "bar_shadow", &conf.bar[1].shadow))) dirty++;
			if (!(bmp_cfg_db_get_int(cfg, "rootvis2", "bar_falloff", &conf.bar[1].falloff))) dirty++;
			if (!(bmp_cfg_db_get_int(cfg, "rootvis2", "peak_enabled", &conf.peak[1].enabled))) dirty++;
			if (!(bmp_cfg_db_get_int(cfg, "rootvis2", "peak_falloff", &conf.peak[1].falloff))) dirty++;
			if (!(bmp_cfg_db_get_int(cfg, "rootvis2", "peak_step", &conf.peak[1].step))) dirty++;
			if (!(bmp_cfg_db_get_int(cfg, "rootvis2", "data_cutoff", &conf.data[1].cutoff))) dirty++;
			if (!(bmp_cfg_db_get_int(cfg, "rootvis2", "data_div", &conf.data[1].div))) dirty++;
			if (!(bmp_cfg_db_get_float(cfg, "rootvis2", "data_linearity", &conf.data[1].linearity))) dirty++;
			if (!(bmp_cfg_db_get_int(cfg, "rootvis2", "data_fps", &conf.data[1].fps))) dirty++;
		}
		
		if (!(bmp_cfg_db_get_int(cfg, "rootvis", "debug", &conf.debug))) dirty++;
		if (!(bmp_cfg_db_get_string(cfg, "rootvis", "bar_color_1", &colortmp))) dirty++;
		 else	color_triplet2arr(conf.bar[0].color[0], colortmp);
		if (!(bmp_cfg_db_get_string(cfg, "rootvis", "bar_color_2", &colortmp))) dirty++;
		 else	color_triplet2arr(conf.bar[0].color[1], colortmp);
		if (!(bmp_cfg_db_get_string(cfg, "rootvis", "bar_color_3", &colortmp))) dirty++;
		 else	color_triplet2arr(conf.bar[0].color[2], colortmp);
		if (!(bmp_cfg_db_get_string(cfg, "rootvis", "bar_color_4", &colortmp))) dirty++;
		 else	color_triplet2arr(conf.bar[0].color[3], colortmp);
		if (!(bmp_cfg_db_get_string(cfg, "rootvis", "bar_shadow_color", &colortmp))) dirty++;
		 else	color_triplet2arr(conf.bar[0].shadow_color, colortmp);
		if (!(bmp_cfg_db_get_string(cfg, "rootvis", "peak_color", &colortmp))) dirty++;
		 else	color_triplet2arr(conf.peak[0].color, colortmp);
		if (conf.stereo) {
			if (!(bmp_cfg_db_get_string(cfg, "rootvis2", "bar_color_1", &colortmp))) dirty++;
			else	color_triplet2arr(conf.bar[1].color[0], colortmp);
			if (!(bmp_cfg_db_get_string(cfg, "rootvis2", "bar_color_2", &colortmp))) dirty++;
			else	color_triplet2arr(conf.bar[1].color[1], colortmp);
			if (!(bmp_cfg_db_get_string(cfg, "rootvis2", "bar_color_3", &colortmp))) dirty++;
			else	color_triplet2arr(conf.bar[1].color[2], colortmp);
			if (!(bmp_cfg_db_get_string(cfg, "rootvis2", "bar_color_4", &colortmp))) dirty++;
			else	color_triplet2arr(conf.bar[1].color[3], colortmp);
			if (!(bmp_cfg_db_get_string(cfg, "rootvis2", "bar_shadow_color", &colortmp))) dirty++;
			else	color_triplet2arr(conf.bar[1].shadow_color, colortmp);
			if (!(bmp_cfg_db_get_string(cfg, "rootvis2", "peak_color", &colortmp))) dirty++;
			else	color_triplet2arr(conf.peak[1].color, colortmp);
		}
		bmp_cfg_db_close(cfg);
		printf("%s", conf.geo[0].display);
		if (dirty > 0) config_write();
	} 
	
	print_status("Configuration finished");
}
