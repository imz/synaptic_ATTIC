/* rgfetchprogress.cc
 *
 * Copyright (c) 2000, 2001 Conectiva S/A
 *               2002, 2003 Michael Vogt <mvo@debian.org>
 *
 * Author: Alfredo K. Kojima <kojima@conectiva.com.br>
 *         Michael Vogt <mvo@debian.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include "config.h"

#include <apt-pkg/acquire-item.h>
#include <apt-pkg/acquire-worker.h>
#include <apt-pkg/strutl.h>
#include <apt-pkg/error.h>

#include "rgfetchprogress.h"
#include "rguserdialog.h"
#include "rgmisc.h"

#include <stdio.h>
#include <pango/pango.h>
#include <gtk/gtk.h>
#include <cassert>

#include "i18n.h"

enum {
   DLDone = -1,
   DLQueued = -2,
   DLFailed = -3,
   DLHit = -4
};

enum {
   FETCH_PIXMAP_COLUMN,
   FETCH_SIZE_COLUMN,
   FETCH_DESCR_COLUMN,
   FETCH_URL_COLUMN,
   N_FETCH_COLUMNS
};

static const int COLUMN_PERCENT_WIDTH=100;
static const int COLUMN_PERCENT_HEIGHT=18;


bool RGFetchProgress::close()
{
   stopDownload(NULL, this);
   
   return TRUE;
}

RGFetchProgress::RGFetchProgress(RGWindow *win)
   : RGGladeWindow(win, "fetch"), _cursorDirty(false), _sock(NULL)
{
   GtkCellRenderer *renderer;
   GtkTreeViewColumn *column;

   _mainProgressBar = glade_xml_get_widget(_gladeXML, "progressbar_download");
   assert(_mainProgressBar);

   _table = glade_xml_get_widget(_gladeXML, "treeview_fetch");
   _tableListStore = gtk_list_store_new(N_FETCH_COLUMNS,
                                        GDK_TYPE_PIXBUF,
                                        G_TYPE_STRING, G_TYPE_STRING, 
					G_TYPE_STRING);
   gtk_tree_view_set_model(GTK_TREE_VIEW(_table),
                           GTK_TREE_MODEL(_tableListStore));

   /* percent column */
   renderer = gtk_cell_renderer_pixbuf_new();
   column = gtk_tree_view_column_new_with_attributes(_("Status"), renderer,
                                                     "pixbuf",
                                                     FETCH_PIXMAP_COLUMN,
                                                     NULL);
   gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
   gtk_tree_view_column_set_fixed_width(column, COLUMN_PERCENT_WIDTH);
   _statusColumn = column;
   _statusRenderer = renderer;
   gtk_tree_view_append_column(GTK_TREE_VIEW(_table), column);

   /* size */
   renderer = gtk_cell_renderer_text_new();
   column = gtk_tree_view_column_new_with_attributes(_("Size"), renderer,
                                                     "text", FETCH_SIZE_COLUMN,
                                                     NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(_table), column);

   /* descr */
   renderer = gtk_cell_renderer_text_new();
   column = gtk_tree_view_column_new_with_attributes(_("Package"), renderer,
                                                     "text",FETCH_DESCR_COLUMN,
						     NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(_table), column);

   /* url */
   renderer = gtk_cell_renderer_text_new();
   column = gtk_tree_view_column_new_with_attributes(_("URI"), renderer,
                                                     "text", FETCH_URL_COLUMN, 
						     NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(_table), column);


   glade_xml_signal_connect_data(_gladeXML,
                                 "on_button_cancel_clicked",
                                 G_CALLBACK(stopDownload), this);

   PangoContext *context = gdk_pango_context_get();
   _layout = pango_layout_new(context);

   // check if we should run embedded somewhere
   // we need to make the window show before we obtain the _gc
   int id = _config->FindI("Volatile::PlugProgressInto", -1);
   //cout << "Plug ID: " << id << endl;
   if (id > 0) {
      gtk_widget_hide(glade_xml_get_widget(_gladeXML, "window_fetch"));
      GtkWidget *vbox = glade_xml_get_widget(_gladeXML, "vbox_fetch");
      _sock =  gtk_plug_new(id);
      gtk_widget_reparent(vbox, _sock);
      gtk_widget_show_all(_sock);
      _win = _sock;
   } 
   gtk_widget_realize(_win);

   // reset the urgency hint here (gtk seems to like showing it for
   // dialogs that come up)
   gtk_window_set_urgency_hint(GTK_WINDOW(_win), FALSE);

   GtkStyle *style = gtk_widget_get_style(_win);
   _font = style->font_desc;
   _gc = style->white_gc;
   _barGC = style->bg_gc[0];
   _textGC = style->black_gc;

   // emit a signal if the user changed the cursor
   g_signal_connect(G_OBJECT(_table), "cursor-changed", 
		    G_CALLBACK(cursorChanged), this);

   // emit a signal if the user changed the cursor
   GtkWidget *expander = glade_xml_get_widget(_gladeXML, "expander");
   assert(expander);
   g_signal_connect (expander, "notify::expanded",
		     G_CALLBACK (expanderActivate), this);

}

void RGFetchProgress::expanderActivate(GObject    *object,
				       GParamSpec *param_spec,
				       gpointer    data)
{
   GtkExpander *expander = GTK_EXPANDER (object);
   RGFetchProgress *me = (RGFetchProgress*)data;

   GtkWidget *win = glade_xml_get_widget(me->_gladeXML, "window_fetch");
   if (gtk_expander_get_expanded (expander)) 
      gtk_window_set_resizable(GTK_WINDOW(win),TRUE);
   else 
      gtk_window_set_resizable(GTK_WINDOW(win), FALSE);
}


void RGFetchProgress::cursorChanged(GtkTreeView *self, void *data)
{
   //cout << "cursor-changed" << endl;
   RGFetchProgress *me = (RGFetchProgress *)data;
   me->_cursorDirty=true;
}

void RGFetchProgress::setDescription(string mainText, string secondText)
{
   gtk_window_set_title(GTK_WINDOW(_win), mainText.c_str());
   gchar *str = g_strdup_printf("<big><b>%s</b></big> \n\n%s",
				  mainText.c_str(), secondText.c_str());
   gtk_label_set_markup(GTK_LABEL(glade_xml_get_widget(_gladeXML, "label_description")), str);
			

   g_free(str);
}

bool RGFetchProgress::MediaChange(const string &Media, const string &Drive)
{
   gchar *msg;

   msg = g_strdup_printf(_("Please insert the media labeled:\n%s"),
			 Media.c_str());

   RGUserDialog userDialog(this);
   _cancelled = !userDialog.proceed(msg);

   RGFlushInterface();
   g_free(msg);
   return true;



#if 0 // this code can be used when apt fixed ubuntu #2281 (patch pending)
   bool res = !userDialog.proceed(msg);
   //cout << "Media change " << res << endl;

   RGFlushInterface();
   g_free(msg);

   if(res) {
      return false;
   } else {
      Update = true;
      return true;
   }
#endif
}


void RGFetchProgress::updateStatus(pkgAcquire::ItemDesc & Itm, int status)
{
   //cout << "void RGFetchProgress::updateStatus()" << endl;

   if (Itm.Owner->ID == 0) {
      Item item;
      item.descr = Itm.ShortDesc;
      item.uri = Itm.Description;
      item.size = string(SizeToStr(Itm.Owner->FileSize));
      item.status = status;
      _items.push_back(item);
      Itm.Owner->ID = _items.size();
      refreshTable(Itm.Owner->ID - 1, true);
   } else if (_items[Itm.Owner->ID - 1].status != status) {
      _items[Itm.Owner->ID - 1].status = status;
      refreshTable(Itm.Owner->ID - 1, false);
   }
}


void RGFetchProgress::IMSHit(pkgAcquire::ItemDesc & Itm)
{
   //cout << "void RGFetchProgress::IMSHit(pkgAcquire::ItemDesc &Itm)" << endl;
   updateStatus(Itm, DLHit);

   RGFlushInterface();
}


void RGFetchProgress::Fetch(pkgAcquire::ItemDesc & Itm)
{
   updateStatus(Itm, DLQueued);

   RGFlushInterface();
}


void RGFetchProgress::Done(pkgAcquire::ItemDesc & Itm)
{
   updateStatus(Itm, DLDone);

   RGFlushInterface();
}


void RGFetchProgress::Fail(pkgAcquire::ItemDesc & Itm)
{
   if (Itm.Owner->Status == pkgAcquire::Item::StatIdle)
      return;

   updateStatus(Itm, DLFailed);

   RGFlushInterface();
}


bool RGFetchProgress::Pulse(pkgAcquire * Owner)
{
   //cout << "RGFetchProgress::Pulse(pkgAcquire *Owner)" << endl;

   pkgAcquireStatus::Pulse(Owner);

   // only show here if there is actually something to download/get
   if (TotalBytes > 0 && !GTK_WIDGET_VISIBLE(_win))
      show();

   for (pkgAcquire::Worker * I = Owner->WorkersBegin(); I != 0;
        I = Owner->WorkerStep(I)) {

      if (I->CurrentItem == 0)
         continue;

      if (I->TotalSize > 0)
         updateStatus(*I->CurrentItem,
                      long (double (I->CurrentSize * 100.0) /
                            double (I->TotalSize)));
      else
         updateStatus(*I->CurrentItem, 100);

   }

   float percent =
      long (double ((CurrentBytes + CurrentItems) * 100.0) /
            double (TotalBytes + TotalItems));
   // work-around a stupid problem with libapt
   if(CurrentItems == TotalItems)
      percent=100.0;

   gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(_mainProgressBar),
                                 percent / 100.0);

   unsigned long ETA =
      (unsigned long)((TotalBytes - CurrentBytes) / CurrentCPS);
   long i = CurrentItems < TotalItems ? CurrentItems + 1 : CurrentItems;
   gchar *s;
   if (CurrentCPS != 0 && ETA != 0) {
      s = g_strdup_printf(_("Download rate: %s/s - %s remaining"),
			  SizeToStr(CurrentCPS).c_str(),
			  TimeToStr(ETA).c_str());
      gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(_gladeXML, "label_eta")),s);
      g_free(s);
   } else {
      gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(_gladeXML, "label_eta")),_("Download rate: unknown"));
   }
   s = g_strdup_printf(_("Downloading file %li of %li"), i, TotalItems);
   gtk_progress_bar_set_text(GTK_PROGRESS_BAR(_mainProgressBar), s);
   g_free(s);

   RGFlushInterface();

   return !_cancelled;
}


void RGFetchProgress::Start()
{
   //cout << "RGFetchProgress::Start()" << endl;
   pkgAcquireStatus::Start();
   _cancelled = false;

   RGFlushInterface();
}


void RGFetchProgress::Stop()
{
   //cout << "RGFetchProgress::Stop()" << endl;
   RGFlushInterface();
   if(_sock != NULL) {
      gtk_widget_destroy(_sock);
   } else {
      hide();
   }
   pkgAcquireStatus::Stop();

   //FIXME: this needs to be handled in a better way (gtk-2 maybe?)
   sleep(1);                    // this sucks, but if ommited, the window will not always
   // closed (e.g. when a package is only deleted)
   RGFlushInterface();
}



void RGFetchProgress::stopDownload(GtkWidget *self, void *data)
{
   RGFetchProgress *me = (RGFetchProgress *) data;

   me->_cancelled = true;
}


GdkPixmap *RGFetchProgress::statusDraw(int width, int height, int status)
{
   int x, y;
   char *str = "";
   GdkPixmap *pix;
   int px, pw;

   pix = gdk_pixmap_new(_win->window, width, height, -1);

   px = 0;
   pw = status * width / 100;

   if (status < 0) {
      if (status == DLDone || status == DLHit)
         gdk_draw_rectangle(pix, _barGC, TRUE, px, 0, pw, height);
      else
         gdk_draw_rectangle(pix, _gc, TRUE, px, 0, pw, height);

      switch (status) {
         case DLQueued:
            str = _("Queued");
            break;
         case DLDone:
            str = _("Done");
            break;
         case DLHit:
            str = _("Hit");
            break;
         case DLFailed:
            str = _("Failed");
            break;
      }
   } else {
      static char buf[16];

      gdk_draw_rectangle(pix, _barGC, TRUE, px, 0, pw, height);
      gdk_draw_rectangle(pix, _gc, TRUE, px + pw, 0, width - pw - 2, height);

      snprintf(buf, sizeof(buf), "%d%%", status);
      str = buf;
   }

   pango_layout_set_font_description(_layout, _font);
   pango_layout_set_text(_layout, str, -1);
   pango_layout_get_pixel_size(_layout, &x, &y);
   x = (width - x) / 2;
   gdk_draw_layout(pix, _textGC, x, 0, _layout);

   return pix;
}


void RGFetchProgress::refreshTable(int row, bool append)
{
   //cout << "RGFetchProgress::refreshTable() " << row << endl;
   GtkTreeIter iter;
   static GdkPixmap *pix = NULL;
   static GdkPixbuf *buf = NULL;
   int w, h;

   // unref pix first (they start with a usage count of 1
   // why not unref'ing it after adding in the table? -- niemeyer
   if (pix != NULL)
      gdk_pixmap_unref(pix);
   if (buf != NULL)
      gdk_pixbuf_unref(buf);

   w = COLUMN_PERCENT_WIDTH;   //gtk_tree_view_column_get_width(_statusColumn);
   h = COLUMN_PERCENT_HEIGHT; // FIXME: height -> get it from somewhere

   pix = statusDraw(w, h, _items[row].status);
   buf = gdk_pixbuf_get_from_drawable(NULL, pix, NULL, 0, 0, 0, 0, w, h);
   GtkTreePath *path;
   if (append == true) {
      gtk_list_store_insert(_tableListStore, &iter, row);
      path = gtk_tree_path_new();
      gtk_tree_path_append_index(path, row);
      //gtk_tree_view_set_cursor(GTK_TREE_VIEW(_table), path, NULL, false);
      if(!_cursorDirty)
	 gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(_table),
				      path, NULL, TRUE, 0.0, 0.0);
      gtk_tree_path_free(path);
      // can't we use the iterator here?
   } else {
      gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(_tableListStore),
                                    &iter, NULL, row);
   }
   gtk_list_store_set(_tableListStore, &iter,
                      FETCH_PIXMAP_COLUMN, buf,
                      FETCH_SIZE_COLUMN, _items[row].size.c_str(),
                      FETCH_DESCR_COLUMN, _items[row].descr.c_str(),
                      FETCH_URL_COLUMN, _items[row].uri.c_str(), -1);
   path = gtk_tree_model_get_path(GTK_TREE_MODEL(_tableListStore), &iter);
   if(!_cursorDirty)
      gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(_table),
				   path, NULL, TRUE, 0.0, 0.0);
   gtk_tree_path_free(path);
}

// vim:sts=4:sw=4
