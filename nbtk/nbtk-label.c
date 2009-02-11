/* nbtk-label.c: Plain label actor
 *
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
 * Written by: Thomas Wood <thomas@linux.intel.com>
 */

/**
 * SECTION:nbtk-label
 * @short_description: Widget for displaying text
 *
 * #NbtkLabel is a simple widget for displaying text. It derives from
 * #NbtkWidget to add extra style and placement functionality over
 * #ClutterText. The internal #ClutterText is publicly accessibly to allow
 * applications to set further properties. 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <clutter/clutter.h>

#include "nbtk-label.h"

#include "nbtk-widget.h"
#include "nbtk-stylable.h"

enum
{
  PROP_0,

  PROP_LABEL
};

#define NBTK_LABEL_GET_PRIVATE(obj)     (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NBTK_TYPE_LABEL, NbtkLabelPrivate))

struct _NbtkLabelPrivate
{
  ClutterActor *label;

  NbtkPadding padding;
};

static void nbtk_stylable_iface_init (NbtkStylableIface *iface);

G_DEFINE_TYPE_WITH_CODE (NbtkLabel, nbtk_label, NBTK_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (NBTK_TYPE_STYLABLE,
                                                nbtk_stylable_iface_init));

static void
nbtk_label_set_property (GObject      *gobject,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  NbtkLabel *label = NBTK_LABEL (gobject);

  switch (prop_id)
    {
    case PROP_LABEL:
      nbtk_label_set_text (label, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
nbtk_label_get_property (GObject    *gobject,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  NbtkLabelPrivate *priv = NBTK_LABEL (gobject)->priv;

  switch (prop_id)
    {
    case PROP_LABEL:
      g_value_set_string (value, clutter_text_get_text (CLUTTER_TEXT (priv->label)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
nbtk_label_style_changed (NbtkWidget *self)
{
  NbtkLabelPrivate *priv = NBTK_LABEL (self)->priv;
  NbtkPadding *padding = NULL;
  ClutterColor *color = NULL;
  gchar *font_name;
  gchar *font_string;
  gint font_size;

  nbtk_stylable_get (NBTK_STYLABLE (self),
                     "color", &color,
                     "font-family", &font_name,
                     "font-size", &font_size,
                     "padding", &padding,
                     NULL);

  if (color)
    {
      clutter_text_set_color (CLUTTER_TEXT (priv->label), color);
      clutter_color_free (color);
    }

  if (padding)
    {
      priv->padding = *padding;
      g_boxed_free (NBTK_TYPE_PADDING, padding);
    }

  if (font_name || font_size)
    {
      if (font_name && font_size)
        {
          font_string = g_strdup_printf ("%s %dpx", font_name, font_size);
          g_free (font_name);
        }
      else
        {
          if (font_size)
            font_string = g_strdup_printf ("%dpx", font_size);
          else
            font_string = font_name;
        }

      clutter_text_set_font_name (CLUTTER_TEXT (priv->label), font_string);
      g_free (font_string);
    }

  if (NBTK_WIDGET_CLASS (nbtk_label_parent_class)->style_changed)
    NBTK_WIDGET_CLASS (nbtk_label_parent_class)->style_changed (self);
}

static void
nbtk_label_get_preferred_width (ClutterActor *actor,
                                ClutterUnit   for_height,
                                ClutterUnit  *min_width_p,
                                ClutterUnit  *natural_width_p)
{
  NbtkLabelPrivate *priv = NBTK_LABEL (actor)->priv;

  clutter_actor_get_preferred_width (priv->label, for_height,
                                     min_width_p,
                                     natural_width_p);

  if (min_width_p)
    *min_width_p += priv->padding.left + priv->padding.right;

  if (natural_width_p)
    *natural_width_p += priv->padding.left + priv->padding.right;
}

static void
nbtk_label_get_preferred_height (ClutterActor *actor,
                                 ClutterUnit   for_width,
                                 ClutterUnit  *min_height_p,
                                 ClutterUnit  *natural_height_p)
{
  NbtkLabelPrivate *priv = NBTK_LABEL (actor)->priv;

  clutter_actor_get_preferred_height (priv->label, for_width,
                                      min_height_p,
                                      natural_height_p);

  if (min_height_p)
    *min_height_p += priv->padding.top + priv->padding.bottom;

  if (natural_height_p)
    *natural_height_p += priv->padding.top + priv->padding.bottom;
}

static void
nbtk_label_allocate (ClutterActor          *actor,
                     const ClutterActorBox *box,
                     gboolean               absolute_origin_changed)
{
  NbtkLabelPrivate *priv = NBTK_LABEL (actor)->priv;
  ClutterActorClass *parent_class;
  ClutterActorBox child_box;

  parent_class = CLUTTER_ACTOR_CLASS (nbtk_label_parent_class);
  parent_class->allocate (actor, box, absolute_origin_changed);

  child_box.x1 = priv->padding.left;
  child_box.y1 = priv->padding.top;
  child_box.x2 = box->x2 - box->x1 - priv->padding.right;
  child_box.y2 = box->y2 - box->y1 - priv->padding.bottom;

  clutter_actor_allocate (priv->label, &child_box, absolute_origin_changed);
}

static void
nbtk_label_paint (ClutterActor *actor)
{
  NbtkLabelPrivate *priv = NBTK_LABEL (actor)->priv;
  ClutterActorClass *parent_class;

  parent_class = CLUTTER_ACTOR_CLASS (nbtk_label_parent_class);
  parent_class->paint (actor);

  clutter_actor_paint (priv->label);
}

static void
nbtk_stylable_iface_init (NbtkStylableIface *iface)
{
  static gboolean is_initialized = FALSE;

  if (G_UNLIKELY (!is_initialized))
    {
      GParamSpec *pspec;

      pspec = g_param_spec_boxed ("padding",
                                  "Padding",
                                  "Padding between the widgets borders "
                                  "and its content",
                                  NBTK_TYPE_PADDING,
                                  G_PARAM_READWRITE);
      nbtk_stylable_iface_install_property (iface, NBTK_TYPE_LABEL, pspec);
    }
}

static void
nbtk_label_class_init (NbtkLabelClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  NbtkWidgetClass *widget_class = NBTK_WIDGET_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (NbtkLabelPrivate));

  gobject_class->set_property = nbtk_label_set_property;
  gobject_class->get_property = nbtk_label_get_property;

  actor_class->paint = nbtk_label_paint;
  actor_class->allocate = nbtk_label_allocate;
  actor_class->get_preferred_width = nbtk_label_get_preferred_width;
  actor_class->get_preferred_height = nbtk_label_get_preferred_height;

  widget_class->style_changed = nbtk_label_style_changed;

  pspec = g_param_spec_string ("text",
                               "Text",
                               "Text of the label",
                               NULL, G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_LABEL, pspec);

}

static void
nbtk_label_init (NbtkLabel *label)
{
  NbtkLabelPrivate *priv;

  label->priv = priv = NBTK_LABEL_GET_PRIVATE (label);

  priv->label = g_object_new (CLUTTER_TYPE_TEXT,
                              "line-alignment", PANGO_ALIGN_CENTER,
                              "ellipsize", PANGO_ELLIPSIZE_MIDDLE,
                              "use-markup", TRUE,
                              NULL);
  clutter_actor_set_parent (priv->label, CLUTTER_ACTOR (label));
}

/**
 * nbtk_label_new:
 * @text: text to set the label to
 *
 * Create a new #NbtkLabel with the specified label
 *
 * Returns: a new #NbtkLabel
 */
NbtkWidget *
nbtk_label_new (const gchar *text)
{
  return g_object_new (NBTK_TYPE_LABEL,
                       "text", (text != NULL && *text != '\0') ? text : "",
                       NULL);
}

/**
 * nbtk_label_get_text:
 * @label: a #NbtkLabel
 *
 * Get the text displayed on the label
 *
 * Returns: the text for the label. This must not be freed by the application
 */
G_CONST_RETURN gchar *
nbtk_label_get_text (NbtkLabel *label)
{
  g_return_val_if_fail (NBTK_IS_LABEL (label), NULL);

  return clutter_text_get_text (CLUTTER_TEXT (label->priv->label));
}

/**
 * nbtk_label_set_text:
 * @label: a #NbtkLabel
 * @text: text to set the label to
 *
 * Sets the text displayed on the label
 */
void
nbtk_label_set_text (NbtkLabel *label,
                     const gchar *text)
{
  NbtkLabelPrivate *priv;

  g_return_if_fail (NBTK_IS_LABEL (label));
  g_return_if_fail (text != NULL);

  priv = label->priv;

  clutter_text_set_text (CLUTTER_TEXT (priv->label), text);

  g_object_notify (G_OBJECT (label), "text");
}

/**
 * nbtk_label_get_clutter_text:
 * @label: a #NbtkLabel
 *
 * Retrieve the internal #ClutterText so that extra parameters can be set
 *
 * Returns: the #ClutterText used by #NbtkLabel. The label is owned by the
 * #NbtkLabel and should not be unref'ed by the application.
 */
ClutterActor*
nbtk_label_get_clutter_text (NbtkLabel *label)
{
  g_return_val_if_fail (NBTK_LABEL (label), NULL);

  return label->priv->label;
}
