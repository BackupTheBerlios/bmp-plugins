/*  
 *  Copyright (C) 2004 Roman Bogorodskiy <bogorodskiy@inbox.ru>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "avcodec.h"
#include "avformat.h"
#include "playlist.h"
#include "wma123.h"

#define ST_BUFF 2048

void audio_decode(const char *filename)
{
	AVCodec *codec;
    	AVPacket pkt;
    	AVCodecContext *c= NULL;
    	AVFormatContext *ic= NULL;
    	int out_size, size, len, err, i;
    	int tns, thh, tmm, tss, st_buff;
    	uint8_t *outbuf, *inbuf_ptr, *s_outbuf;
    	FifoBuffer f;
    	int oss_fd = -1;
    	int val;
    
    	err = av_open_input_file(&ic, filename, NULL, 0, NULL);
    	if (err < 0) {
        	(void)fprintf(stderr, "Error file %s\n", filename);
		exit(1);
    	}

    	for (i = 0; i < ic->nb_streams; i++) {
        	c = &ic->streams[i]->codec;
        	if (c->codec_type == CODEC_TYPE_AUDIO)
            		break;
    	}

    	av_find_stream_info(ic);

    	codec = avcodec_find_decoder(c->codec_id);        
    	if (!codec) {
        	(void)fprintf(stderr, "Codec was not found.\n");
        	exit(1);
    	}
    
	/* open it */
    	if (avcodec_open(c, codec) < 0) {
        	(void)fprintf(stderr, "could not open codec\n");
        	exit(1);
    	}
    
    	st_buff = ST_BUFF;

    	outbuf = malloc(AVCODEC_MAX_AUDIO_FRAME_SIZE);
    	s_outbuf = malloc(st_buff);

        (void)fprintf(stderr, "\n\n");
    	dump_format(ic, 0, filename, 0);
   
       	if (quiet != 1) {	
		if (ic->title[0] != '\0')
        		(void)fprintf(stderr, "Title: %s\n", ic->title);
    
		if (ic->author[0] != '\0')
        		(void)fprintf(stderr, "Author: %s\n", ic->author);
    	
		if (ic->album[0] != '\0')
        		(void)fprintf(stderr, "Album: %s\n", ic->album);
    
		if (ic->year != 0)
        		(void)fprintf(stderr, "Year: %d\n", ic->year);
    
		if (ic->track != 0)
        		(void)fprintf(stderr, "Track: %d\n", ic->track);
    
		if (ic->genre[0] != '\0')
        		(void)fprintf(stderr, "Genre: %s\n", ic->genre);
    
		if (ic->copyright[0] != '\0')
        		(void)fprintf(stderr, "Copyright: %s\n", ic->copyright);
    
		if (ic->comment[0] != '\0')
        		(void)fprintf(stderr, "Comments: %s\n", ic->comment);

    		if (ic->duration != 0) {
			tns = ic->duration/1000000LL;
			thh = tns/3600;
			tmm = (tns%3600)/60;
			tss = (tns%60);
	
			(void)fprintf(stderr, "Time: %2d:%02d:%02d\n", thh, tmm, tss);
    		}
	}
		
	tns = 0;
	thh = 0;
	tmm = 0;
	tss = 0;
	
    	if ((oss_fd = open(audio_dev,  O_WRONLY, 0)) == -1) {	
        	(void)fprintf (stderr, "Cannot open audio device %s\n", audio_dev);
		exit(1);
    	}     
    
    	val = AFMT_S16_LE;
    	err = ioctl(oss_fd, SNDCTL_DSP_SETFMT, &val);
    	
	if (err < 0) {
		perror("SNDCTL_DSP_SETFMT");
		exit (1);
    	}    

    	val = c->channels-1;
    	err = ioctl(oss_fd, SNDCTL_DSP_STEREO, &val);
    
	if (err < 0) {
		perror("SNDCTL_DSP_STEREO");
		exit (1);
    	}    

    	val = c->sample_rate;
    	err = ioctl(oss_fd, SNDCTL_DSP_SPEED, &val);
    
	if(err < 0) {
		perror("SNDCTL_DSP_SPEED");
		exit (1);
    	}    

    	val = 16;
    	err = ioctl(oss_fd, SNDCTL_DSP_SAMPLESIZE, &val);
    
	if(err < 0) {
		perror("SNDCTL_DSP_SAMPLESIZE");
		exit (1);
    	}    

    	for(;;){
		if (av_read_frame(ic, &pkt) < 0)
	    		break;

		size = pkt.size;
		inbuf_ptr = pkt.data;
		tns = pkt.pts/1000000LL;
		thh = tns/3600;
		tmm = (tns%3600)/60;
		tss = (tns%60);
	
		if (quiet != 1)
			(void)fprintf(stderr, "Play Time: %2d:%02d:%02d bitrate: %d kb/s\r", thh, tmm, 
				tss, c->bit_rate / 1000);

        	if (size == 0)
            		break;

        	while (size > 0) {
            		len = avcodec_decode_audio(c, (short *)outbuf, &out_size, 
                                       inbuf_ptr, size);
	    	
            		if (len < 0) 
				break;
            
	    
            		if (out_size <= 0) 
				continue;
	    
	    
            		if (out_size > 0) {
				fifo_init(&f, out_size*2);
				fifo_write(&f, outbuf, out_size, &f.wptr);
			
				while(fifo_read(&f, s_outbuf, st_buff, &f.rptr) == 0) 
					write(oss_fd, (short *)s_outbuf, st_buff);
		
				fifo_free(&f);
            		}
            
			size -= len;
            		inbuf_ptr += len;
	        
			if (pkt.data)
        	    		av_free_packet(&pkt);
        	}

	}

    	(void)fprintf(stderr, "\n");
    	close(oss_fd);
    
    	if (s_outbuf)
		free(s_outbuf);
    	if (outbuf)
		free(outbuf);

	avcodec_close(c);
    	av_close_input_file(ic);
}

static void
help()
{

	(void)fprintf(stderr, "usage: [args] file1 file2 ...\n\n");
	(void)fprintf(stderr, "arguments:\n");
	(void)fprintf(stderr, "\t-q\tbe quiet (don't print title) [default: off]\n");
	(void)fprintf(stderr, "\t-h\tprint this text\n");
	(void)fprintf(stderr,"\t-a <dev>\t-use device <dev> for audio output [default: /dev/dsp0]\n");	
	exit(0);
}

int main(int argc, char *argv[])
{
	char **playlist_array;
    	playlist_t *playlist;
    	int items, 
	    i = 0;
    	struct stat stat_s;
   
	int ch, fd;

	while ((ch = getopt(argc, argv, "a:hq")) != -1)
        	switch (ch) {
			case 'a':
				audio_dev = optarg;
				break;
			case 'q':
                     		quiet = 1;
                     		break;
			case 'h':
				help();
             		case '?':
             		default:
                     		usage();
             	}
     
	argc -= optind;
     	argv += optind;

       	
    	if (argc < 1)
		usage();
   	
    	avcodec_init();

    	avcodec_register_all();
    	av_register_all();
    
    	playlist = playlist_create();
    	
	for (i = 0; i < argc; i++) {
    		if (stat(argv[i], &stat_s) == 0) {
			if (S_ISDIR(stat_s.st_mode)) {
	    			if (playlist_append_directory(playlist, argv[i]) == 0)
				(void)fprintf(stderr, "Warning: Could not read directory %s.\n", argv[i]);
			} else {	
	    			playlist_append_file(playlist, argv[i]);
			}
    		} else 
			playlist_append_file(playlist, argv[i]);
    	}

    	if (playlist_length(playlist)) {
		playlist_array = playlist_to_array(playlist, &items);
		playlist_destroy(playlist);
		playlist = NULL;
    	} else
		exit(1);
   
       	i = 0;	
    	while (i < items) {
		audio_decode(playlist_array[i]);
		i++;
    	}

    	playlist_array_destroy(playlist_array, items);
    	
	exit(0);
}

static void
usage()
{

	(void)fprintf(stderr, "usage: wma123 [-hq] [-a device] file1 file2 ...\n");
	exit(0);
}
	
