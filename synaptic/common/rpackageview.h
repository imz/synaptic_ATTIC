/* rpackageview.h - Package sectioning system
 * 
 * Copyright (c) 2004 Conectiva S/A 
 *               2004 Michael Vogt <mvo@debian.org>
 * 
 * Author: Gustavo Niemeyer
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


#ifndef RPACKAGEVIEW_H
#define RPACKAGEVIEW_H

#include <string>
#include <map>

#include "rpackage.h"
#include "rpackagefilter.h"

#include "i18n.h"

using namespace std;

struct RFilter;

class RPackageView {
 protected:

   map<string, vector<RPackage *> > _view;

   bool _hasSelection;
   string _selectedName;

   // packages in selected
   vector<RPackage *> _selectedView;

   // all packages in current global filter
   vector<RPackage *> &_all;

 public:
   RPackageView(vector<RPackage *> &allPackages): _all(allPackages) {};
   virtual ~RPackageView() {};

   bool hasSelection() { return _hasSelection; };
   string getSelected() { return _selectedName; };
   bool setSelected(string name);
   void showAll() { 
      _selectedView = _all; 
      _hasSelection = false;
      _selectedName.clear();
   };

   vector<string> getSubViews();

   virtual string getName() = 0;
   virtual void addPackage(RPackage *package) = 0;

   typedef vector<RPackage *>::iterator iterator;

   virtual iterator begin() { return _selectedView.begin(); };
   virtual iterator end() { return _selectedView.end(); };

   virtual void clear();
   virtual void clearSelection();
};



class RPackageViewSections:public RPackageView {
 public:
   RPackageViewSections(vector<RPackage *> &allPkgs) : RPackageView(allPkgs) {};

   string getName() {
      return _("Sections");
   };

   void addPackage(RPackage *package);
};

class RPackageViewAlphabetic : public RPackageView {
 public:
   RPackageViewAlphabetic(vector<RPackage *> &allPkgs) : RPackageView(allPkgs) {};
   string getName() {
      return _("Alphabetic");
   };

   void addPackage(RPackage *package) {
      char letter[2] = { ' ', '\0' };
      letter[0] = toupper(package->name()[0]);
      _view[letter].push_back(package);
   };
};

class RPackageViewStatus:public RPackageView {
 public:
   RPackageViewStatus(vector<RPackage *> &allPkgs) : RPackageView(allPkgs) {};

   string getName() {
      return _("Status");
   };

   void addPackage(RPackage *package);
};

class RPackageViewSearch : public RPackageView {
 protected:
   vector<string> searchStrings;
   string searchName;
   int searchType;
   int found; // nr of found pkgs for the last search
 public:
   RPackageViewSearch(vector<RPackage *> &allPkgs) 
      : RPackageView(allPkgs), found(0) {};

   // 1. make sure that searchName is in a encoding that the GUI supports
   //    (e.g. UTF-8)
   // 2. make sure that searchString is in locale encoding for non-english
   //    searches
   int setSearch(string searchName, int type, string searchString);

   string getName() {
      return _("Search History");
   };

   void addPackage(RPackage *package);
};


class RPackageViewFilter : public RPackageView {
 protected:
   vector<RFilter *> _filterL;
   set<string> _sectionList;   // list of all available package sections

 public:
   void storeFilters();
   void restoreFilters();
   // called after the filtereditor was run
   void refreshFilters();

   bool registerFilter(RFilter *filter);
   void unregisterFilter(RFilter *filter);

   void makePresetFilters();

   RFilter* findFilter(string name);
   unsigned int nrOfFilters() { return _filterL.size(); };
   RFilter *findFilter(unsigned int index) {
      if (index > _filterL.size())
         return NULL;
      else
         return _filterL[index];
   };
   vector<string> getFilterNames();
   const set<string> &getSections() { return _sectionList; };

   RPackageViewFilter(vector<RPackage *> &allPkgs);

   // build packages list on "demand"
   virtual iterator begin();

   // we never need to clear because we build the view "on-demand"
   virtual void clear() {clearSelection();};

   string getName() {
      return _("Custom Filters");
   };

   void addPackage(RPackage *package);
};



#endif

// vim:sts=3:sw=3