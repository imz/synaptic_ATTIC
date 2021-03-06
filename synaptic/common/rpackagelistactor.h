/* rcacheactor.h
 * 
 * Copyright (c) 2003 Michael Vogt <mvo@debian.org>
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

#ifndef RPACKAGELISTACTOR_H
#define RPACKAGELISTACTOR_H

#include "rpackagelister.h"
#include <iostream>

class RPackageListActor : public RPackageObserver {

   public:

   enum listEvent {
      PKG_ADDED,
      PKG_REMOVED
   };

   protected:

   RPackageLister *_lister;
   vector<RPackage *> _lastDisplayList;

   public:

   virtual void run(vector<RPackage *> &List, int listEvent) = 0;

   virtual void notifyPreFilteredChange() override {
      updateState();
   }

   virtual void notifyPostFilteredChange() override;
   virtual void notifyChange(RPackage *pkg) override {}

   virtual void updateState() {
      _lastDisplayList = _lister->getPackages();
   };

   RPackageListActor(RPackageLister *lister)
         : _lister(lister) {
      _lister->registerObserver(this);
   };

   virtual ~RPackageListActor() {
      _lister->unregisterObserver(this);
   };
};


#endif

// vim:sts=3:sw=3
