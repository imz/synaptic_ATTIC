/* rgfindwindow.h
 *
 * Copyright (c) 2003 Michael Vogt
 *
 * Author: Michael Vogt <mvo@debian.org>
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


#include "rggladewindow.h"
#include "rpackagefilter.h"

class RGFindWindow;

typedef void RGFindWindowFindAction(void *self, RGFindWindow * win);

class RGFindWindow:public RGGladeWindow {

   //RPatternPackageFilter::DepType _searchType;
   GList *_prevSearches;
   GtkWidget *_entry, *_findB, *_omenu;

   static void doFind(GtkWindow *widget, void *data);
   static void doClose(GtkWindow *widget, void *data);
   static void cbEntryChanged(GtkWindow *widget, void *data);

 public:
   RGFindWindow(RGWindow *owner);
   virtual ~RGFindWindow() {
   };

   int getSearchType();

   string getFindString();
   void selectText();
};
