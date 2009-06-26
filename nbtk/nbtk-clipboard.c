/* nbtk-clipboard.c */

#include "nbtk-clipboard.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <clutter/x11/clutter-x11.h>
#include <string.h>

G_DEFINE_TYPE (NbtkClipboard, nbtk_clipboard, G_TYPE_OBJECT)

#define CLIPBOARD_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), NBTK_TYPE_CLIPBOARD, NbtkClipboardPrivate))

struct _NbtkClipboardPrivate
{
  Window clipboard_window;
  gchar *clipboard_text;
};

typedef struct _EventFilterData EventFilterData;
struct _EventFilterData
{
  NbtkClipboard *clipboard;
  NbtkClipboardCallbackFunc callback;
  gpointer user_data;
};

static Atom __atom_clip = None;

static void
nbtk_clipboard_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
nbtk_clipboard_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
nbtk_clipboard_dispose (GObject *object)
{
  G_OBJECT_CLASS (nbtk_clipboard_parent_class)->dispose (object);
}

static void
nbtk_clipboard_finalize (GObject *object)
{
  NbtkClipboardPrivate *priv = ((NbtkClipboard *) object)->priv;

  g_free (priv->clipboard_text);
  priv->clipboard_text = NULL;

  G_OBJECT_CLASS (nbtk_clipboard_parent_class)->finalize (object);
}

static ClutterX11FilterReturn
nbtk_clipboard_provider (XEvent        *xev,
                         ClutterEvent  *cev,
                         NbtkClipboard *clipboard)
{
  XSelectionEvent notify_event;
  XSelectionRequestEvent *req_event;

  if (xev->type != SelectionRequest)
    return CLUTTER_X11_FILTER_CONTINUE;

  req_event = &xev->xselectionrequest;

  clutter_x11_trap_x_errors ();

  XChangeProperty (req_event->display,
                   req_event->requestor,
                   req_event->property,
                   req_event->target,
                   8,
                   PropModeReplace,
                   (guchar*) clipboard->priv->clipboard_text,
                   strlen (clipboard->priv->clipboard_text));

  notify_event.type = SelectionNotify;
  notify_event.display = req_event->display;
  notify_event.requestor = req_event->requestor;
  notify_event.selection = req_event->selection;
  notify_event.target = req_event->target;
  notify_event.time = req_event->time;

  if (req_event->property == None)
    notify_event.property = req_event->target;
  else
    notify_event.property = req_event->property;

  /* notify the requestor that they have a copy of the selection */
  XSendEvent (req_event->display, req_event->requestor, False, 0,
              (XEvent *) &notify_event);
  /* Make it happen non async */
  XSync (clutter_x11_get_default_display(), FALSE);

  clutter_x11_untrap_x_errors (); /* FIXME: Warn here on fail ? */

  return CLUTTER_X11_FILTER_REMOVE;
}


static void
nbtk_clipboard_class_init (NbtkClipboardClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (NbtkClipboardPrivate));

  object_class->get_property = nbtk_clipboard_get_property;
  object_class->set_property = nbtk_clipboard_set_property;
  object_class->dispose = nbtk_clipboard_dispose;
  object_class->finalize = nbtk_clipboard_finalize;
}

static void
nbtk_clipboard_init (NbtkClipboard *self)
{
  self->priv = CLIPBOARD_PRIVATE (self);

  self->priv->clipboard_window =
    XCreateSimpleWindow (clutter_x11_get_default_display (),
                         clutter_x11_get_root_window (),
                         -1, -1, 1, 1, 0, 0, 0);

  /* Only create once */
  if (__atom_clip == None)
    __atom_clip = XInternAtom(clutter_x11_get_default_display (), 
			      "CLIPBOARD", 0);

  clutter_x11_add_filter ((ClutterX11FilterFunc) nbtk_clipboard_provider,
                          self);
}

ClutterX11FilterReturn
nbtk_clipboard_x11_event_filter (XEvent          *xev,
                                 ClutterEvent    *cev,
                                 EventFilterData *filter_data)
{
  Atom actual_type;
  int actual_format, result;
  unsigned long nitems, bytes_after;
  unsigned char *data = NULL;

  if(xev->type != SelectionNotify)
    return CLUTTER_X11_FILTER_CONTINUE;

  if (xev->xselection.property == None)
    {
      /* clipboard empty */
      filter_data->callback (filter_data->clipboard,
                             NULL,
                             filter_data->user_data);

      clutter_x11_remove_filter ((ClutterX11FilterFunc) nbtk_clipboard_x11_event_filter,
                                 filter_data);
      g_free (filter_data);
      return CLUTTER_X11_FILTER_REMOVE;
    }

  clutter_x11_trap_x_errors ();

  result = XGetWindowProperty (xev->xselection.display,
			       xev->xselection.requestor,
			       xev->xselection.property,
			       0L, G_MAXINT,
			       True,
			       AnyPropertyType,
			       &actual_type,
			       &actual_format,
			       &nitems,
			       &bytes_after,
			       &data);

  if (clutter_x11_untrap_x_errors () || result != Success)
    {
      /* FIXME: handle failure better */
      g_warning ("Clipboard: prop retrival failed");
    }

  filter_data->callback (filter_data->clipboard, (char*) data, 
			 filter_data->user_data);

  clutter_x11_remove_filter 
                    ((ClutterX11FilterFunc) nbtk_clipboard_x11_event_filter,
		     filter_data);

  g_free (filter_data);

  if (data)
    XFree (data);

  return CLUTTER_X11_FILTER_REMOVE;
}

/**
 * nbtk_clipboard_get_default:
 *
 * Get the global #NbtkClipboard object that represents the clipboard.
 *
 * Returns: a #NbtkClipboard owned by Nbtk and must not be unrefferenced or
 * freed.
 */
NbtkClipboard*
nbtk_clipboard_get_default ()
{
  static NbtkClipboard *default_clipboard = NULL;

  if (!default_clipboard)
    {
      default_clipboard = g_object_new (NBTK_TYPE_CLIPBOARD, NULL);
    }

  return default_clipboard;
}

/**
 * nbtk_clipboard_get_text:
 * @clipboard: A #NbtkCliboard
 * @callback: function to be called when the text is retreived
 * @user_data: data to be passed to the callback
 *
 * Request the data from the clipboard in text form. @callback is executed
 * when the data is retreived.
 *
 */
void
nbtk_clipboard_get_text (NbtkClipboard             *clipboard,
                         NbtkClipboardCallbackFunc  callback,
                         gpointer                   user_data)
{
  EventFilterData *data;

  Display *dpy;

  g_return_if_fail (NBTK_IS_CLIPBOARD (clipboard));
  g_return_if_fail (callback != NULL);

  data = g_new0 (EventFilterData, 1);
  data->clipboard = clipboard;
  data->callback = callback;
  data->user_data = user_data;

  clutter_x11_add_filter 
                   ((ClutterX11FilterFunc)nbtk_clipboard_x11_event_filter,
		    data);

  dpy = clutter_x11_get_default_display ();

  clutter_x11_trap_x_errors (); /* safety on */

  XConvertSelection (dpy,
                     __atom_clip,
                     XA_STRING, XA_STRING,
                     clipboard->priv->clipboard_window,
                     CurrentTime);

  clutter_x11_untrap_x_errors ();
}

/**
 * nbtk_clipboard_set_text:
 * @clipboard: A #NbtkClipboard
 * @text: text to copy to the clipboard
 *
 * Sets text as the current contents of the clipboard.
 *
 */
void
nbtk_clipboard_set_text (NbtkClipboard *clipboard,
                         const gchar   *text)
{
  NbtkClipboardPrivate *priv;
  Display *dpy;

  g_return_if_fail (NBTK_IS_CLIPBOARD (clipboard));
  g_return_if_fail (text != NULL);

  priv = clipboard->priv;

  /* make a copy of the text */
  g_free (priv->clipboard_text);
  priv->clipboard_text = g_strdup (text);

  /* tell X we own the clipboard selection */
  dpy = clutter_x11_get_default_display ();

  clutter_x11_trap_x_errors ();

  XSetSelectionOwner (dpy, __atom_clip, priv->clipboard_window, CurrentTime);
  XSync (dpy, FALSE);

  clutter_x11_untrap_x_errors ();
}
