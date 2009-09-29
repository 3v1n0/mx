/*
 * mx-subtexture.h: Class to wrap a texture and "subframe" it.
 * based on
 * mx-texture-frame.c: Expandible texture actor
 *
 * Copyright 2007 OpenedHand
 * Copyright 2009 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * Boston, MA 02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cogl/cogl.h>
#include <clutter/clutter.h>

#include "mx-subtexture.h"

enum
{
  PROP_0,

  PROP_PARENT_TEXTURE,

  PROP_TOP,
  PROP_LEFT,
  PROP_WIDTH,
  PROP_HEIGHT
};

G_DEFINE_TYPE (MxSubtexture, mx_subtexture, CLUTTER_TYPE_ACTOR);

#define MX_SUBTEXTURE_GET_PRIVATE(obj)     (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MX_TYPE_SUBTEXTURE, MxSubtexturePrivate))

struct _MxSubtexturePrivate
{
  ClutterTexture *parent_texture;

  int             left;
  int             top;
  int             width;
  int             height;

  CoglHandle      material;
};

static void
mx_subtexture_get_preferred_width (ClutterActor *self,
                                   gfloat        for_height,
                                   gfloat       *min_width_p,
                                   gfloat       *natural_width_p)
{
  MxSubtexturePrivate *priv = MX_SUBTEXTURE (self)->priv;

  if (G_UNLIKELY (priv->parent_texture == NULL))
    {
      if (min_width_p)
        *min_width_p = 0;

      if (natural_width_p)
        *natural_width_p = 0;
    }
  else
    {
      if (min_width_p)
        *min_width_p = priv->width;
      if (natural_width_p)
        *natural_width_p = priv->width;
    }
}

static void
mx_subtexture_get_preferred_height (ClutterActor *self,
                                    gfloat        for_width,
                                    gfloat       *min_height_p,
                                    gfloat       *natural_height_p)
{
  MxSubtexturePrivate *priv = MX_SUBTEXTURE (self)->priv;

  if (G_UNLIKELY (priv->parent_texture == NULL))
    {
      if (min_height_p)
        *min_height_p = 0;

      if (natural_height_p)
        *natural_height_p = 0;
    }
  else
    {
      if (min_height_p)
        *min_height_p = priv->height;
      if (natural_height_p)
        *natural_height_p = priv->height;
    }
}

static void
mx_subtexture_realize (ClutterActor *self)
{
  MxSubtexturePrivate *priv = MX_SUBTEXTURE (self)->priv;

  if (priv->material != COGL_INVALID_HANDLE)
    return;

  priv->material = cogl_material_new ();

  CLUTTER_ACTOR_SET_FLAGS (self, CLUTTER_ACTOR_REALIZED);
}

static void
mx_subtexture_unrealize (ClutterActor *self)
{
  MxSubtexturePrivate *priv = MX_SUBTEXTURE (self)->priv;

  if (priv->material == COGL_INVALID_HANDLE)
    return;

  cogl_material_unref (priv->material);
  priv->material = COGL_INVALID_HANDLE;

  CLUTTER_ACTOR_UNSET_FLAGS (self, CLUTTER_ACTOR_REALIZED);
}

static void
mx_subtexture_paint (ClutterActor *self)
{
  MxSubtexturePrivate *priv = MX_SUBTEXTURE (self)->priv;
  CoglHandle cogl_texture = COGL_INVALID_HANDLE;
  ClutterActorBox box = { 0, 0, 0, 0 };
  gfloat tx1, ty1, tx2, ty2, tex_width, tex_height, width, height;
  guint8 opacity;

  /* no need to paint stuff if we don't have a texture */
  if (G_UNLIKELY (priv->parent_texture == NULL))
    return;

  /* parent texture may have been hidden, so need to make sure it gets
   * realized
   */
  if (!CLUTTER_ACTOR_IS_REALIZED (priv->parent_texture))
    clutter_actor_realize (CLUTTER_ACTOR (priv->parent_texture));

  cogl_texture = clutter_texture_get_cogl_texture (priv->parent_texture);
  if (cogl_texture == COGL_INVALID_HANDLE)
    return;

  tex_width  = cogl_texture_get_width (cogl_texture);
  tex_height = cogl_texture_get_height (cogl_texture);

  clutter_actor_get_allocation_box (self, &box);

  width = box.x2 - box.x1;
  height = box.y2 - box.y1;

  tx1 = 1.0 *  priv->left / tex_width;
  ty1 = 1.0 * priv->top / tex_height;

  tx2 = 1.0 * (priv->left + priv->width) / tex_width;
  ty2 = 1.0 * (priv->top + priv->height) / tex_height;


  opacity = clutter_actor_get_paint_opacity (self);

  g_assert (priv->material != COGL_INVALID_HANDLE);

  /* set the source material using the parent texture's COGL handle */
  cogl_material_set_color4ub (priv->material, 255, 255, 255, opacity);
  cogl_material_set_layer (priv->material, 0, cogl_texture);
  cogl_set_source (priv->material);

  cogl_rectangle_with_texture_coords (0,0, (float) width, (float) height,
                                      tx1, ty1, tx2, ty2);
}

static inline void
mx_subtexture_set_frame_internal (MxSubtexture *frame,
                                  int           left,
                                  int           top,
                                  int           width,
                                  int           height)
{
  MxSubtexturePrivate *priv = frame->priv;
  GObject *gobject = G_OBJECT (frame);
  gboolean changed = FALSE;

  g_object_freeze_notify (gobject);

  if (priv->top != top)
    {
      priv->top = top;
      g_object_notify (gobject, "top");
      changed = TRUE;
    }

  if (priv->left != left)
    {
      priv->left = left;
      g_object_notify (gobject, "left");
      changed = TRUE;
    }

  if (priv->width != width)
    {
      priv->width = width;
      g_object_notify (gobject, "width");
      changed = TRUE;
    }

  if (priv->height != height)
    {
      priv->height = height;
      g_object_notify (gobject, "height");
      changed = TRUE;
    }

  if (changed && CLUTTER_ACTOR_IS_VISIBLE (frame))
    clutter_actor_queue_redraw (CLUTTER_ACTOR (frame));

  g_object_thaw_notify (gobject);
}

static void
mx_subtexture_set_property (GObject      *gobject,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  MxSubtexture *frame = MX_SUBTEXTURE (gobject);
  MxSubtexturePrivate *priv = frame->priv;

  switch (prop_id)
    {
    case PROP_PARENT_TEXTURE:
      mx_subtexture_set_parent_texture (frame,
                                        g_value_get_object (value));
      break;

    case PROP_TOP:
      mx_subtexture_set_frame_internal (frame,
                                        priv->left,
                                        g_value_get_int (value),
                                        priv->width,
                                        priv->height);
      break;

    case PROP_LEFT:
      mx_subtexture_set_frame_internal (frame,
                                        g_value_get_int (value),
                                        priv->top,
                                        priv->width,
                                        priv->height);
      break;

    case PROP_WIDTH:
      mx_subtexture_set_frame_internal (frame,
                                        priv->left,
                                        priv->top,
                                        g_value_get_int (value),
                                        priv->height);
      break;

    case PROP_HEIGHT:
      mx_subtexture_set_frame_internal (frame,
                                        priv->left,
                                        priv->top,
                                        priv->width,
                                        g_value_get_int (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mx_subtexture_get_property (GObject    *gobject,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  MxSubtexturePrivate *priv = MX_SUBTEXTURE (gobject)->priv;

  switch (prop_id)
    {
    case PROP_PARENT_TEXTURE:
      g_value_set_object (value, priv->parent_texture);
      break;

    case PROP_LEFT:
      g_value_set_int (value, priv->left);
      break;

    case PROP_TOP:
      g_value_set_int (value, priv->top);
      break;

    case PROP_WIDTH:
      g_value_set_int (value, priv->width);
      break;

    case PROP_HEIGHT:
      g_value_set_int (value, priv->height);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mx_subtexture_dispose (GObject *gobject)
{
  MxSubtexturePrivate *priv = MX_SUBTEXTURE (gobject)->priv;

  if (priv->parent_texture)
    {
      g_object_unref (priv->parent_texture);
      priv->parent_texture = NULL;
    }

  if (priv->material)
    {
      cogl_material_unref (priv->material);
      priv->material = COGL_INVALID_HANDLE;
    }

  G_OBJECT_CLASS (mx_subtexture_parent_class)->dispose (gobject);
}

static void
mx_subtexture_class_init (MxSubtextureClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (gobject_class, sizeof (MxSubtexturePrivate));

  actor_class->get_preferred_width =
    mx_subtexture_get_preferred_width;
  actor_class->get_preferred_height =
    mx_subtexture_get_preferred_height;
  actor_class->realize = mx_subtexture_realize;
  actor_class->unrealize = mx_subtexture_unrealize;
  actor_class->paint = mx_subtexture_paint;

  gobject_class->set_property = mx_subtexture_set_property;
  gobject_class->get_property = mx_subtexture_get_property;
  gobject_class->dispose = mx_subtexture_dispose;

  pspec = g_param_spec_object ("parent-texture",
                               "Parent Texture",
                               "The parent ClutterTexture",
                               CLUTTER_TYPE_TEXTURE,
                               G_PARAM_READWRITE |
                               G_PARAM_CONSTRUCT);
  g_object_class_install_property (gobject_class, PROP_PARENT_TEXTURE, pspec);

  pspec = g_param_spec_int ("left",
                            "Left",
                            "Left offset",
                            0, G_MAXINT,
                            0,
                            G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_LEFT, pspec);

  pspec = g_param_spec_int ("top",
                            "Top",
                            "Top offset",
                            0, G_MAXINT,
                            0,
                            G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_TOP, pspec);

  pspec = g_param_spec_int ("width",
                            "Width",
                            "Width",
                            0, G_MAXINT,
                            0,
                            G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_WIDTH, pspec);

  pspec = g_param_spec_int ("height",
                            "Height",
                            "Height",
                            0, G_MAXINT,
                            0,
                            G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_HEIGHT, pspec);
}

static void
mx_subtexture_init (MxSubtexture *self)
{
  MxSubtexturePrivate *priv;

  self->priv = priv = MX_SUBTEXTURE_GET_PRIVATE (self);

  priv->material = COGL_INVALID_HANDLE;
}

/**
 * mx_subtexture_new:
 * @texture: a #ClutterTexture or %NULL
 * @left: left
 * @top: top
 * @width: width
 * @height: height
 *
 * A #MxSubtexture is a specialized texture that efficiently clones
 * an area of the given @texture while keeping preserving portions of the
 * same texture.
 *
 * A #MxSubtexture can be used to make a rectangular texture fit a
 * given size without stretching its borders.
 *
 * Return value: the newly created #MxSubtexture
 */
ClutterActor*
mx_subtexture_new (ClutterTexture *texture,
                   gint            left,
                   gint            top,
                   gint            width,
                   gint            height)
{
  g_return_val_if_fail (texture == NULL || CLUTTER_IS_TEXTURE (texture), NULL);

  return g_object_new (MX_TYPE_SUBTEXTURE,
                       "parent-texture", texture,
                       "top", top,
                       "left", left,
                       "width", width,
                       "height", height,
                       NULL);
}

/**
 * mx_subtexture_get_parent_texture:
 * @frame: A #MxSubtexture
 *
 * Return the texture used by the #MxSubtexture
 *
 * Returns: a #ClutterTexture owned by the #MxSubtexture
 */
ClutterTexture *
mx_subtexture_get_parent_texture (MxSubtexture *frame)
{
  g_return_val_if_fail (MX_IS_SUBTEXTURE (frame), NULL);

  return frame->priv->parent_texture;
}

/**
 * mx_subtexture_set_parent_texture:
 * @frame: A #MxSubtexture
 * @texture: A #ClutterTexture
 *
 * Set the #ClutterTexture used by this #MxSubtexture
 *
 */
void
mx_subtexture_set_parent_texture (MxSubtexture   *frame,
                                  ClutterTexture *texture)
{
  MxSubtexturePrivate *priv;
  gboolean was_visible;

  g_return_if_fail (MX_IS_SUBTEXTURE (frame));
  g_return_if_fail (texture == NULL || CLUTTER_IS_TEXTURE (texture));

  priv = frame->priv;

  was_visible = CLUTTER_ACTOR_IS_VISIBLE (frame);

  if (priv->parent_texture == texture)
    return;

  if (priv->parent_texture)
    {
      g_object_unref (priv->parent_texture);
      priv->parent_texture = NULL;

      if (was_visible)
        clutter_actor_hide (CLUTTER_ACTOR (frame));
    }

  if (texture)
    {
      priv->parent_texture = g_object_ref (texture);

      if (was_visible && CLUTTER_ACTOR_IS_VISIBLE (priv->parent_texture))
        clutter_actor_show (CLUTTER_ACTOR (frame));
    }

  clutter_actor_queue_relayout (CLUTTER_ACTOR (frame));

  g_object_notify (G_OBJECT (frame), "parent-texture");
}

/**
 * mx_subtexture_set_frame:
 * @frame: A #MxSubtexture
 * @left: left
 * @top: top
 * @width: width
 * @height: height
 *
 * Set the frame of the subtexture
 *
 */
void
mx_subtexture_set_frame (MxSubtexture *frame,
                         gint          left,
                         gint          top,
                         gint          width,
                         gint          height)
{
  g_return_if_fail (MX_IS_SUBTEXTURE (frame));

  mx_subtexture_set_frame_internal (frame, left, top, width, height);
}

/**
 * mx_subtexture_get_frame:
 * @frame: A #MxSubtexture
 * @left: left
 * @top: top
 * @width: width
 * @height: height
 *
 * Retrieve the current frame.
 *
 */
void
mx_subtexture_get_frame (MxSubtexture *frame,
                         gint         *left,
                         gint         *top,
                         gint         *width,
                         gint         *height)
{
  MxSubtexturePrivate *priv;

  g_return_if_fail (MX_IS_SUBTEXTURE (frame));

  priv = frame->priv;

  if (top)
    *top = priv->top;

  if (left)
    *left = priv->left;

  if (width)
    *width = priv->width;

  if (height)
    *height = priv->height;
}
