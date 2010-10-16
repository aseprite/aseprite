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
 *      MacOs lock unlock functions.
 *
 *      By Ronaldo H Yamada.
 *
 *      See readme.txt for copyright information.
 */

#include <allegro.h>
#include <MacMemory.h>
#include <stdlib.h>

struct lock_mem;
typedef struct lock_mem lock_mem;
struct lock_mem{
   void * address;
   unsigned long size;
   int count;
   lock_mem * next;
};

lock_mem * top=NULL;



/*
 *
 */
static void mac_unlock_all()
{
   lock_mem * temp;
   lock_mem * next;
   
   for(temp = top;temp != NULL;temp = next){
      UnholdMemory(temp->address,temp->size);
      next = temp->next;
      DisposePtr((Ptr)temp);
   }
}



/*
 *
 */
void _mac_lock(void * address,unsigned long size)
{
   static first=1;
   lock_mem * temp;
   if(first)
      atexit(mac_unlock_all);
   first=0;
   
   for(temp = top;temp != NULL;temp=temp->next){
      if(address == temp->address){
         if(size == temp->size){
	    temp->count++;
	 }
	 else{
	    UnholdMemory(temp->address,temp->size);
	    HoldMemory(address,size);
	    temp->address=address;
	    temp->size=size;
	    temp->count=1;
	 }
         return;
      }
   }
   temp=(lock_mem *)NewPtr(sizeof(lock_mem));
   if(temp){
      temp->address=address;
      temp->size=size;
      temp->count=1;
      temp->next=top;
      top=temp;
   }
   HoldMemory(address,size);
}



/*
 *
 */
void _mac_unlock(void * address,unsigned long size)
{
   lock_mem * temp;
   lock_mem * last;
   
   last=NULL;
   for(temp = top;temp != NULL;temp=temp->next){
      if(address == temp->address){
         temp->count--;
         if(temp <= 0){
	    if(last == NULL)
	       top=temp->next;
	    else
	       last->next=temp->next;
	    UnholdMemory(temp->address,temp->size);
	    DisposePtr((Ptr)temp);
	 }
         return;
      }
      else{
         last = temp;
      }
   }
}
