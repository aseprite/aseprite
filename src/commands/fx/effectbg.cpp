/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "jinete/jinete.h"

#include "app.h"
#include "base/mutex.h"
#include "base/scoped_lock.h"
#include "core/cfg.h"
#include "effect/effect.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "raster/sprite.h"
#include "widgets/editor.h"
#include "widgets/statebar.h"

/**********************************************************************
 Apply effect in two threads: bg-thread to modify the sprite, and the
 main thread to monitoring the progress.
 **********************************************************************/

struct ThreadData
{
  Effect* effect;		/* effect to be applied */
  Mutex mutex;			/* mutex to access to 'pos', 'done'
				   and 'cancelled' fields in different
				   threads */
  float pos;			/* current progress position */
  bool done : 1;		/* was the effect completelly applied? */
  bool cancelled : 1;		/* was the effect cancelled by the user?  */
  Monitor *monitor;		/* monitor to update the progress-bar */
  Progress *progress;		/* the progress-bar */
  JThread thread;		/* thread to apply the effect in background */
  Frame* alert_window;		/* alert for the user to cancel the
				   effect-progress if he wants */
};

/**
 * Called by @ref effect_apply to informate the progress of the
 * effect.
 * 
 * [effect thread]
 */
static void effect_progress_hook(void *_data, float progress)
{
  ThreadData *data = (ThreadData *)_data;

  ScopedLock lock(data->mutex);
  data->pos = progress;
}

/**
 * Called by @ref effect_apply to know if the user cancelled the
 * operation.
 * 
 * [effect thread]
 */
static bool effect_is_cancelled_hook(void *_data)
{
  ThreadData *data = (ThreadData *)_data;
  bool cancelled;

  ScopedLock lock(data->mutex);
  cancelled = data->cancelled;

  return cancelled;
}

/**
 * Applies the effect to the sprite in a background thread.
 * 
 * [effect thread]
 */
static void effect_bg(void *_data)
{
  ThreadData *data = (ThreadData *)_data;

  /* apply the effect */
  effect_apply_to_target(data->effect);

  /* mark the work as 'done' */
  ScopedLock lock(data->mutex);
  data->done = true;
}

/**
 * Called by the gui-monitor (a timer in the gui module that is called
 * every 100 milliseconds).
 * 
 * [main thread]
 */
static void monitor_effect_bg(void *_data)
{
  ThreadData *data = (ThreadData *)_data;
  float pos;
  bool done;

  {
    ScopedLock lock(data->mutex);
    pos = data->pos;
    done = data->done;
  }

  if (data->progress)
    data->progress->setPos(pos);

  if (data->done)
    remove_gui_monitor(data->monitor);
}

/**
 * Called to destroy the data of the monitor.
 * 
 * [main thread]
 */
static void monitor_free(void *_data)
{
  ThreadData *data = (ThreadData *)_data;

  if (data->alert_window != NULL)
    data->alert_window->closeWindow(NULL);
}

/**
 * Applies the effect in a background thread meanwhile the progress
 * bar is shown to the user.
 * 
 * [main thread]
 */
void effect_apply_to_target_with_progressbar(Effect* effect)
{
  ThreadData* data = new ThreadData;

  effect->progress_data = data;
  effect->progress = effect_progress_hook;
  effect->is_cancelled = effect_is_cancelled_hook;

  data->effect = effect;
  data->pos = 0.0;
  data->done = false;
  data->cancelled = false;
  data->progress = app_get_statusbar()->addProgress();
  data->thread = jthread_new(effect_bg, data);
  data->alert_window = jalert_new(PACKAGE
				  "<<Applying effect...||&Cancel");
  data->monitor = add_gui_monitor(monitor_effect_bg,
				  monitor_free, data);

  /* TODO error handling */

  data->alert_window->open_window_fg();

  {
    ScopedLock lock(data->mutex);
    if (!data->done) {
      remove_gui_monitor(data->monitor);
      data->cancelled = true;
    }
  }

  /* wait the `effect_bg' thread */
  jthread_join(data->thread);

  delete data->progress;
  jwidget_free(data->alert_window);
  delete data;
}
