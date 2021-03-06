
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <stdlib.h>

#include <libbiji/libbiji.h>
#include <libgd/gd.h>

#include "bjb-bijiben.h"
#include "bjb-empty-results-box.h"
#include "bjb-window-base.h"
#include "bjb-main-toolbar.h"
#include "bjb-main-view.h"
#include "bjb-note-view.h"


#define BIJIBEN_MAIN_WIN_TITLE N_("Notes")

#define BJB_DEFAULT_FONT "Serif 10"


enum {
  PROP_0,
  PROP_NOTE,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };


enum {
  BJB_WIN_BASE_VIEW_CHANGED,
  BJB_WIN_BASE_ACTIVATED,
  BJB_WIN_BASE_SIGNALS
};

static guint bjb_win_base_signals [BJB_WIN_BASE_SIGNALS] = { 0 };


struct _BjbWindowBasePriv
{
  BjbSettings          *settings;
  BjbController        *controller;
  gchar                *entry; // FIXME, remove this


  GtkWidget            *vbox;
  BjbMainToolbar       *main_toolbar;
  BjbSearchToolbar     *search_bar;


  GtkStack             *stack;
  BjbWindowViewType     current_view;
  BjbMainView          *view;
  BjbNoteView          *note_view;
  GtkWidget            *spinner;
  GtkWidget            *no_note;


  /* when a note is opened */
  BijiNoteObj          *note;
  gboolean              detached; // detached note


  PangoFontDescription *font ;
};

/* Gobject */
G_DEFINE_TYPE (BjbWindowBase, bjb_window_base, GTK_TYPE_APPLICATION_WINDOW);


static GObject *
bjb_window_base_constructor (GType                  gtype,
                             guint                  n_properties,
                             GObjectConstructParam  *properties)
{
  GObject *obj;
  {
    obj = G_OBJECT_CLASS (bjb_window_base_parent_class)->constructor (gtype, n_properties, properties);
  }
  return obj;
}


static void
bjb_window_base_finalize (GObject *object)
{
  BjbWindowBase *self = BJB_WINDOW_BASE (object);
  BjbWindowBasePriv *priv = self->priv;

  g_clear_object (&priv->controller);

  G_OBJECT_CLASS (bjb_window_base_parent_class)->finalize (object);
}




static void
bjb_window_base_get_property (GObject  *object,
                              guint     property_id,
                              GValue   *value,
                              GParamSpec *pspec)
{
  BjbWindowBase *self = BJB_WINDOW_BASE (object);

  switch (property_id)
  {
  case PROP_NOTE:
    g_value_set_object (value, self->priv->note);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
bjb_window_base_set_property (GObject  *object,
                              guint     property_id,
                              const GValue *value,
                              GParamSpec *pspec)
{
  BjbWindowBase *self = BJB_WINDOW_BASE (object);

  switch (property_id)
  {
  case PROP_NOTE:
    self->priv->note = g_value_get_object (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}








/* Just disconnect to avoid crash, the finalize does the real
 * job */
static void
bjb_window_base_destroy (gpointer a, BjbWindowBase * self)
{
  bjb_main_view_disconnect_scrolled_window (self->priv->view);
  bjb_controller_disconnect (self->priv->controller);
}


static gboolean
bjb_application_window_state_changed (GtkWidget *widget,
                                      GdkEvent  *event,
                                      gpointer   user_data)
{
  GSettings *settings;
  GdkWindowState state;
  gboolean maximized;

  settings = user_data;
  state = gdk_window_get_state (gtk_widget_get_window (widget));
  maximized = state & GDK_WINDOW_STATE_MAXIMIZED;

  g_settings_set_boolean (settings, "window-maximized", maximized);

  return FALSE;
}


static gboolean
bjb_application_window_configured (GtkWidget *widget,
                                   GdkEvent  *event,
                                   gpointer   user_data)
{
  BjbWindowBase *win;
  GSettings *settings;
  GVariant *variant;
  GdkWindowState state;
  gint32 size[2];
  gint32 position[2];

  win = BJB_WINDOW_BASE (user_data);
  settings = G_SETTINGS (win->priv->settings);
  state = gdk_window_get_state (gtk_widget_get_window (widget));
  if (state & GDK_WINDOW_STATE_MAXIMIZED)
    return FALSE;

  gtk_window_get_size (GTK_WINDOW (win),
                       (gint *) &size[0],
                       (gint *) &size[1]);
  variant = g_variant_new_fixed_array (G_VARIANT_TYPE_INT32,
                                       size, 2,
                                       sizeof (size[0]));
  g_settings_set_value (settings, "window-size", variant);

  gtk_window_get_position (GTK_WINDOW (win),
                           (gint *) &position[0],
                           (gint *) &position[1]);
  variant = g_variant_new_fixed_array (G_VARIANT_TYPE_INT32,
                                       position, 2,
                                       sizeof (position[0]));
  g_settings_set_value (settings, "window-position", variant);

  return FALSE;
}




/* Gobj */
static void
bjb_window_base_constructed (GObject *obj)
{
  BjbWindowBase *self = BJB_WINDOW_BASE (obj);
  BjbWindowBasePriv *priv;
  const gchar *icons_path;
  gchar *full_path;
  GList *icons = NULL;
  GdkPixbuf *bjb ;
  GError *error = NULL;
  gboolean maximized;
  const gint32 *position;
  const gint32 *size;
  gsize n_elements;
  GVariant *variant;

  G_OBJECT_CLASS (bjb_window_base_parent_class)->constructed (obj);

  priv = self->priv;
  priv->settings = bjb_app_get_settings ((gpointer) g_application_get_default ());




  gtk_window_set_position (GTK_WINDOW (self),GTK_WIN_POS_CENTER);
  gtk_window_set_title (GTK_WINDOW (self), _(BIJIBEN_MAIN_WIN_TITLE));

  variant = g_settings_get_value (G_SETTINGS (priv->settings), "window-size");
  size = g_variant_get_fixed_array (variant, &n_elements, sizeof (gint32));
  if (n_elements == 2)
    gtk_window_set_default_size (GTK_WINDOW (self), size[0], size[1]);

  g_variant_unref (variant);

  variant = g_settings_get_value (G_SETTINGS (priv->settings), "window-position");
  position = g_variant_get_fixed_array (variant, &n_elements, sizeof (gint32));
  if (n_elements == 2)
    gtk_window_move (GTK_WINDOW (self), position[0], position[1]);

  g_variant_unref (variant);

  maximized = g_settings_get_boolean (G_SETTINGS (priv->settings), "window-maximized");
  if (maximized)
    gtk_window_maximize (GTK_WINDOW (self));


  /* Icon for window. TODO - Should be BjbApp */
  icons_path = bijiben_get_bijiben_dir ();
  full_path = g_build_filename (icons_path,
                                "icons",
                                "hicolor",
                                "48x48",
                                "apps",
                                "bijiben.png",
                                NULL);

  bjb = gdk_pixbuf_new_from_file (full_path, &error);
  g_free (full_path);

  if ( error )
  {
    g_message("%s", error->message);
    g_error_free(error);
  }

  icons = g_list_prepend(icons,bjb);
  gtk_window_set_default_icon_list(icons);
  g_list_foreach (icons, (GFunc) g_object_unref, NULL);
  g_list_free (icons);

  /*  We probably want to offer a no entry window at first (startup) */
  priv->entry = NULL ;
  priv->font = pango_font_description_from_string (BJB_DEFAULT_FONT);

  priv->controller = bjb_controller_new
    (bijiben_get_manager (BIJIBEN_APPLICATION(g_application_get_default())),
     GTK_WINDOW (obj),
     priv->entry );

  /* Shared toolbar */
  priv->view = bjb_main_view_new (GTK_WIDGET (obj), priv->controller);
  priv->main_toolbar = bjb_main_toolbar_new (priv->view, priv->controller);
  gtk_window_set_titlebar (GTK_WINDOW (self), GTK_WIDGET (priv->main_toolbar));

  /* Search entry toolbar */
  priv->search_bar = bjb_search_toolbar_new (GTK_WIDGET (obj), priv->controller);
  gtk_box_pack_start (GTK_BOX (priv->vbox), GTK_WIDGET (priv->search_bar), FALSE, FALSE, 0);

  /* UI : stack for different views */
  priv->stack = GTK_STACK (gtk_stack_new ());
  gtk_box_pack_start (GTK_BOX (priv->vbox), GTK_WIDGET (priv->stack), TRUE, TRUE, 0);

  priv->spinner = gtk_spinner_new ();
  gtk_stack_add_named (priv->stack, priv->spinner, "spinner");
  gtk_stack_set_visible_child_name (priv->stack, "spinner");
  gtk_widget_show (priv->spinner);
  gtk_spinner_start (GTK_SPINNER (priv->spinner));

  priv->no_note = bjb_empty_results_box_new ();
  gtk_stack_add_named (priv->stack, priv->no_note, "empty");

  gtk_stack_add_named (priv->stack, GTK_WIDGET (priv->view), "main-view");

  g_signal_connect (GTK_WIDGET (self),
                    "destroy",
                    G_CALLBACK (bjb_window_base_destroy),
                    self);

  g_signal_connect (self,
                    "window-state-event",
                    G_CALLBACK (bjb_application_window_state_changed),
                    priv->settings);

  g_signal_connect (self,
                    "configure-event",
                    G_CALLBACK (bjb_application_window_configured),
                    self);

  /* If a note is requested at creation, show it
   * This is a specific type of window not associated with any view */
  if (priv->note == NULL)
  {
    bjb_window_base_switch_to (self, BJB_WINDOW_BASE_MAIN_VIEW);
  }

  else
  {
    priv->detached = TRUE;
    bjb_window_base_switch_to_item (self, BIJI_ITEM (priv->note));
  }
}

static void
bjb_window_base_init (BjbWindowBase *self)
{
  BjbWindowBasePriv *priv;

  self->priv = priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                                   BJB_TYPE_WINDOW_BASE,
                                                   BjbWindowBasePriv);

  /* Default window has no note opened */
  priv->note_view = NULL;
  priv->note = NULL;
  priv->detached = FALSE;

  priv->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (self), priv->vbox);
}

static void
bjb_window_base_class_init (BjbWindowBaseClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);


  gobject_class->constructed = bjb_window_base_constructed;
  gobject_class->constructor = bjb_window_base_constructor;
  gobject_class->finalize = bjb_window_base_finalize ;
  gobject_class->get_property = bjb_window_base_get_property;
  gobject_class->set_property = bjb_window_base_set_property;

  g_type_class_add_private (klass, sizeof (BjbWindowBasePriv));

  bjb_win_base_signals[BJB_WIN_BASE_VIEW_CHANGED] = g_signal_new ("view-changed" ,
                                                    G_OBJECT_CLASS_TYPE (klass),
                                                    G_SIGNAL_RUN_LAST,
                                                    0,
                                                    NULL,
                                                    NULL,
                                                    g_cclosure_marshal_VOID__VOID,
                                                    G_TYPE_NONE,
                                                    0);

  bjb_win_base_signals[BJB_WIN_BASE_ACTIVATED] =    g_signal_new ("activated" ,
                                                    G_OBJECT_CLASS_TYPE (klass),
                                                    G_SIGNAL_RUN_LAST,
                                                    0,
                                                    NULL,
                                                    NULL,
                                                    g_cclosure_marshal_VOID__BOOLEAN,
                                                    G_TYPE_NONE,
                                                    1,
                                                    G_TYPE_BOOLEAN);

  properties[PROP_NOTE] = g_param_spec_object ("note",
                                               "NoteObj",
                                               "Currently opened note",
                                               BIJI_TYPE_NOTE_OBJ,
                                               G_PARAM_READWRITE |
                                               G_PARAM_CONSTRUCT |
                                               G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, NUM_PROPERTIES, properties);

}


GtkWindow *
bjb_window_base_new                    (BijiNoteObj *note)
{
  return g_object_new (BJB_TYPE_WINDOW_BASE,
                       "application", g_application_get_default(),
                       "note", note,
                       NULL);
}


BjbController *
bjb_window_base_get_controller ( BjbWindowBase *window )
{
  return window->priv->controller ;
}

PangoFontDescription *
window_base_get_font(GtkWidget *window)
{
  BjbWindowBase *b = BJB_WINDOW_BASE(window);
  return b->priv->font ;
}

BijiNoteObj *
bjb_window_base_get_note (BjbWindowBase *self)
{
  g_return_val_if_fail (BJB_IS_WINDOW_BASE (self), NULL);

  return self->priv->note;
}



static void
destroy_note_if_needed (BjbWindowBase *self)
{
  self->priv->note = NULL;

  if (self->priv->note_view && GTK_IS_WIDGET (self->priv->note_view))
    g_clear_pointer (&(self->priv->note_view), gtk_widget_destroy);
}


void
bjb_window_base_switch_to (BjbWindowBase *self, BjbWindowViewType type)
{
  BjbWindowBasePriv *priv = self->priv;
  priv->current_view = type;

  if (type != BJB_WINDOW_BASE_NOTE_VIEW)
    destroy_note_if_needed (self);

  switch (type)
  {

    /* Precise the window does not display any specific note
     * Refresh the model
     * Ensure the main view receives the proper signals
     *
     * main view & archive view are the same widget
     */

    case BJB_WINDOW_BASE_MAIN_VIEW:
      bjb_search_toolbar_connect (priv->search_bar);
      bjb_main_view_connect_signals (priv->view);
      gtk_widget_show (GTK_WIDGET (priv->search_bar));
      gtk_stack_set_visible_child_name (priv->stack, "main-view");
      break;

   case BJB_WINDOW_BASE_ARCHIVE_VIEW:
      bjb_search_toolbar_connect (priv->search_bar);
      bjb_main_view_connect_signals (priv->view);
      gtk_widget_show (GTK_WIDGET (priv->search_bar));
      gtk_stack_set_visible_child_name (priv->stack, "main-view");
      break;

    case BJB_WINDOW_BASE_SPINNER_VIEW:
      gtk_stack_set_visible_child_name (priv->stack, "spinner");
      break;


    case BJB_WINDOW_BASE_NO_NOTE:
      bjb_empty_results_box_set_type (BJB_EMPTY_RESULTS_BOX (priv->no_note),
                                      BJB_EMPTY_RESULTS_NO_NOTE);
      gtk_widget_show (priv->no_note);
      gtk_widget_hide (GTK_WIDGET (priv->search_bar));
      gtk_stack_set_visible_child_name (priv->stack, "empty");
      break;


    case BJB_WINDOW_BASE_NO_RESULT:
      bjb_empty_results_box_set_type (BJB_EMPTY_RESULTS_BOX (priv->no_note),
                                      BJB_EMPTY_RESULTS_NO_RESULTS);
      gtk_widget_show (priv->no_note);
      gtk_widget_hide (GTK_WIDGET (priv->search_bar));
      gtk_stack_set_visible_child_name (priv->stack, "empty");
      break;


    case BJB_WINDOW_BASE_ERROR_TRACKER:
      bjb_empty_results_box_set_type (BJB_EMPTY_RESULTS_BOX (priv->no_note),
                                      BJB_EMPTY_RESULTS_TRACKER);
      gtk_widget_show_all (priv->no_note);
      gtk_widget_hide (GTK_WIDGET (priv->search_bar));
      gtk_stack_set_visible_child_name (priv->stack, "empty");
      break;


    case BJB_WINDOW_BASE_NOTE_VIEW:
      gtk_widget_show_all (GTK_WIDGET (priv->note_view));
      gtk_widget_hide (GTK_WIDGET (priv->search_bar));
      gtk_stack_set_visible_child_name (priv->stack, "note-view");
      break;


    default:
      return;
  }

  g_signal_emit (G_OBJECT (self), bjb_win_base_signals[BJB_WIN_BASE_VIEW_CHANGED],0);
}


void
bjb_window_base_switch_to_item (BjbWindowBase *bwb, BijiItem *item)
{
  BjbWindowBasePriv *priv = bwb->priv;
  GtkWidget *w = GTK_WIDGET (bwb);

  bjb_search_toolbar_disconnect (priv->search_bar);
  bjb_search_toolbar_fade_out (priv->search_bar);
  destroy_note_if_needed (bwb);

  if (BIJI_IS_NOTE_OBJ (item))
  {

    BijiNoteObj *note = BIJI_NOTE_OBJ (item);

    priv->note = note;

    priv->note_view = bjb_note_view_new (w, note);
    gtk_stack_add_named (priv->stack, GTK_WIDGET (priv->note_view), "note-view");

    g_object_add_weak_pointer (G_OBJECT (priv->note_view),
                               (gpointer *) &priv->note_view);

    bjb_window_base_switch_to (bwb, BJB_WINDOW_BASE_NOTE_VIEW);
    gtk_widget_show_all (w);
    bjb_note_view_grab_focus (priv->note_view);
  }
}

BjbWindowViewType
bjb_window_base_get_view_type (BjbWindowBase *win)
{
  return win->priv->current_view;
}

BijiManager *
bjb_window_base_get_manager(GtkWidget * win)
{
  return bijiben_get_manager (BIJIBEN_APPLICATION (g_application_get_default()));
}

void
bjb_window_base_set_entry(GtkWidget *win, gchar *search_entry)
{
  BjbWindowBase *bmw;

  g_return_if_fail (BJB_IS_WINDOW_BASE (win));

  bmw = BJB_WINDOW_BASE (win);
  bmw->priv->entry = search_entry;
}


gchar *
bjb_window_base_get_entry(GtkWidget *win)
{
  BjbWindowBase *bmw = BJB_WINDOW_BASE(win);
  return bmw->priv->entry ;
}

gpointer
bjb_window_base_get_main_view (BjbWindowBase *self)
{
  return (gpointer) self->priv->view;
}

gboolean
bjb_window_base_get_show_search_bar (BjbWindowBase *self)
{

  /* There is no search bar at startup,
   * when main toolbar is first built... */
  if (!self->priv->search_bar)
    return FALSE;

  return gtk_search_bar_get_search_mode (
            GTK_SEARCH_BAR (self->priv->search_bar));
}

gboolean
bjb_window_base_set_show_search_bar (BjbWindowBase *self,
                                     gboolean show)
{
  if (show)
    bjb_search_toolbar_fade_in (self->priv->search_bar);

  else
    bjb_search_toolbar_fade_out (self->priv->search_bar);

  return TRUE;
}

gboolean
bjb_window_base_toggle_search_button (BjbWindowBase *self,
                                      gboolean active)
{
  bjb_main_toolbar_set_search_toggle_state (self->priv->main_toolbar,
                                            active);

  return TRUE;
}


void
bjb_window_base_set_active (BjbWindowBase *self, gboolean active)
{
  gboolean available;

  available = (self->priv->current_view != BJB_WINDOW_BASE_NOTE_VIEW);

  if (active == TRUE)
  {
    g_signal_emit (self,
                   bjb_win_base_signals[BJB_WIN_BASE_ACTIVATED],
                   0,
                   available);
  }
}


gboolean
bjb_window_base_is_detached (BjbWindowBase *self)
{
  return self->priv->detached;
}
