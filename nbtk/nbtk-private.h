/*
 * nbtk-private.h: Private declarations
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

#ifndef __NBTK_PRIVATE_H__
#define __NBTK_PRIVATE_H__

#include <glib.h>
#include "nbtk-widget.h"
#include "nbtk-bin.h"
#include "nbtk-table.h"

G_BEGIN_DECLS

#define I_(str)         (g_intern_static_string ((str)))

#define NBTK_PARAM_READABLE     \
        (G_PARAM_READABLE |     \
         G_PARAM_STATIC_NICK | G_PARAM_STATIC_NAME | G_PARAM_STATIC_BLURB)

#define NBTK_PARAM_READWRITE    \
        (G_PARAM_READABLE | G_PARAM_WRITABLE | \
         G_PARAM_STATIC_NICK | G_PARAM_STATIC_NAME | G_PARAM_STATIC_BLURB)

G_END_DECLS

ClutterActor *_nbtk_widget_get_dnd_clone (NbtkWidget *widget);
void _nbtk_bin_get_align_factors (NbtkBin *bin, gdouble *x_align, gdouble *y_align);

/* used by NbtkTableChild to update row/column count */
void _nbtk_table_update_row_col (NbtkTable *table, gint row, gint col);

#endif /* __NBTK_PRIVATE_H__ */
