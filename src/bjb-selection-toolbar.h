/*
 * Photos - access, organize and share your photos on GNOME
 * Copyright © 2012 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifndef BJB_SELECTION_TOOLBAR_H
#define BJB_SELECTION_TOOLBAR_H

#include <libgd/gd.h>

G_BEGIN_DECLS

#define BJB_TYPE_SELECTION_TOOLBAR (bjb_selection_toolbar_get_type ())

#define BJB_SELECTION_TOOLBAR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), BJB_TYPE_SELECTION_TOOLBAR, BjbSelectionToolbar))

#define BJB_SELECTION_TOOLBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), BJB_TYPE_SELECTION_TOOLBAR, BjbSelectionToolbarClass))

#define BJB_IS_SELECTION_TOOLBAR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BJB_TYPE_SELECTION_TOOLBAR))

#define BJB_IS_SELECTION_TOOLBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BJB_TYPE_SELECTION_TOOLBAR))

#define BJB_SELECTION_TOOLBAR_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), BJB_TYPE_SELECTION_TOOLBAR, BjbSelectionToolbarClass))

typedef struct _BjbSelectionToolbar        BjbSelectionToolbar;
typedef struct _BjbSelectionToolbarClass   BjbSelectionToolbarClass;
typedef struct _BjbSelectionToolbarPrivate BjbSelectionToolbarPrivate;

struct _BjbSelectionToolbar
{
  GtkToolbar parent_instance;
  BjbSelectionToolbarPrivate *priv;
};

struct _BjbSelectionToolbarClass
{
  GtkToolbarClass parent_class;
};

GType bjb_selection_toolbar_get_type (void) G_GNUC_CONST;

BjbSelectionToolbar * bjb_selection_toolbar_new (GdMainView   *selection,
                                                 BjbMainView  *bjb_main_view);

G_END_DECLS

#endif /* BJB_SELECTION_TOOLBAR_H */
