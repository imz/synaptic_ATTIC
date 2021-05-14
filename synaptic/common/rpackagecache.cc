/* rpackagecache.cc - package cache wrapper
 *
 * Copyright (c) 2000-2003 Conectiva S/A
 *
 * Author: Alfredo K. Kojima <kojima@conectiva.com.br>
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

#include "rpackagecache.h"
#include "rconfiguration.h"
#include "i18n.h"

#include <assert.h>
#include <algorithm>
#include <apt-pkg/error.h>
#include <apt-pkg/sourcelist.h>
#include <apt-pkg/pkgcachegen.h>
#include <apt-pkg/configuration.h>
#include <apt-pkg/policy.h>


bool RPkgPolicy::IsImportantDep(pkgCache::DepIterator dep)
{
  if(pkgPolicy::IsImportantDep(dep))
    return true;

  if(dep->Type==pkgCache::Dep::Recommends) {
     return _config->FindB("Synaptic::UseRecommends", false);
  } else if(dep->Type==pkgCache::Dep::Suggests) {
     return _config->FindB("Synaptic::UseSuggests", false);
  } else {
     return false;
  }
}

bool RPackageCache::open(OpProgress &progress, bool locking)
{
   if(locking)
      lock();

   if (_error->PendingError())
      return false;

   // delete any old structures
   _dcache.reset();
   _policy.reset();
   _cache.reset();
   _map.reset();

   // Read the source list
   //pkgSourceList list;
   assert(_list != NULL);
   if (!_list->ReadMainList())
      return _error->Error(_("The list of sources could not be read.\n\
Go to the repository dialog to correct the problem."));

   if (! pkgMakeStatusCache(*_list, progress, locking))
      // TODO: we could handle this case (no MMap could be created) as an error
      ;
   // TODO: why not re-use the returned MMap as _map?

   if (_error->PendingError())
      return _error->
         Error(_
               ("The package lists or status file could not be parsed or opened."));

   // Open the cache file
   FileFd File;
   File.Open(_config->FindFile("Dir::Cache::pkgcache"), FileFd::ReadOnly);
   if (_error->PendingError())
      return false;

   _map.reset(new MMap(File, MMap::Public | MMap::ReadOnly));
   if (_error->PendingError())
      return false;

   // Create the dependency cache
   _cache.reset(new pkgCache(*_map));
   if (_error->PendingError())
      return false;

   _policy.reset(new RPkgPolicy(_cache.get()));
   if (_error->PendingError() == true)
      return false;
   if (ReadPinFile(*_policy) == false)
      return false;

   if (ReadPinFile(*_policy, RStateDir() + "/preferences") == false)
      return false;

   _dcache.reset(new pkgDepCache(_cache.get(), _policy.get()));
   _dcache->Init(&progress);

   //progress.Done();
   if (_error->PendingError())
      return false;

   // Check that the system is OK
   if (_dcache->DelCount() != 0 || _dcache->InstCount() != 0)
      return _error->Error(_("Internal Error, non-zero counts"));

   return true;
}

vector<string> RPackageCache::getPolicyArchives(bool filenames_only=false)
{
   //std::cout << "RPackageCache::getPolicyComponents() " << std::endl;

   vector<string> archives;
   for (pkgCache::PkgFileIterator F = _cache->FileBegin(); F.end() == false;
        F++) {
      pkgIndexFile *Indx;
      _list->FindIndex(F, Indx);
      _system->FindIndex(F, Indx);

      if(filenames_only) {
	 if(F.FileName())
	    archives.push_back(F.FileName());
      } else {
	 if (!F.RelStr().empty()) {
	    //printf("Archive: %s, Origin: %s, Component: %s, Filename: %s\n",
	    //       F.Archive(), F.Origin(), F.Component(), F.FileName());
	    if (F.Archive() != NULL) {
	       if (find(archives.begin(), archives.end(), F.Archive())
		   == archives.end()) {
		  archives.push_back(F.Archive());
	       }
	    }
	 }
      }
   }
   return archives;
}


bool RPackageCache::lock()
{
   if (_locked)
      return true;

   _system->Lock();
   _locked = true;

   //FIXME: should depend on the result of _system->lock()
   return true;
}


void RPackageCache::releaseLock()
{
   if (!_locked)
      return;

   _system->UnLock();
   _locked = false;
}
