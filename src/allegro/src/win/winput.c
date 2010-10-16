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
 *      Input management functions.
 *
 *      By Eric Botcazou.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintwin.h"

#ifndef SCAN_DEPEND
   #include <process.h>
#endif

#ifndef ALLEGRO_WINDOWS
   #error something is wrong with the makefile
#endif

#define PREFIX_I                "al-winput INFO: "
#define PREFIX_W                "al-winput WARNING: "
#define PREFIX_E                "al-winput ERROR: "


#define MAX_EVENTS 8

/* input thread event queue */
int _win_input_events;
HANDLE _win_input_event_id[MAX_EVENTS];
void (*_win_input_event_handler[MAX_EVENTS])(void);

/* pending event waiting for being processed */
static HANDLE pending_event_id;
static void (*pending_event_handler)(void);

/* internal input thread management */
static HANDLE ack_event = NULL;
static int reserved_events = 0;
static int input_need_thread = FALSE;
static HANDLE input_thread = NULL;



/* input_thread_proc: [input thread]
 *  Thread function that handles the input when there is
 *  no dedicated window thread because the library has
 *  been attached to an external window.
 */
static void input_thread_proc(LPVOID unused)
{
   int result;

   _win_thread_init();
   _TRACE(PREFIX_I "input thread starts\n");

   /* event loop */
   while (TRUE) {
      result = WaitForMultipleObjects(_win_input_events, _win_input_event_id, FALSE, INFINITE);
      if (result == WAIT_OBJECT_0 + 2)
         break;  /* thread suicide */
      else if ((result >= WAIT_OBJECT_0) && (result < WAIT_OBJECT_0 + _win_input_events))
         (*_win_input_event_handler[result - WAIT_OBJECT_0])();
   }

   _TRACE(PREFIX_I "input thread exits\n");
   _win_thread_exit();
}



/* register_pending_event: [input thread]
 *  Registers effectively the pending event.
 */
static void register_pending_event(void)
{
   /* add the pending event to the queue */
   _win_input_event_id[_win_input_events] = pending_event_id;
   _win_input_event_handler[_win_input_events] = pending_event_handler;

   /* adjust the size of the queue */
   _win_input_events++;

   /* acknowledge the registration */
   SetEvent(ack_event);
}



/* unregister_pending_event: [input thread]
 *  Unregisters effectively the pending event.
 */
static void unregister_pending_event(void)
{
   int i, found = -1;

   /* look for the pending event in the event queue */
   for (i=reserved_events; i<_win_input_events; i++) {
      if (_win_input_event_id[i] == pending_event_id) {
         found = i;
         break;
      }
   }

   if (found >= 0) {
      /* shift the queue to the left */
      for (i=found; i<_win_input_events-1; i++) {
         _win_input_event_id[i] = _win_input_event_id[i+1];
         _win_input_event_handler[i] = _win_input_event_handler[i+1];
      }

      /* adjust the size of the queue */
      _win_input_events--;
   }

   /* acknowledge the unregistration */
   SetEvent(ack_event);
}



/* _win_input_register_event: [primary thread]
 *  Adds an event to the input thread event queue.
 */
int _win_input_register_event(HANDLE event_id, void (*event_handler)(void))
{
   if (_win_input_events == MAX_EVENTS)
      return -1;

   /* record the event */
   pending_event_id = event_id;
   pending_event_handler = event_handler;

   /* create the input thread if none */
   if (input_need_thread && !input_thread)
      input_thread = (HANDLE) _beginthread(input_thread_proc, 0, NULL);

   /* ask the input thread to register the pending event */
   SetEvent(_win_input_event_id[0]);

   /* wait for the input thread to acknowledge */
   WaitForSingleObject(ack_event, INFINITE);

   _TRACE(PREFIX_I "1 input event registered (total = %d)\n", _win_input_events-reserved_events);
   return 0;
}



/* _win_input_unregister_event: [primary thread]
 *  Removes an event from the input thread event queue.
 */
void _win_input_unregister_event(HANDLE event_id)
{
   /* record the event */
   pending_event_id = event_id;

   /* ask the input thread to unregister the pending event */
   SetEvent(_win_input_event_id[1]);

   /* wait for the input thread to acknowledge */
   WaitForSingleObject(ack_event, INFINITE);

   /* kill the input thread if no more event */
   if (input_need_thread && (_win_input_events == reserved_events)) {
      SetEvent(_win_input_event_id[2]);  /* thread suicide */
      input_thread = NULL;
   }

   _TRACE(PREFIX_I "1 input event unregistered (total = %d)\n", _win_input_events-reserved_events);
}



/* _win_input_init: [primary thread]
 *  Initializes the module.
 */
void _win_input_init(int need_thread)
{
   _win_input_event_id[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
   _win_input_event_handler[0] = register_pending_event;
   _win_input_event_id[1] = CreateEvent(NULL, FALSE, FALSE, NULL);
   _win_input_event_handler[1] = unregister_pending_event;

   if (need_thread) {
      input_need_thread = TRUE;
      _win_input_event_id[2] = CreateEvent(NULL, FALSE, FALSE, NULL);
      reserved_events = 3;
   }
   else {
      input_need_thread = FALSE;
      reserved_events = 2;
   }

   _win_input_events = reserved_events;

   ack_event = CreateEvent(NULL, FALSE, FALSE, NULL);
}



/* _win_input_exit: [primary thread]
 *  Shuts down the module.
 */
void _win_input_exit(void)
{
   int i;

   for (i=0; i<reserved_events; i++)
      CloseHandle(_win_input_event_id[i]);

   _win_input_events = 0;

   CloseHandle(ack_event);
}

