#include <glib/gi18n.h>

#include "bjb-rename-note.h"

gchar *
note_title_dialog(GtkWindow *win, gchar *instructions, const gchar *current_title)
{
  GtkWidget *dialog;
  GtkWidget *area;
  GtkWidget *entry;
  gint result ;
  gchar *title = NULL ;
	
  dialog = gtk_dialog_new_with_buttons(instructions,
	                                     win,
	                                     GTK_DIALOG_MODAL| GTK_DIALOG_DESTROY_WITH_PARENT,
										                   _("_Cancel"),
	                                     GTK_RESPONSE_CANCEL,
	                                     _("_OK"),
	                                     GTK_RESPONSE_OK,
	                                     NULL);
    
  area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));    
  entry = gtk_entry_new ();

  if ( current_title )
  {
    gtk_entry_set_text (GTK_ENTRY(entry),current_title);
  }
    
  gtk_container_set_border_width (GTK_CONTAINER (area), 8);
  gtk_box_pack_start (GTK_BOX (area), entry, TRUE, TRUE,2);
  gtk_widget_show_all(area);

  result = gtk_dialog_run (GTK_DIALOG (dialog));    
  

  if ( result == GTK_RESPONSE_OK)
  {
    title= g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
  }

  gtk_widget_destroy (dialog);
  return title ;  
}

