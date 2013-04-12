/*
--------------Copyright (c) 2010-2012 H a r a l d  A c h i t z---------------
-----------< h a r a l d dot a c h i t z at g m a i l dot c o m >------------
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/


#include "report.hpp"

#include <sbbdep/log.hpp>
#include <sbbdep/pkgname.hpp>

#include <set>
#include <algorithm>

namespace sbbdep {
namespace cli{



void printSyncReport(Cache::SyncData syncdata)
{
  using StringSet = std::set<std::string> ; //make it sortd

  StringSet deleted, reinstalled, installed, upgraded;


  // take removed, than check inserted, if inserted is also in deleted, its an update
  deleted.insert(syncdata.removed.begin(),syncdata.removed.end() );
  // so make all inserted to deleted,
  // take all installed and search if deleted,
  // if found , remove from deleted and add to upgraded, else it is inserted

  for(auto& val : syncdata.installed)
    {   // need package name, if the name is in removed and in installed, than
      PkgName pkgname(val);
      auto compair = [&pkgname](const std::string& fullname)->bool
        {
          return pkgname.Name() == PkgName(fullname).Name();
        };

      auto isdeleted = std::find_if(deleted.begin(), deleted.end(), compair);
      if( isdeleted == deleted.end() )
        {
          installed.insert(val);
        }
      else
        {
          PkgName delname(*isdeleted);
          std::string updateinfo = val +
              "  (was " + delname.FullName().substr( delname.Name().size()+1 ) +")";
          upgraded.insert(updateinfo);
          deleted.erase(isdeleted);
        }

    }

  // finally transfer the re installed and then print all
  reinstalled.insert(syncdata.reinstalled.begin(), syncdata.reinstalled.end());


  LogInfo()    << "\n---synchronisation summary:\n" ;
  for(auto& val : deleted) LogInfo()    << "deleted: " << val << std::endl;
  for(auto& val : upgraded) LogInfo()   << "upgraded: " << val  << std::endl;
  for(auto& val : installed) LogInfo()  << "installed: " << val  << std::endl;
  for(auto& val : reinstalled) LogInfo()<< "reinstalled: " << val  << std::endl;

}

//--------------------------------------------------------------------------------------------------



//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
}} // ns
