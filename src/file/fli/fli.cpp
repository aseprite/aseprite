
/*
 * Written 1998 Jens Ch. Restemeier <jchrr@hrz.uni-bielefeld.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* Modified by David Capello to use with ASE (2001-2010).
   See ../README.txt for more information
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fli.h"

/*
 * To avoid endian-problems I wrote these functions:
 */
static unsigned char fli_read_char(FILE *f)
{
	unsigned char b;
	fread(&b,1,1,f);
	return b;
}

static unsigned short fli_read_short(FILE *f)
{
	unsigned char b[2];
	fread(&b,1,2,f);
	return (unsigned short)(b[1]<<8) | b[0];
}

static unsigned long fli_read_long(FILE *f)
{
	unsigned char b[4];
	fread(&b,1,4,f);
	return (unsigned long)(b[3]<<24) | (b[2]<<16) | (b[1]<<8) | b[0];
}

static void fli_write_char(FILE *f, unsigned char b)
{
	fwrite(&b,1,1,f);
}

static void fli_write_short(FILE *f, unsigned short w)
{
	unsigned char b[2];
	b[0]=w&255;
	b[1]=(w>>8)&255;
	fwrite(&b,1,2,f);
}

static void fli_write_long(FILE *f, unsigned long l)
{
	unsigned char b[4];
	b[0]=l&255;
	b[1]=(l>>8)&255;
	b[2]=(l>>16)&255;
	b[3]=(l>>24)&255;
	fwrite(&b,1,4,f);
}

void fli_read_header(FILE *f, s_fli_header *fli_header)
{
	fli_header->filesize=fli_read_long(f);	/* 0 */
	fli_header->magic=fli_read_short(f);	/* 4 */
	fli_header->frames=fli_read_short(f);	/* 6 */
	fli_header->width=fli_read_short(f);	/* 8 */
	fli_header->height=fli_read_short(f);	/* 10 */
	fli_header->depth=fli_read_short(f);	/* 12 */
	fli_header->flags=fli_read_short(f);	/* 14 */
	if (fli_header->magic == HEADER_FLI) {
		/* FLI saves speed in 1/70s */
		fli_header->speed=fli_read_short(f)*14;		/* 16 */
	} else {
		if (fli_header->magic == HEADER_FLC) {
			/* FLC saves speed in 1/1000s */
			fli_header->speed=fli_read_long(f);	/* 16 */
		} else {
			fprintf(stderr, "error: magic number is wrong !\n");
			fli_header->magic = NO_HEADER;
		}
	}

	if (fli_header->width == 0)
	  fli_header->width = 320;

	if (fli_header->height == 0)
	  fli_header->height = 200;
}

void fli_write_header(FILE *f, s_fli_header *fli_header)
{
	fli_header->filesize=ftell(f);
	fseek(f, 0, SEEK_SET);
	fli_write_long(f, fli_header->filesize);	/* 0 */
	fli_write_short(f, fli_header->magic);	/* 4 */
	fli_write_short(f, fli_header->frames);	/* 6 */
	fli_write_short(f, fli_header->width);	/* 8 */
	fli_write_short(f, fli_header->height);	/* 10 */
	fli_write_short(f, fli_header->depth);	/* 12 */
	fli_write_short(f, fli_header->flags);	/* 14 */
	if (fli_header->magic == HEADER_FLI) {
		/* FLI saves speed in 1/70s */
		fli_write_short(f, fli_header->speed / 14);	/* 16 */
	} else {
		if (fli_header->magic == HEADER_FLC) {
			/* FLC saves speed in 1/1000s */
			fli_write_long(f, fli_header->speed);	/* 16 */
			fseek(f, 80, SEEK_SET);
			fli_write_long(f, fli_header->oframe1);	/* 80 */
			fli_write_long(f, fli_header->oframe2);	/* 84 */
		} else {
			fprintf(stderr, "error: magic number in header is wrong !\n");
		}
	}
}

void fli_read_frame(FILE *f, s_fli_header *fli_header, unsigned char *old_framebuf, unsigned char *old_cmap, unsigned char *framebuf, unsigned char *cmap)
{
	s_fli_frame fli_frame;
	unsigned long framepos;
	int c;
	framepos=ftell(f);

	fli_frame.size=fli_read_long(f);
	fli_frame.magic=fli_read_short(f);
	fli_frame.chunks=fli_read_short(f);

	if (fli_frame.magic == FRAME) {
		fseek(f, framepos+16, SEEK_SET);
		for (c=0;c<fli_frame.chunks;c++) {
			s_fli_chunk chunk;
			unsigned long chunkpos;
			chunkpos = ftell(f);
			chunk.size=fli_read_long(f);
			chunk.magic=fli_read_short(f);
			switch (chunk.magic) {
				case FLI_COLOR:   fli_read_color(f, fli_header, old_cmap, cmap); break;
				case FLI_COLOR_2: fli_read_color_2(f, fli_header, old_cmap, cmap); break;
				case FLI_BLACK:	  fli_read_black(f, fli_header, framebuf); break;
				case FLI_BRUN:    fli_read_brun(f, fli_header, framebuf); break;
				case FLI_COPY:    fli_read_copy(f, fli_header, framebuf); break;
				case FLI_LC:      fli_read_lc(f, fli_header, old_framebuf, framebuf); break;
				case FLI_LC_2:    fli_read_lc_2(f, fli_header, old_framebuf, framebuf); break;
				case FLI_MINI:	  /* unused, skip */ break;
				default: /* unknown, skip */ break;
			}
			if (chunk.size & 1) chunk.size++;
			fseek(f,chunkpos+chunk.size,SEEK_SET);
		}
	} /* else: unknown, skip */ 
	fseek(f, framepos+fli_frame.size, SEEK_SET);
}

void fli_write_frame(FILE *f, s_fli_header *fli_header, unsigned char *old_framebuf, unsigned char *old_cmap, unsigned char *framebuf, unsigned char *cmap, unsigned short codec_mask)
{
	s_fli_frame fli_frame;
	unsigned long framepos, frameend;
	framepos=ftell(f);
	fseek(f, framepos+16, SEEK_SET);

	switch (fli_header->frames) {
		case 0: fli_header->oframe1=framepos; break;
		case 1: fli_header->oframe2=framepos; break;
	}

	fli_frame.size=0;
	fli_frame.magic=FRAME;
	fli_frame.chunks=0;

	/* 
	 * create color chunk 
	 */
	if (fli_header->magic == HEADER_FLI) {
		if (fli_write_color(f, fli_header, old_cmap, cmap)) fli_frame.chunks++;
	} else {
		if (fli_header->magic == HEADER_FLC) {
			if (fli_write_color_2(f, fli_header, old_cmap, cmap)) fli_frame.chunks++;
		} else {
			fprintf(stderr, "error: magic number in header is wrong !\n");
		}
	}

#if 0
	if (codec_mask & W_COLOR) {
		if (fli_write_color(f, fli_header, old_cmap, cmap)) fli_frame.chunks++;
	}
	if (codec_mask & W_COLOR_2) {
		if (fli_write_color_2(f, fli_header, old_cmap, cmap)) fli_frame.chunks++;
	}
#endif
	/* create bitmap chunk */
	if (old_framebuf==NULL) {
		fli_write_brun(f, fli_header, framebuf);
	} else {
		fli_write_lc(f, fli_header, old_framebuf, framebuf);
	}
	fli_frame.chunks++;
	
	frameend=ftell(f);
	fli_frame.size=frameend-framepos;
	fseek(f, framepos, SEEK_SET);
	fli_write_long(f, fli_frame.size);
	fli_write_short(f, fli_frame.magic);
	fli_write_short(f, fli_frame.chunks);
	fseek(f, frameend, SEEK_SET);
	fli_header->frames++;
}

/*
 * palette chunks from the classical Autodesk Animator.
 */
void fli_read_color(FILE *f, s_fli_header *fli_header, unsigned char *old_cmap, unsigned char *cmap)
{
	unsigned short num_packets, cnt_packets, col_pos;
	col_pos=0;
	num_packets=fli_read_short(f);
	for (cnt_packets=num_packets; cnt_packets>0; cnt_packets--) {
		unsigned short skip_col, num_col, col_cnt;
		skip_col=fli_read_char(f);
		num_col=fli_read_char(f);
		if (num_col==0) {
			for (col_pos=0; col_pos<768; col_pos++) {
				cmap[col_pos]=fli_read_char(f)<<2;
			}
			return;
		}
		for (col_cnt=skip_col; (col_cnt>0) && (col_pos<768); col_cnt--) {
			cmap[col_pos]=old_cmap[col_pos];col_pos++;
			cmap[col_pos]=old_cmap[col_pos];col_pos++;
			cmap[col_pos]=old_cmap[col_pos];col_pos++;
		}
		for (col_cnt=num_col; (col_cnt>0) && (col_pos<768); col_cnt--) {
			cmap[col_pos++]=fli_read_char(f)<<2;
			cmap[col_pos++]=fli_read_char(f)<<2;
			cmap[col_pos++]=fli_read_char(f)<<2;
		}
	}
}

int fli_write_color(FILE *f, s_fli_header *fli_header, unsigned char *old_cmap, unsigned char *cmap)
{
	unsigned long chunkpos;
	unsigned short num_packets;
	s_fli_chunk chunk; 
	chunkpos=ftell(f);
	fseek(f, chunkpos+8, SEEK_SET);
	num_packets=0;
	if (old_cmap==NULL) {
		unsigned short col_pos;
		num_packets=1;
		fli_write_char(f, 0); /* skip no color */
		fli_write_char(f, 0); /* 256 color */
		for (col_pos=0; col_pos<768; col_pos++) {
			fli_write_char(f, cmap[col_pos]>>2);
		}
	} else {
		unsigned short cnt_skip, cnt_col, col_pos, col_start;
		col_pos=0;
		do {
			cnt_skip=0; 
			while ((col_pos<256) && (old_cmap[col_pos*3+0]==cmap[col_pos*3+0]) && (old_cmap[col_pos*3+1]==cmap[col_pos*3+1]) && (old_cmap[col_pos*3+2]==cmap[col_pos*3+2])) {
				cnt_skip++; col_pos++;
			}
			col_start=col_pos*3;
			cnt_col=0; 
			while ((col_pos<256) && !((old_cmap[col_pos*3+0]==cmap[col_pos*3+0]) && (old_cmap[col_pos*3+1]==cmap[col_pos*3+1]) && (old_cmap[col_pos*3+2]==cmap[col_pos*3+2]))) {
				cnt_col++; col_pos++;
			}
			if (cnt_col>0) {
				num_packets++;
				fli_write_char(f, cnt_skip & 255);
				fli_write_char(f, cnt_col & 255);
				while (cnt_col>0) {
					fli_write_char(f, cmap[col_start++]>>2);
					fli_write_char(f, cmap[col_start++]>>2);
					fli_write_char(f, cmap[col_start++]>>2);
					cnt_col--;
				}
			}
		} while (col_pos<256);
	}

	if (num_packets>0) {
		chunk.size=ftell(f)-chunkpos;
		chunk.magic=FLI_COLOR;

		fseek(f, chunkpos, SEEK_SET);
		fli_write_long(f, chunk.size);
		fli_write_short(f, chunk.magic);
		fli_write_short(f, num_packets);

		if (chunk.size & 1) chunk.size++;
		fseek(f,chunkpos+chunk.size,SEEK_SET);
		return 1;
	}
	fseek(f,chunkpos, SEEK_SET);
	return 0;
}

/*
 * palette chunks from Autodesk Animator pro
 */
void fli_read_color_2(FILE *f, s_fli_header *fli_header, unsigned char *old_cmap, unsigned char *cmap)
{
	unsigned short num_packets, cnt_packets, col_pos;
	num_packets=fli_read_short(f);
	col_pos=0;
	for (cnt_packets=num_packets; cnt_packets>0; cnt_packets--) {
		unsigned short skip_col, num_col, col_cnt;
		skip_col=fli_read_char(f);
		num_col=fli_read_char(f);
		if (num_col == 0) {
			for (col_pos=0; col_pos<768; col_pos++) {
				cmap[col_pos]=fli_read_char(f);
			}
			return;
		} 
		for (col_cnt=skip_col; (col_cnt>0) && (col_pos<768); col_cnt--) {
			cmap[col_pos]=old_cmap[col_pos];col_pos++;
			cmap[col_pos]=old_cmap[col_pos];col_pos++;
			cmap[col_pos]=old_cmap[col_pos];col_pos++;
		}
		for (col_cnt=num_col; (col_cnt>0) && (col_pos<768); col_cnt--) {
			cmap[col_pos++]=fli_read_char(f);				
			cmap[col_pos++]=fli_read_char(f);
			cmap[col_pos++]=fli_read_char(f);
		}
	}
}

int fli_write_color_2(FILE *f, s_fli_header *fli_header, unsigned char *old_cmap, unsigned char *cmap)
{
	unsigned long chunkpos;
	unsigned short num_packets;
	s_fli_chunk chunk; 
	chunkpos=ftell(f);
	fseek(f, chunkpos+8, SEEK_SET);
	num_packets=0;
	if (old_cmap==NULL) {
		unsigned short col_pos;
		num_packets=1;
		fli_write_char(f, 0); /* skip no color */
		fli_write_char(f, 0); /* 256 color */
		for (col_pos=0; col_pos<768; col_pos++) {
			fli_write_char(f, cmap[col_pos]);
		}
	} else {
		unsigned short cnt_skip, cnt_col, col_pos, col_start;
		col_pos=0;
		do {
			cnt_skip=0; 
			while ((col_pos<256) && (old_cmap[col_pos*3+0]==cmap[col_pos*3+0]) && (old_cmap[col_pos*3+1]==cmap[col_pos*3+1]) && (old_cmap[col_pos*3+2]==cmap[col_pos*3+2])) {
				cnt_skip++; col_pos++;
			}
			col_start=col_pos*3;
			cnt_col=0; 
			while ((col_pos<256) && !((old_cmap[col_pos*3+0]==cmap[col_pos*3+0]) && (old_cmap[col_pos*3+1]==cmap[col_pos*3+1]) && (old_cmap[col_pos*3+2]==cmap[col_pos*3+2]))) {
				cnt_col++; col_pos++;
			}
			if (cnt_col>0) {
				num_packets++;
				fli_write_char(f, cnt_skip);
				fli_write_char(f, cnt_col);
				while (cnt_col>0) {
					fli_write_char(f, cmap[col_start++]);
					fli_write_char(f, cmap[col_start++]);
					fli_write_char(f, cmap[col_start++]);
					cnt_col--;
				}
			}
		} while (col_pos<256);
	}

	if (num_packets>0) {
		chunk.size=ftell(f)-chunkpos;
		chunk.magic=FLI_COLOR_2;

		fseek(f, chunkpos, SEEK_SET);
		fli_write_long(f, chunk.size);
		fli_write_short(f, chunk.magic);
		fli_write_short(f, num_packets);

		if (chunk.size & 1) chunk.size++;
		fseek(f,chunkpos+chunk.size,SEEK_SET);
		return 1;
	}
	fseek(f,chunkpos, SEEK_SET);
	return 0;
}

/*
 * completely black frame
 */
void fli_read_black(FILE *f, s_fli_header *fli_header, unsigned char *framebuf)
{
	memset(framebuf, 0, fli_header->width * fli_header->height);
}

void fli_write_black(FILE *f, s_fli_header *fli_header, unsigned char *framebuf)
{
	s_fli_chunk chunk; 

	chunk.size=6;
	chunk.magic=FLI_BLACK;

	fli_write_long(f, chunk.size);
	fli_write_short(f, chunk.magic);
}

/*
 * Uncompressed frame
 */
void fli_read_copy(FILE *f, s_fli_header *fli_header, unsigned char *framebuf)
{
	fread(framebuf, fli_header->width, fli_header->height, f);
}

void fli_write_copy(FILE *f, s_fli_header *fli_header, unsigned char *framebuf)
{
	
	unsigned long chunkpos;
	s_fli_chunk chunk; 
	chunkpos=ftell(f);
	fseek(f, chunkpos+6, SEEK_SET);
	fwrite(framebuf, fli_header->width, fli_header->height, f);
	chunk.size=ftell(f)-chunkpos;
	chunk.magic=FLI_COPY;

	fseek(f, chunkpos, SEEK_SET);
	fli_write_long(f, chunk.size);
	fli_write_short(f, chunk.magic);
	
	if (chunk.size & 1) chunk.size++;
	fseek(f,chunkpos+chunk.size,SEEK_SET);
}

/*
 * This is a RLE algorithm, used for the first image of an animation
 */
void fli_read_brun(FILE *f, s_fli_header *fli_header, unsigned char *framebuf)
{
	unsigned short yc;
	unsigned char *pos;
	for (yc=0; yc < fli_header->height; yc++) {
		unsigned short xc, pc, pcnt;
		pc=fli_read_char(f);
		xc=0;
		pos=framebuf+(fli_header->width * yc);
		for (pcnt=pc; pcnt>0; pcnt--) {
			unsigned short ps;
			ps=fli_read_char(f);
			if (ps & 0x80) {
				unsigned short len; 
				for (len=-(signed char)ps; len>0; len--) {
					pos[xc++]=fli_read_char(f);
				} 
			} else {
				unsigned char val;
				val=fli_read_char(f);
				memset(&(pos[xc]), val, ps);
				xc+=ps;
			}
		}
	} 
}

void fli_write_brun(FILE *f, s_fli_header *fli_header, unsigned char *framebuf)
{
	unsigned long chunkpos;
	s_fli_chunk chunk; 
	unsigned short yc;
	unsigned char *linebuf;
	
	chunkpos=ftell(f);
	fseek(f, chunkpos+6, SEEK_SET);

	for (yc=0; yc < fli_header->height; yc++) {
		unsigned short xc, t1, pc, tc;
		unsigned long linepos, lineend, bc;
		linepos=ftell(f); bc=0;
		fseek(f, 1, SEEK_CUR);
		linebuf=framebuf + (yc*fli_header->width);
		xc=0; tc=0; t1=0;
		while (xc < fli_header->width) {
			pc=1;
			while ((pc<120) && ((xc+pc)<fli_header->width) && (linebuf[xc+pc] == linebuf[xc])) {
				pc++; 
			}
			if (pc>2) {
				if (tc>0) {
					bc++;
					fli_write_char(f, (tc-1)^0xFF);
					fwrite(linebuf+t1, 1, tc, f);
					tc=0;
				}
				bc++;
				fli_write_char(f, pc);
				fli_write_char(f, linebuf[xc]);
				t1=xc+pc;
			} else {
				tc+=pc;
				if (tc>120) {
					bc++;
					fli_write_char(f, (tc-1)^0xFF);
					fwrite(linebuf+t1, 1, tc, f);
					tc=0;
					t1=xc+pc;
				}
			}
			xc+=pc;
		}
		if (tc>0) {
			bc++;
			fli_write_char(f, (tc-1)^0xFF);
			fwrite(linebuf+t1, 1, tc, f);
			tc=0;
		}
		lineend=ftell(f); 
		fseek(f, linepos, SEEK_SET);
		fli_write_char(f, bc);
		fseek(f, lineend, SEEK_SET);
	}

	chunk.size=ftell(f)-chunkpos;
	chunk.magic=FLI_BRUN;

	fseek(f, chunkpos, SEEK_SET);
	fli_write_long(f, chunk.size);
	fli_write_short(f, chunk.magic);
	
	if (chunk.size & 1) chunk.size++;
	fseek(f,chunkpos+chunk.size,SEEK_SET);
}

/*
 * This is the delta-compression method from the classic Autodesk Animator. 
 * It's basically the RLE method from above, but it allows to skip unchanged
 * lines at the beginning and end of an image, and unchanged pixels in a line
 * This chunk is used in FLI files.
 */
void fli_read_lc(FILE *f, s_fli_header *fli_header, unsigned char *old_framebuf, unsigned char *framebuf)
{
	unsigned short yc, firstline, numline;
	unsigned char *pos;
	memcpy(framebuf, old_framebuf, fli_header->width * fli_header->height);
	firstline = fli_read_short(f);
	numline = fli_read_short(f);
	for (yc=0; yc < numline; yc++) {
		unsigned short xc, pc, pcnt;
		pc=fli_read_char(f);
		xc=0;
		pos=framebuf+(fli_header->width * (firstline+yc));
		for (pcnt=pc; pcnt>0; pcnt--) {
			unsigned short ps,skip;
			skip=fli_read_char(f);
			ps=fli_read_char(f);
			xc+=skip;
			if (ps & 0x80) {
				unsigned char val;
				ps=-(signed char)ps;
				val=fli_read_char(f);
				memset(&(pos[xc]), val, ps);
				xc+=ps;
			} else {
				fread(&(pos[xc]), ps, 1, f);
				xc+=ps;
			}
		}
	} 
}

void fli_write_lc(FILE *f, s_fli_header *fli_header, unsigned char *old_framebuf, unsigned char *framebuf)
{
	unsigned long chunkpos;
	s_fli_chunk chunk; 
	unsigned short yc, firstline, numline, lastline;
	unsigned char *linebuf, *old_linebuf;
	
	chunkpos=ftell(f);
	fseek(f, chunkpos+6, SEEK_SET);

	/* first check, how many lines are unchanged at the beginning */
	firstline=0;
	while ((memcmp(old_framebuf+(firstline*fli_header->width), framebuf+(firstline*fli_header->width), fli_header->width)==0) && (firstline<fli_header->height)) firstline++;
	
	/* then check from the end, how many lines are unchanged */
	if (firstline<fli_header->height) {
		lastline=fli_header->height-1;
		while ((memcmp(old_framebuf+(lastline*fli_header->width), framebuf+(lastline*fli_header->width), fli_header->width)==0) && (lastline>firstline)) lastline--;
		numline=(lastline-firstline)+1;
	} else {
		numline=0; 
	}
	if (numline==0) firstline=0;

	fli_write_short(f, firstline);
	fli_write_short(f, numline);

	for (yc=0; yc < numline; yc++) {
		unsigned short xc, sc, cc, tc;
		unsigned long linepos, lineend, bc;
		linepos=ftell(f); bc=0;
		fseek(f, 1, SEEK_CUR);

		linebuf=framebuf + ((firstline+yc)*fli_header->width);
		old_linebuf=old_framebuf + ((firstline+yc)*fli_header->width);
		xc=0;
		while (xc < fli_header->width) {
			sc=0;
			while ((linebuf[xc]==old_linebuf[xc]) && (xc<fli_header->width) && (sc<255)) {
				xc++; sc++;
			}
			fli_write_char(f, sc);
			cc=1;
			while ((linebuf[xc]==linebuf[xc+cc]) && ((xc+cc)<fli_header->width) && (cc<120)) {
				cc++;
			}
			if (cc>2) {
				bc++;
				fli_write_char(f, (cc-1)^0xFF);
				fli_write_char(f, linebuf[xc]);
				xc+=cc;
			} else {
				tc=0;
				do {
					sc=0;
					while ((linebuf[tc+xc+sc]==old_linebuf[tc+xc+sc]) && ((tc+xc+sc)<fli_header->width) && (sc<5)) {
						sc++;
					}
					cc=1;
					while ((linebuf[tc+xc]==linebuf[tc+xc+cc]) && ((tc+xc+cc)<fli_header->width) && (cc<10)) {
						cc++;
					}
					tc++;
				} while ((tc<120) && (cc<9) && (sc<4) && ((xc+tc)<fli_header->width));
				bc++;
				fli_write_char(f, tc);
				fwrite(linebuf+xc, tc, 1, f);
				xc+=tc;
			}
		}
		lineend=ftell(f); 
		fseek(f, linepos, SEEK_SET);
		fli_write_char(f, bc);
		fseek(f, lineend, SEEK_SET);
	}

	chunk.size=ftell(f)-chunkpos;
	chunk.magic=FLI_LC;

	fseek(f, chunkpos, SEEK_SET);
	fli_write_long(f, chunk.size);
	fli_write_short(f, chunk.magic);
	
	if (chunk.size & 1) chunk.size++;
	fseek(f,chunkpos+chunk.size,SEEK_SET);
}


/*
 * This is an enhanced version of the old delta-compression used by
 * the autodesk animator pro. It's word-oriented, and allows to skip
 * larger parts of the image. This chunk is used in FLC files.
 */
void fli_read_lc_2(FILE *f, s_fli_header *fli_header, unsigned char *old_framebuf, unsigned char *framebuf)
{
	unsigned short yc, lc, numline;
	unsigned char *pos;
	memcpy(framebuf, old_framebuf, fli_header->width * fli_header->height);
	yc=0;
	numline = fli_read_short(f);
	for (lc=0; lc < numline; lc++) {
		unsigned short xc, pc, pcnt, lpf, lpn;
		pc=fli_read_short(f);
		lpf=0; lpn=0;
		while (pc & 0x8000) {
			if (pc & 0x4000) {
				yc+=-(signed short)pc;
			} else {
				lpf=1;lpn=pc&0xFF;
			}
			pc=fli_read_short(f);
		}
		xc=0;
		pos=framebuf+(fli_header->width * yc);
		for (pcnt=pc; pcnt>0; pcnt--) {
			unsigned short ps,skip;
			skip=fli_read_char(f);
			ps=fli_read_char(f);
			xc+=skip;
			if (ps & 0x80) {
				unsigned char v1,v2;
				ps=-(signed char)ps;
				v1=fli_read_char(f);
				v2=fli_read_char(f);
				while (ps>0) {
					pos[xc++]=v1;
					pos[xc++]=v2;
					ps--;
				}
			} else {
				fread(&(pos[xc]), ps, 2, f);
				xc+=ps << 1;
			}
		}
		if (lpf) pos[xc]=lpn;
		yc++;
	} 
}
