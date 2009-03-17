/* nbtk-button.c: Plain button actor
 *
 * Copyright 2007 OpenedHand
 * Copyright 2008 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Written by: Emmanuele Bassi <ebassi@openedhand.com>
 *             Thomas Wood <thomas@linux.intel.com>
 */

/**
 * SECTION:nbtk-button
 * @short_description: Button widget
 *
 * A button widget with support for either a text label or icon, toggle mode
 * and transitions effects between states.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <clutter/clutter.h>

#include "nbtk-button.h"

#include "nbtk-marshal.h"
#include "nbtk-stylable.h"
#include "nbtk-style.h"
#include "nbtk-texture-frame.h"
#include "nbtk-texture-cache.h"
#include "nbtk-tooltip.h"
#include "nbtk-table.h"

enum
{
  PROP_0,

  PROP_LABEL,
  PROP_TOGGLE,
  PROP_ACTIVE,
  PROP_TRANSITION
};

enum
{
  CLICKED,

  LAST_SIGNAL
};

#define NBTK_BUTTON_GET_PRIVATE(obj)    \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NBTK_TYPE_BUTTON, NbtkButtonPrivate))

struct _NbtkButtonPrivate
{
  gchar *text;

  ClutterActor *label;
  ClutterActor *icon;
  NbtkWidget   *table;
  NbtkWidget   *tooltip;

  guint8 old_opacity;

  guint is_pressed : 1;
  guint is_hover : 1;
  guint is_checked : 1;
  guint is_toggle : 1;
  guint is_icon_set : 1; /* TRUE if the icon was set by the application */

  gint transition_duration;

  ClutterActor     *old_bg;
  ClutterAnimation *animation;
};

static guint button_signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE (NbtkButton, nbtk_button, NBTK_TYPE_BIN)

static void destroy_old_bg (NbtkButton *button);

static void
destroy_old_bg (NbtkButton *button)
{
  NbtkButtonPrivate *priv = button->priv;

  if (priv->old_bg)
    {
      clutter_actor_unparent (priv->old_bg);
      priv->old_bg = NULL;
    }
}

static inline void
nbtk_button_update_label_style (NbtkButton *button)
{
  NbtkButtonPrivate *priv = button->priv;
  ClutterColor *real_color = NULL;
  gchar *font_string = NULL;
  gchar *font_name = NULL;
  gint font_size = 0;

  nbtk_stylable_get (NBTK_STYLABLE (button),
                     "color", &real_color,
                     "font-family", &font_name,
                     "font-size", &font_size,
                     NULL);

  if (font_name || font_size)
    {
      if (font_name && font_size)
        font_string = g_strdup_printf ("%s %dpx", font_name, font_size);
      else
        {
          if (font_size)
            font_string = g_strdup_printf ("%dpx", font_size);
          else
            font_string = font_name;
        }

      clutter_text_set_font_name (CLUTTER_TEXT (priv->label), font_string);

      if (font_string != font_name)
        g_free (font_string);
    }

  g_free (font_name);

  if (real_color)
    {
      clutter_text_set_color (CLUTTER_TEXT (priv->label), real_color);
      clutter_color_free (real_color);
    }
}

static void
nbtk_button_style_changed (NbtkWidget *widget)
{
  NbtkButton *button = NBTK_BUTTON (widget);
  NbtkButtonPrivate *priv = button->priv;
  ClutterActor *bg_image;

  /* update the label styling */
  if (priv->label)
    nbtk_button_update_label_style (button);

  /* Remove the old background if it's around */
  destroy_old_bg (button);

  /* Store background, NbtkWidget will unparent it */
  bg_image = nbtk_widget_get_border_image (widget);
  if (bg_image)
    priv->old_bg = g_object_ref (bg_image);

  /* Chain up to update style bits */
  NBTK_WIDGET_CLASS (nbtk_button_parent_class)->style_changed (widget);
  
  /* Parent old background */
  if (priv->old_bg)
    {
      clutter_actor_set_parent (priv->old_bg, CLUTTER_ACTOR (button));
      g_object_unref (priv->old_bg);
    }

  /* Do animation */
  /* run a transition effect if applicable */
  if (NBTK_BUTTON_GET_CLASS (button)->transition)
    {
      if (NBTK_BUTTON_GET_CLASS (button)->transition (button, priv->old_bg))
        destroy_old_bg (button);
    }
  else
    {
      /* no transition, so just remove the old background */
      destroy_old_bg (button);
    }
}

static void
nbtk_button_real_pressed (NbtkButton *button)
{
  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (button), "active");
}

static void
nbtk_button_real_released (NbtkButton *button)
{
  NbtkButtonPrivate *priv = button->priv;

  if (priv->is_checked)
    nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (button), "checked");
  else if (!priv->is_hover)
    nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (button), NULL);
  else
    nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (button), "hover");

}

static gboolean
nbtk_button_button_press (ClutterActor       *actor,
                          ClutterButtonEvent *event)
{

  if (event->button == 1)
    {
      NbtkButton *button = NBTK_BUTTON (actor);
      NbtkButtonClass *klass = NBTK_BUTTON_GET_CLASS (button);

      button->priv->is_pressed = TRUE;

      clutter_grab_pointer (actor);

      if (klass->pressed)
        klass->pressed (button);

      return TRUE;
    }

  return FALSE;
}

static gboolean
nbtk_button_button_release (ClutterActor       *actor,
                            ClutterButtonEvent *event)
{
  if (event->button == 1)
    {
      NbtkButton *button = NBTK_BUTTON (actor);
      NbtkButtonClass *klass = NBTK_BUTTON_GET_CLASS (button);

      if (!button->priv->is_pressed)
        return FALSE;

      clutter_ungrab_pointer ();

      if (button->priv->is_toggle)
        {
          nbtk_button_set_checked (button, !button->priv->is_checked);
          if (button->priv->tooltip)
            nbtk_tooltip_hide (NBTK_TOOLTIP (button->priv->tooltip));
        }

      button->priv->is_pressed = FALSE;

      if (klass->released)
        klass->released (button);

      g_signal_emit (button, button_signals[CLICKED], 0);

      return TRUE;
    }

  return FALSE;
}

static gboolean
nbtk_button_enter (ClutterActor         *actor,
                   ClutterCrossingEvent *event)
{
  NbtkButton *button = NBTK_BUTTON (actor);

  if (!button->priv->is_checked)
    nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (button), "hover");

  button->priv->is_hover = 1;

  if (button->priv->tooltip)
    nbtk_tooltip_show (NBTK_TOOLTIP (button->priv->tooltip));

  return FALSE;
}

static gboolean
nbtk_button_leave (ClutterActor         *actor,
                   ClutterCrossingEvent *event)
{
  NbtkButton *button = NBTK_BUTTON (actor);

  button->priv->is_hover = 0;


  if (button->priv->is_pressed)
    {
      NbtkButtonClass *klass = NBTK_BUTTON_GET_CLASS (button);

      clutter_ungrab_pointer ();

      button->priv->is_pressed = FALSE;

      if (klass->released)
        klass->released (button);
    }

  if (button->priv->is_checked)
    nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (button), "checked");
  else
    nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (button), NULL);

  if (button->priv->tooltip)
    nbtk_tooltip_hide (NBTK_TOOLTIP (button->priv->tooltip));

  return FALSE;
}

static void
nbtk_button_set_property (GObject      *gobject,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  NbtkButton *button = NBTK_BUTTON (gobject);
  NbtkButtonPrivate *priv = NBTK_BUTTON (gobject)->priv;

  switch (prop_id)
    {
    case PROP_LABEL:
      nbtk_button_set_label (button, g_value_get_string (value));
      break;
    case PROP_TOGGLE:
      nbtk_button_set_toggle_mode (button, g_value_get_boolean (value));
      break;
    case PROP_ACTIVE:
      nbtk_button_set_checked (button, g_value_get_boolean (value));
      break;
    case PROP_TRANSITION:
      priv->transition_duration = g_value_get_int (value);
      break;


    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
nbtk_button_get_property (GObject    *gobject,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  NbtkButtonPrivate *priv = NBTK_BUTTON (gobject)->priv;

  switch (prop_id)
    {
    case PROP_LABEL:
      g_value_set_string (value, priv->text);
      break;
    case PROP_TOGGLE:
      g_value_set_boolean (value, priv->is_toggle);
      break;
    case PROP_ACTIVE:
      g_value_set_boolean (value, priv->is_checked);
      break;
    case PROP_TRANSITION:
      g_value_set_int (value, priv->transition_duration);
      break;


    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
nbtk_button_finalize (GObject *gobject)
{
  NbtkButtonPrivate *priv = NBTK_BUTTON (gobject)->priv;

  g_free (priv->text);

  G_OBJECT_CLASS (nbtk_button_parent_class)->finalize (gobject);
}

static void
nbtk_button_dispose (GObject *gobject)
{
  NbtkButton *button = NBTK_BUTTON (gobject);

  destroy_old_bg (button);

  G_OBJECT_CLASS (nbtk_button_parent_class)->dispose (gobject);
}

static void
nbtk_button_allocate (ClutterActor          *self,
                      const ClutterActorBox *box,
                      gboolean               absolute_origin_changed)
{
  NbtkButton *button = NBTK_BUTTON (self);
  NbtkButtonPrivate *priv = button->priv;

  CLUTTER_ACTOR_CLASS (nbtk_button_parent_class)->allocate (self, box, absolute_origin_changed);

  if (priv->old_bg)
    {
      ClutterActorBox frame_box = {
          0, 0, box->x2 - box->x1, box->y2 - box->y1
      };

      clutter_actor_allocate (priv->old_bg,
                              &frame_box,
                              absolute_origin_changed);
    }
}

static void
nbtk_button_draw_background (NbtkWidget         *self,
                             ClutterActor       *background,
                             const ClutterColor *color)
{
  NbtkButtonPrivate *priv = NBTK_BUTTON (self)->priv;
  NbtkWidgetClass *parent_class;

  parent_class = NBTK_WIDGET_CLASS (nbtk_button_parent_class);
  parent_class->draw_background (self, background, color);
  
  if (priv->old_bg)
    clutter_actor_paint (priv->old_bg);
}

static void
nbtk_button_hide (ClutterActor *actor)
{
  NbtkButton *button = (NbtkButton *) actor;

  /* hide the tooltip, if there is one */
  if (button->priv->tooltip)
    nbtk_tooltip_hide (NBTK_TOOLTIP (button->priv->tooltip));

  CLUTTER_ACTOR_CLASS (nbtk_button_parent_class)->hide (actor);
}

static void
nbtk_button_class_init (NbtkButtonClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  NbtkWidgetClass *nbtk_widget_class = NBTK_WIDGET_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (NbtkButtonPrivate));

  klass->pressed = nbtk_button_real_pressed;
  klass->released = nbtk_button_real_released;

  gobject_class->set_property = nbtk_button_set_property;
  gobject_class->get_property = nbtk_button_get_property;
  gobject_class->dispose = nbtk_button_dispose;
  gobject_class->finalize = nbtk_button_finalize;

  actor_class->button_press_event = nbtk_button_button_press;
  actor_class->button_release_event = nbtk_button_button_release;
  actor_class->enter_event = nbtk_button_enter;
  actor_class->leave_event = nbtk_button_leave;
  actor_class->allocate = nbtk_button_allocate;
  actor_class->hide = nbtk_button_hide;

  nbtk_widget_class->style_changed = nbtk_button_style_changed;
  nbtk_widget_class->draw_background = nbtk_button_draw_background;

  pspec = g_param_spec_string ("label",
                               "Label",
                               "Label of the button",
                               NULL, G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_LABEL, pspec);

  pspec = g_param_spec_boolean ("toggle-mode",
                                "Toggle Mode",
                                "Enable or disable toggling",
                                FALSE, G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_TOGGLE, pspec);

  pspec = g_param_spec_boolean ("checked",
                                "Checked",
                                "Indicates if a toggle button is \"on\""
                                " or \"off\"",
                                FALSE, G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_ACTIVE, pspec);

  pspec = g_param_spec_int ("transition-duration",
                            "Transition Duration",
                            "Duration of the state transition effect",
                            0, G_MAXINT, 250, G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_TRANSITION, pspec);


  /**
   * NbtkButton::clicked:
   * @button: the object that received the signal
   *
   * Emitted when the user activates the button, either with a mouse press and
   * release or with the keyboard.
   */

  button_signals[CLICKED] =
    g_signal_new ("clicked",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (NbtkButtonClass, clicked),
                  NULL, NULL,
                  _nbtk_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}

static void
nbtk_button_init (NbtkButton *button)
{
  button->priv = NBTK_BUTTON_GET_PRIVATE (button);
  button->priv->transition_duration = 250;

  button->priv->table = g_object_new (NBTK_TYPE_TABLE, "stylable", FALSE, NULL);
  clutter_container_add_actor (CLUTTER_CONTAINER (button),
                               CLUTTER_ACTOR (button->priv->table));
}

/**
 * nbtk_button_new:
 *
 * Create a new button
 *
 * Returns: a new #NbtkButton
 */
NbtkWidget *
nbtk_button_new (void)
{
  return g_object_new (NBTK_TYPE_BUTTON, NULL);
}

/**
 * nbtk_button_new_with_label:
 * @text: text to set the label to
 *
 * Create a new #NbtkButton with the specified label
 *
 * Returns: a new #NbtkButton
 */
NbtkWidget *
nbtk_button_new_with_label (const gchar *text)
{
  return g_object_new (NBTK_TYPE_BUTTON, "label", text, NULL);
}

/**
 * nbtk_button_get_label:
 * @button: a #NbtkButton
 *
 * Get the text displayed on the button
 *
 * Returns: the text for the button. This must not be freed by the application
 */
G_CONST_RETURN gchar *
nbtk_button_get_label (NbtkButton *button)
{
  g_return_val_if_fail (NBTK_IS_BUTTON (button), NULL);

  return button->priv->text;
}

/**
 * nbtk_button_set_label:
 * @button: a #Nbtkbutton
 * @text: text to set the label to
 *
 * Sets the text displayed on the button
 */
void
nbtk_button_set_label (NbtkButton  *button,
                       const gchar *text)
{
  NbtkButtonPrivate *priv;

  g_return_if_fail (NBTK_IS_BUTTON (button));

  priv = button->priv;

  g_free (priv->text);

  if (text)
    priv->text = g_strdup (text);
  else
    priv->text = g_strdup ("");

  if (priv->label)
    {
      clutter_text_set_text (CLUTTER_TEXT (priv->label), priv->text);
    }
  else
    {
      priv->label = g_object_new (CLUTTER_TYPE_TEXT,
                                  "text", priv->text,
                                  "line-alignment", PANGO_ALIGN_CENTER,
                                  "ellipsize", PANGO_ELLIPSIZE_MIDDLE,
                                  "use-markup", TRUE,
                                  NULL);
      nbtk_table_add_actor (NBTK_TABLE (priv->table), priv->label, 0, 1);
      clutter_container_child_set (CLUTTER_CONTAINER (priv->table),
                                   priv->label,
                                   "y-align", 0.5,
                                   "y-fill", FALSE,
                                   NULL);
    }


  nbtk_button_update_label_style (button);

  g_object_notify (G_OBJECT (button), "label");
}

/**
 * nbtk_button_get_toggle_mode:
 * @button: a #NbtkButton
 *
 * Get the toggle mode status of the button.
 *
 * Returns: #TRUE if toggle mode is set, otherwise #FALSE
 */
gboolean
nbtk_button_get_toggle_mode (NbtkButton *button)
{
  g_return_val_if_fail (NBTK_IS_BUTTON (button), FALSE);

  return button->priv->is_toggle;
}

/**
 * nbtk_button_set_toggle_mode:
 * @button: a #Nbtkbutton
 * @toggle: #TRUE or #FALSE
 *
 * Enables or disables toggle mode for the button. In toggle mode, the active
 * state will be "toggled" when the user clicks the button.
 */
void
nbtk_button_set_toggle_mode (NbtkButton  *button,
                             gboolean     toggle)
{
  g_return_if_fail (NBTK_IS_BUTTON (button));

  button->priv->is_toggle = toggle;

  g_object_notify (G_OBJECT (button), "toggle-mode");
}

/**
 * nbtk_button_get_checked:
 * @button: a #NbtkButton
 *
 * Get the state of the button that is in toggle mode.
 *
 * Returns: #TRUE if the button is checked, or #FALSE if not
 */
gboolean
nbtk_button_get_checked (NbtkButton *button)
{
  g_return_val_if_fail (NBTK_IS_BUTTON (button), FALSE);

  return button->priv->is_checked;
}

/**
 * nbtk_button_set_checked:
 * @button: a #Nbtkbutton
 * @checked: #TRUE or #FALSE
 *
 * Sets the pressed state of the button. This is only really useful if the
 * button has #toggle-mode mode set to #TRUE.
 */
void
nbtk_button_set_checked (NbtkButton  *button,
                         gboolean     checked)
{
  g_return_if_fail (NBTK_IS_BUTTON (button));

  if (button->priv->is_checked != checked)
    {
      button->priv->is_checked = checked;

      if (checked)
        nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (button), "checked");
      else
        if (button->priv->is_hover)
          nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (button), "hover");
        else
          nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (button), NULL);
    }

  g_object_notify (G_OBJECT (button), "checked");
}

/**
 * nbtk_button_set_icon_from_file:
 * @button: a #NbtkButton
 * @filename: path to an image to set
 *
 * Sets the icon of the button to the image specified in @filename.
 * If @filename is NULL, the current icon is removed.
 */
void
nbtk_button_set_icon_from_file (NbtkButton *button,
                                gchar      *filename)
{
  NbtkButtonPrivate *priv;
  GError *err = NULL;

  g_return_if_fail (NBTK_IS_BUTTON (button));

  priv = button->priv;

  if (filename == NULL)
    {
      if (priv->icon)
        clutter_container_remove_actor (CLUTTER_CONTAINER (priv->table),
                                        priv->icon);
      priv->is_icon_set = FALSE;
    }

  if (!priv->icon)
    {
      priv->icon = clutter_texture_new_from_file (filename, &err);
      nbtk_table_add_actor (NBTK_TABLE (priv->table), priv->icon, 0, 0);
    }
  else
    {
      clutter_texture_set_from_file (CLUTTER_TEXTURE (priv->icon), filename, &err);
    }

  if (err)
    {
      g_warning ("Failed to load icon from file. %s", err->message);
      g_error_free (err);
      return;
    }



  priv->is_icon_set = TRUE;
}

/**
 * nbtk_button_set_tooltip:
 * @button: a #NbtkButton
 * @label: text to display in the tooltip, or NULL to unset the tooltip
 *
 * Set or remove a tooltip from the button.
 */
void
nbtk_button_set_tooltip (NbtkButton  *button,
                         const gchar *label)
{
  NbtkButtonPrivate *priv;

  g_return_if_fail (NBTK_IS_BUTTON (button));

  priv = button->priv;

  if (label)
    {
      priv->tooltip = g_object_new (NBTK_TYPE_TOOLTIP,
                                    "widget", button,
                                    "label", label,
                                    NULL);
    }
  else
    g_object_unref (priv->tooltip);
}
