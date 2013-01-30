/* biji-tracker.h
 * Copyright (C) Pierre-Yves LUYTEN 2012 <py@luyten.fr>
 * 
 * bijiben is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * bijiben is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef _BIJI_TRACKER_H
#define _BIJI_TRACKER_H

#include <gtk/gtk.h>
#include <tracker-sparql.h>

#include <libbiji/libbiji.h>

/* todo : find this on glib */
typedef void (*BijiFunc) (gpointer user_data);

/* All notes matching (either content or tag) */
GList * biji_get_notes_with_strings_or_tag_finish (GObject *source_object, GAsyncResult *res, BijiNoteBook *book);
void biji_get_notes_with_string_or_tag_async (gchar *needle, GAsyncReadyCallback f, gpointer user_data);

/* Get tags */
GList * biji_get_all_tags_finish (GObject *source_object, GAsyncResult *res);

void biji_get_all_tracker_tags_async (GAsyncReadyCallback f, gpointer user_data);

void push_tag_to_tracker (const gchar *tag, BijiFunc afterward, gpointer user_data);

void remove_tag_from_tracker(gchar *tag);

void push_existing_or_new_tag_to_note (gchar *tag,BijiNoteObj *note);

void remove_tag_from_note (gchar *tag, BijiNoteObj *note) ;

/* Insert or update */
void bijiben_push_note_to_tracker(BijiNoteObj *note);

void biji_note_delete_from_tracker(BijiNoteObj *note);

#endif /*_BIJI_TRACKER_H*/
