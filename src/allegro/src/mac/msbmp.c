/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      System bitmaps support.
 *
 *      By Ronaldo Hideki Yamada.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "macalleg.h"
#include "allegro/platform/aintmac.h"
#include <string.h>

#pragma mark system bitmaps vtables

GFX_VTABLE __mac_sys_vtable8;
GFX_VTABLE __mac_sys_vtable15;
GFX_VTABLE __mac_sys_vtable24;

GFX_VTABLE __cache_vtable8;
GFX_VTABLE __cache_vtable15;
GFX_VTABLE __cache_vtable24;

extern void _mac_sys_clear15(struct BITMAP *bmp,int color);
extern void _mac_sys_putpixel8(BITMAP *dst, int x, int y, int color);
extern void _ppc_linear_putpixel8(BITMAP *dst, int x, int y, int color);
extern void _ppc_linear_hline8(BITMAP *dst, int x1, int y, int x2, int color);
extern void _ppc_linear_hline8_32(BITMAP *dst, int x1, int y, int x2, int color);
extern void _ppc_linear_vline8(BITMAP *dst, int x, int y1, int y2, int color);
extern void _ppc_linear_rectfill8(BITMAP *dst, int x1, int y1, int x2, int y2, int color);
extern void _ppc_linear_rectfill8_32(BITMAP *dst, int x1, int y1, int x2, int y2, int color);
extern void _ppc_linear_clear_to_color8(BITMAP *dst, int color);
extern void _ppc_linear_clear_to_color8_32(BITMAP *dst, int color);
extern void _ppc_linear_clear_to_color15(BITMAP *dst, int color);
extern void _ppc_linear_clear_to_color15_32(BITMAP *dst, int color);

#pragma mark system bitmaps methods



/*
 *
 */
void _mac_init_system_bitmap(void){
   BlockMove(&__linear_vtable8,&__mac_sys_vtable8,sizeof(GFX_VTABLE));
   BlockMove(&__linear_vtable15,&__mac_sys_vtable15,sizeof(GFX_VTABLE));
   BlockMove(&__linear_vtable24,&__mac_sys_vtable24,sizeof(GFX_VTABLE));

   __mac_sys_vtable8.clear_to_color=_ppc_linear_clear_to_color8;
   __mac_sys_vtable8.rectfill=_ppc_linear_rectfill8;
   __mac_sys_vtable8.blit_to_self=_mac_sys_blit8;
   __mac_sys_vtable8.blit_to_self_forward=_mac_sys_selfblit8;
   __mac_sys_vtable8.blit_to_self_backward=_mac_sys_selfblit8;
   __mac_sys_vtable8.putpixel=_ppc_linear_putpixel8;
   __mac_sys_vtable8.hline=_ppc_linear_hline8;
   __mac_sys_vtable8.vline=_ppc_linear_vline8;

   __mac_sys_vtable15.blit_to_self=_mac_sys_blit8;
   __mac_sys_vtable15.blit_to_self_forward=_mac_sys_selfblit8;
   __mac_sys_vtable15.blit_to_self_backward=_mac_sys_selfblit8;
   __mac_sys_vtable15.clear_to_color=_ppc_linear_clear_to_color15;

   BlockMove(&__mac_sys_vtable8,&__cache_vtable8,sizeof(GFX_VTABLE));
   BlockMove(&__mac_sys_vtable15,&__cache_vtable15,sizeof(GFX_VTABLE));
   BlockMove(&__mac_sys_vtable24,&__cache_vtable24,sizeof(GFX_VTABLE));
   __cache_vtable8.clear_to_color=_ppc_linear_clear_to_color8_32;
   __cache_vtable8.hline=_ppc_linear_hline8_32;
   __cache_vtable8.rectfill=_ppc_linear_rectfill8_32;
   __cache_vtable15.clear_to_color=_ppc_linear_clear_to_color15_32;
}



/*
 *
 */
BITMAP *_mac_create_system_bitmap(int w, int h){
   BITMAP      *b;
   CGrafPtr   graf;
   GDHandle   oldDevice;
   Ptr	 data;
   unsigned char*theseBits;
   long      sizeOfOff, offRowBytes;
   short      thisDepth;
   Rect      bounds;
   int	 size;
   mac_bitmap   *mb;
   int      i;
   SetRect(&bounds,0,0,w,h);
   oldDevice = GetGDevice();
   SetGDevice(MainGDevice);
   graf = (CGrafPtr)NewPtrClear(sizeof(CGrafPort));
   if (graf != 0L){
      OpenCPort(graf);
      thisDepth = (**(*graf).portPixMap).pixelSize;
      offRowBytes = ((((long)thisDepth *(long)w) + 255L) >> 8L) << 5L;
      sizeOfOff = (long)h * offRowBytes+32;
      
      data = NewPtrClear(sizeOfOff);
      theseBits=(unsigned char*)((long)(data+31)&(-32L));
      if (theseBits != 0L){
	 (**(*graf).portPixMap).baseAddr = (char*)theseBits;
	 (**(*graf).portPixMap).rowBytes = (short)offRowBytes + 0x8000;
	 (**(*graf).portPixMap).bounds = bounds;

	 (**(*graf).portPixMap).pmTable = MainCTable;
	 ClipRect(&bounds);
	 RectRgn(graf->visRgn,&bounds);
	 ForeColor(blackColor);
	 BackColor(whiteColor);
	 EraseRect(&bounds);
      }
      else
      {
	 CloseCPort(graf);      
	 DisposePtr((Ptr)graf);
	 graf = 0L;
	 return NULL;
      }
   }
   SetGDevice(oldDevice);
   size = sizeof(mac_bitmap);
   mb = (mac_bitmap *)NewPtr(size);
   if (!mb)return NULL;
   mb->cg=graf;
   mb->rowBytes=offRowBytes;
   mb->data=data;
   mb->first=theseBits;
   mb->flags=1;
   if((long)theseBits&31){
      mb->cachealigned=0;
   }
   else{
      mb->cachealigned=1;
   }
   size=sizeof(BITMAP)+ sizeof(char *) * h;
   b=(BITMAP*)NewPtr(size);
   if(b==NULL){
      DisposePtr((Ptr)mb);return NULL;
   }
   b->w = b->cr = w;
   b->h = b->cb = h;
   b->clip = TRUE;
   b->cl = b->ct = 0;
      switch((**(*graf).portPixMap).pixelSize){
	 case 8:
	    b->vtable = &__cache_vtable8;
	    break;
	 case 16:
	    b->vtable = &__cache_vtable15;
	    break;
	 case 32:
	    b->vtable = &__cache_vtable24;
	    break;
	 default:
	    DisposePtr((Ptr)b);
	    DisposePtr((Ptr)mb);
	    b=NULL;
      }
/*   b->vtable = &__mac_sys_vtable8;*/
   b->write_bank = b->read_bank = _stub_bank_switch;
   b->dat = NULL;
   b->id = BMP_ID_SYSTEM;
   b->extra = (void*)mb;
   b->x_ofs = 0;
   b->y_ofs = 0;
   b->seg = _video_ds();
   b->line[0] = (unsigned char *)theseBits;
    for (i=1; i<h; i++)b->line[i] = b->line[i-1] + offRowBytes;
   mb->last=b->line[i-1]+w;
   return b;
}



/*
 *
 */
void _mac_destroy_system_bitmap(BITMAP *bmp){
   mac_bitmap *mbmp;
   if(bmp){
      mbmp=GETMACBITMAP(bmp);
      if(mbmp){
	 if(mbmp->flags){
	    if(mbmp->data){
	       CloseCPort(mbmp->cg);
	       DisposePtr((Ptr)mbmp->cg);
	       DisposePtr(mbmp->data);
	    }
	 }
	 DisposePtr((Ptr)mbmp);
      }
      else{
	 bmp->id&=~BMP_ID_SYSTEM;
	 destroy_bitmap(bmp);
      }
      DisposePtr((Ptr)bmp);
   }
}



/*
 *
 */
void _mac_sys_set_clip(struct BITMAP *dst){
   RgnHandle rclip;
   mac_bitmap *mdst;
   mdst=GETMACBITMAP(dst);
   SetPort((GrafPtr)mdst->cg);
   rclip=NewRgn();
   MacSetRectRgn(rclip,dst->cl,dst->ct,dst->cr,dst->cb);
   SetClip(rclip);
   DisposeRgn(rclip);      
}



/*
 *
 */
void _mac_sys_clear_to_color8 (BITMAP *bmp, int color){
   int w,h,inc,wr;
   unsigned char *p;
   float f;
      color&=0xFF;
      *(long *)(&f)=color*0x1010101;
      h=bmp->h;
      wr=bmp->w;
      p=bmp->line[0];
      inc=bmp->line[1]-bmp->line[0]-wr;
      while(h){
         w=wr;
	 while(w){
            *p++=color;            
	    w--;
	 }
	 p=p+inc;
	 h--;
   }
}




/*
 *
 */
void _mac_sys_blit8(BITMAP *src,BITMAP *dst, int src_x, int src_y, int dst_x, int dst_y, int w, int h){
   DSpBlitInfo di;
   Rect rsrc,rdst;
   mac_bitmap *msrc;
   mac_bitmap *mdst;
   msrc=GETMACBITMAP(src);
   mdst=GETMACBITMAP(dst);
   SetRect(&rsrc,src_x,src_y,src_x+w,src_y+h);
   SetRect(&rdst,dst_x,dst_y,dst_x+w,dst_y+h);
   SetPort((GrafPtr)mdst->cg);
   di.completionFlag=false;
   di.filler[0]=0;
   di.filler[1]=0;
   di.filler[2]=0;
   di.filler[3]=0;
   di.completionProc=NULL;
   di.srcContext=NULL;
   di.srcBuffer=msrc->cg;
   di.srcRect=rsrc;
   di.srcKey=0;
   di.dstContext=NULL;
   di.dstBuffer=mdst->cg;
   di.dstRect=rdst;
   di.dstKey=0;
   di.mode=0;
   di.reserved[0]=0;
   di.reserved[1]=0;
   di.reserved[2]=0;
   di.reserved[3]=0;
   DSpBlit_Fastest(&di,false);
}



/*
 *
 */
void system_stretch_blit(BITMAP *src,BITMAP *dst,int sx,int sy,int sw,int sh,int dx,int dy,int dw,int dh){
   Rect rsrc,rdst;
   RgnHandle rclip;
   mac_bitmap *msrc;
   mac_bitmap *mdst;
   msrc=GETMACBITMAP(src);
   mdst=GETMACBITMAP(dst);
   SetRect(&rsrc,sx,sy,sx+sw,sy+sh);
   SetRect(&rdst,dx,dy,dx+dw,dy+dh);
   SetPort((GrafPtr)mdst->cg);
   rclip=NewRgn();
   MacSetRectRgn(rclip,dst->cl,dst->ct,dst->cr,dst->cb);
   SetClip(rclip);
   DisposeRgn(rclip);      
   CopyBits(   &((GrafPtr)msrc->cg)->portBits,
	    &((GrafPtr)mdst->cg)->portBits,
	    &rsrc,&rdst,srcCopy,NULL);
}



/*
 *
 */
void _mac_sys_selfblit8(BITMAP *src,BITMAP *dst, int src_x, int src_y, int dst_x, int dst_y, int w, int h){
   Rect rsrc,rdst;
   mac_bitmap *msrc;
   mac_bitmap *mdst;
   msrc=GETMACBITMAP(src);
   mdst=GETMACBITMAP(dst);
   SetRect(&rsrc,src_x,src_y,src_x+w,src_y+h);
   SetRect(&rdst,dst_x,dst_y,dst_x+w,dst_y+h);
   SetPort((GrafPtr)mdst->cg);
   CopyBits(   &((GrafPtr)msrc->cg)->portBits,
	       &((GrafPtr)mdst->cg)->portBits,
	       &rsrc,&rdst,srcCopy,NULL);

}



/*
 *
 */
int _mac_sys_triangle(struct BITMAP *bmp, int x1, int y1, int x2, int y2, int x3, int y3, int color){
   int done=0;
   mac_bitmap *mbmp;
   PolyHandle triPoly;
   if(_drawing_mode==DRAW_MODE_SOLID){
      mbmp=GETMACBITMAP(bmp);
      SetPort((GrafPtr)mbmp->cg);
      RGBForeColor(&((**MainCTable).ctTable[color].rgb));
      triPoly = OpenPoly();
	 MoveTo(x1,y1);
	 LineTo(x2,y2);
	 LineTo(x3,y3);
      ClosePoly();
      PaintPoly(triPoly);
      KillPoly(triPoly);
      done=1;
   }
   return done;
}


/*
 *
 */
void _mac_sys_rectfill8(struct BITMAP *bmp, int x1, int y1, int x2, int y2, int color){
   mac_bitmap *mbmp;
   BITMAP *parent;
   Rect r;
   int t;
   if(x1>x2){t=x1;x1=x2;x2=t;}
   if(y1>y2){t=y1;y1=y2;y2=t;}
   if(bmp->clip) {
      if (x1 < bmp->cl)x1 = bmp->cl;
      if (x2 >= bmp->cr)x2 = bmp->cr-1;
      if (x2 < x1)return;
      if (y1 < bmp->ct)y1 = bmp->ct;
      if (y2 >= bmp->cb)y2 = bmp->cb-1;
      if (y2 < y1)return;
   }
   switch(_drawing_mode){
      case DRAW_MODE_SOLID:
	 SetRect(&r,x1,y1,x2+1,y2+1);
	 parent = bmp;
	 mbmp=GETMACBITMAP(bmp);
	 SetPort((GrafPtr)mbmp->cg);
	 RGBForeColor(&((**MainCTable).ctTable[color].rgb));
	 PaintRect(&r);   
	 RGBForeColor(&ForeDef);
	 break;
      default:
	 _normal_rectfill(bmp,x1,y1,x2,y2,color);
	 break;
   }
}



/*
 *
 */
void _mac_sys_hline8(struct BITMAP *bmp, int x1, int y, int x2, int color)
{
#define CALC_PATTERN()   xoff=(x1 - _drawing_x_anchor) & _drawing_x_mask;\
   sbase = _drawing_pattern->line[(y - _drawing_y_anchor) & _drawing_y_mask];\
   s_end = sbase+_drawing_x_mask+1;\
   s = sbase+xoff;\
   xoff=_drawing_x_mask+1-(((long)last-(long)p)&_drawing_x_mask)-xoff;\
   if(xoff>=0)xoff-=(_drawing_x_mask+1);\
   last1=last+xoff
      
   unsigned long d;
   unsigned char * p;
   unsigned char * last;
   unsigned char * sbase;
   unsigned char * s_end;
   unsigned char * s;
   unsigned char * last1;
   unsigned char c=color&0xFF;
   int ctr;
   UnsignedWide uw;
   register double ck;
   long xoff;
   int t;
   if(x1>x2){t=x1;x1=x2;x2=t;}
   if(bmp->clip) {
      if ((y < bmp->ct)||(y >= bmp->cb))return;
      if (x1 < bmp->cl)x1 = bmp->cl;
      if (x2 >= bmp->cr)x2 = bmp->cr-1;
      if (x2 < x1)return;
   }
   last=bmp->line[y]+x2;
   p=bmp->line[y]+x1;
   
   d=0x01010101*c;
   uw.hi=d;
   uw.lo=d;
   ck=*(double*)&uw;
   switch(_drawing_mode){
      case DRAW_MODE_SOLID:
	 while((unsigned long)p&3L&&p<last)*p++=c;
	 last-=3;
	 while(p<last){
	    *(unsigned long*)p=d;
	    p+=4;
	 }
	 last+=3;
	 while(p<last)*p++=c;
	 break;
      case DRAW_MODE_XOR:
	 while((unsigned long)p&3L&&p<last)*p++^=c;
	 last-=3;
	 while(p<last){
	    *(unsigned long*)p^=d;
	    p+=4;
	 }
	 last+=3;
	 while(p<last)*p++^=c;
	 break;
      case DRAW_MODE_TRANS:{
	    unsigned char * blender = color_map->data[(unsigned char)color];
	    do{*p++=blender[*p];}while(p<=last);
         }
         break;
      case DRAW_MODE_COPY_PATTERN:
	 CALC_PATTERN();
	 while(p<last1){
	    while(s<s_end){*p++=*s++;}
	    s = sbase;
	 }
	 while(p<last){
	    *p++=*s++;
	 }
         break;
      case DRAW_MODE_SOLID_PATTERN:
	 CALC_PATTERN();
	 while(p<last1){      
	    while(s<s_end){
	       if(*s++){
		  *p++=c;
	       }
	       else{
		  *p++=0;
	       }
	    }
	    s = sbase;
	 }
	 while(p<last){
	    if(*s++){
	       *p++=c;
	    }
	    else{
	       *p++=0;
	    }
	 }
         break;
      case DRAW_MODE_MASKED_PATTERN:
         CALC_PATTERN();
	 while(p<last1){      
	    while(s<s_end){
	       if(*s++){
		  *p=c;
	       }
	       p++;
	    }
	    s = sbase;
	 }
	 while(p<last){
	    if(*s++){
	       *p=c;
	    }
	    p++;
	 }
	 break;
      default:
	 _linear_hline8(bmp,x1,y,x2,color);
	 break;
    }
}



/*
 *
 */
void _mac_sys_vline8(struct BITMAP *bmp, int x, int y1, int y2, int color){
#define CALC_PATTERN() xoff=((x - _drawing_x_anchor) & _drawing_x_mask);\
      pinc=_drawing_pattern->line[1]-_drawing_pattern->line[0];\
      sbase = _drawing_pattern->line[0]+xoff;\
      s_end = _drawing_pattern->line[_drawing_y_mask]+xoff;\
      s = _drawing_pattern->line[((y1) - _drawing_y_anchor) & _drawing_y_mask]+xoff
      
   unsigned long inc;
   unsigned char *p;
   unsigned char *last;
   unsigned long xoff;
   unsigned long pinc;
   unsigned char * sbase;
   unsigned char * s_end;
   unsigned char * s;
   unsigned char * work;
   
   int t;
   if(y1>y2){t=y1;y1=y2;y2=t;}
   if(bmp->clip) {
      if (y1 < bmp->ct)y1 = bmp->ct;
      if (y2 >= bmp->cb)y2 = bmp->cb-1;
      if (y2 < y1)return;
      if ((x < bmp->cl)||(x >= bmp->cr))return;
   }
   t=color;
   p=bmp->line[y1]+x;
   last=bmp->line[y2]+x;
   inc=bmp->line[1]-bmp->line[0];

   switch(_drawing_mode){
      case DRAW_MODE_SOLID:
	 do{
	    *p = color;
	    p += inc;	 
	 }while(p <= last);
	 break;
      case DRAW_MODE_XOR:
	 do{*p ^= color;p += inc;}while(p <= last);
	 break;
      case DRAW_MODE_TRANS:{
            unsigned char * blender = color_map->data[(unsigned char)color];
            do{*p=blender[*p];p+=inc;}while(p<=last);
         }
	 break;
      case DRAW_MODE_COPY_PATTERN:
	 CALC_PATTERN();
	 do{
	    work=s+(long)last-(long)p;
	    s_end=(unsigned long)s_end<(unsigned long)work?s_end:work;
	    do{*p=*s;p+=inc;s+=pinc;}while(s<=s_end&&p<=last);
	    s = sbase;
	 }while(p<=last);
	 break;
      case DRAW_MODE_MASKED_PATTERN:
	 CALC_PATTERN();
	 do{
	    work=s+(long)last-(long)p;
	    s_end=(unsigned long)s_end<(unsigned long)work?s_end:work;
	    do{
	       if(*s) *p=color;
	       else *p=0;
	       p += inc; s += pinc;
	    }while(s <= s_end&&p<=last);
	    s = sbase;
	 }while(p <= last);
	 break;
      case DRAW_MODE_SOLID_PATTERN:
	 CALC_PATTERN();
	 do{
	    work=s+(long)last-(long)p;
	    s_end=(unsigned long)s_end<(unsigned long)work?s_end:work;
	    do{
	       if(*s) *p=color;
	       p+=inc;s+=pinc;
	    }while(s<=s_end&&p<=last);
	    s = sbase;
	 }while(p<=last);
	 break;  
      default:
	 _linear_vline8(bmp,x,y1,y2,color);
	 break;
   }
}




/*
 *
 */
void _mac_sys_putpixel8(BITMAP *dst, int x, int y, int color)
{
   unsigned char *p;
   unsigned char s;
   if((unsigned)_drawing_mode>DRAW_MODE_TRANS){
      _linear_putpixel8(dst,x,y,color);
      return;
   }
   if(dst->clip) {
      if (y < dst->ct)
         return;
      if (y >= dst->cb)
         return;
      if (x < dst->cl)
         return;
      if (x >= dst->cr)
         return;
   }

   p=dst->line[y]+x;
   if(_drawing_mode==DRAW_MODE_SOLID)
      *p=color;
   else if(_drawing_mode==DRAW_MODE_XOR)
      *p^=color;
   else if(_drawing_mode==DRAW_MODE_TRANS){
      *p=color_map->data[(unsigned char)color&0xFF][*p];
   }
   else{
      s = *(_drawing_pattern->line[((y) - _drawing_y_anchor) & _drawing_y_mask]
           +((x - _drawing_x_anchor) & _drawing_x_mask));
      if(_drawing_mode==DRAW_MODE_COPY_PATTERN)
         *p=s;
      else if(_drawing_mode==DRAW_MODE_MASKED_PATTERN){
         if(s) *p=color;
         else *p=0;
      }
      else /*if(_drawing_mode==DRAW_MODE_SOLID_PATTERN)*/
         if(s) *p=color;
   }
}



/*
 *
 */
void _mac_sys_clear15(struct BITMAP *bmp,int color){
   int w,h,inc,wr;
   unsigned short *p;
   float f;
      color &= 0x7FFF;
      *(long *)(&f)=color*0x10001;
      h=bmp->h;
      wr=bmp->w;
      p=(unsigned short *)bmp->line[0];
      inc=bmp->line[1]-bmp->line[0]-wr*2;
      while(h){
         w=wr;
	 while(w){
            *p++=color;
	    w--;
	 }
	 p=(unsigned short *)(((unsigned char *)p)+inc);
	 h--;
   }
}



/*
 *
 */
BITMAP *_CGrafPtr_to_system_bitmap(CGrafPtr cg){
   unsigned char *theseBits;
   long      offRowBytes;
   Rect      bounds;
   int	 size;
   BITMAP      *b=NULL;
   mac_bitmap   *mb;
   int	 i,h,w;
   GrafPtr      svcg;
   
   bounds = (*cg).portRect;
   h=bounds.bottom-bounds.top;
   w=bounds.right-bounds.left;
   theseBits = (unsigned char *)(**(*cg).portPixMap).baseAddr;
   offRowBytes = 0x7FFF&(**(*cg).portPixMap).rowBytes;
   
   size = sizeof(mac_bitmap);
   mb = (mac_bitmap *)NewPtr(size);
   
   GetPort(&svcg);
   SetPort((GrafPtr)cg);
   ForeColor(blackColor);
   BackColor(whiteColor);
   SetPort(svcg);
   if (mb){
      mb->flags=0;
      mb->cg=cg;
      mb->rowBytes=offRowBytes;
      mb->data=NULL;
      mb->first=theseBits;
      if(((long)theseBits|offRowBytes)&31)
         mb->cachealigned=0;
      else
         mb->cachealigned=1;
	 
      size=sizeof(BITMAP)+ sizeof(char *) * h;
      b=(BITMAP*)NewPtr(size);
      if(b==NULL){DisposePtr((Ptr)mb);return NULL;}
      b->w = b->cr = w;
      b->h = b->cb = h;
      b->clip = TRUE;
      b->cl = b->ct = 0;
      b->write_bank = b->read_bank = _stub_bank_switch;
      b->dat = NULL;
      b->id = BMP_ID_SYSTEM;
      b->extra = (void*)mb;
      b->x_ofs = 0;
      b->y_ofs = 0;
      b->seg = _video_ds();
      b->line[0] = theseBits;
      for (i=1; i<b->h; i++)b->line[i] = (theseBits += offRowBytes);
      mb->last=theseBits+w;
      switch((**(*cg).portPixMap).pixelSize){
	 case 8:
	    b->vtable = &__mac_sys_vtable8;
	    break;
	 case 16:
	    b->vtable = &__mac_sys_vtable15;
	    break;
	 case 32:
	    b->vtable = &__mac_sys_vtable24;
	    break;
	 default:
	    DisposePtr((Ptr)b);
	    DisposePtr((Ptr)mb);
	    b=NULL;
      }
   }
   return b;
}
