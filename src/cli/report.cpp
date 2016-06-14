/*
--------------Copyright (c) 2010-2016 H a r a l d  A c h i t z---------------
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
#include <sbbdep/cache.hpp>

#include <iomanip>

namespace sbbdep {
namespace cli{



void
printSyncReport(Cache& cache,
                const SyncData& syncdata)
{

  if(syncdata.wasNewCache)
    {
      LogInfo()
          << "successfully indexed " << syncdata.installed.size()
          << " packages for new cache " << cache.getName() ;

      return ;
    }

  if(syncdata.empty ())
    {
      LogInfo () << "cache up to date";

    }
  else
    {
      LogInfo () << "Synchronization summary:\n";
      for(auto&& p : syncdata.removed)
        {
          LogInfo () << "removed: " << p;
        }
      if(not syncdata.removed.empty())
        LogInfo () << "" ; // just a new line

      for(auto&& p : syncdata.installed)
        {
          LogInfo () << "installed: " << p;
        }
      if(not syncdata.installed.empty())
         LogInfo () << "" ; // just a new line

      for(auto&& p : syncdata.reinstalled)
        {
          LogInfo () << "reinstalled: " << p;
        }
      if(not syncdata.reinstalled.empty())
         LogInfo () << "" ; // just a new line

      size_t longest = 0 ;
      for(auto&& pp : syncdata.updated)
        {
          const auto len = pp.second.fullName ().size();
          if(len > longest)
            longest = len ;
        }


      for(auto&& pp : syncdata.updated)
        {
          LogInfo () << "updated: "
              << std::setw(longest + 3) << std::left
              << pp.second.fullName ()
              << std::setw(0)
              << "from " // extract version arch buildstr
              << pp.first.fullName ().substr ( pp.first.name ().size () + 1)
              ;
        }

      if (syncdata.problemsOld.size () > 0 or syncdata.problemsNew.size () > 0)
        {
          LogInfo () << "\nNote: problem analyzing following packages:" ;
          LogInfo () << " (This does not mean that there is any problem "
              "or inconsistency on this system\n"
              " but maybe you want to double check those packages)" ;

            for (auto&& p : syncdata.problemsOld )
              {
                LogInfo () << "(updated/removed)?: " << p ;
              }

            for (auto&& p : syncdata.problemsNew )
              {
                LogInfo () << "(updated/installed)?: " << p;
              }

            LogInfo () << "If everything is OK and this message "
                "comes again you can rebuild the cache by remove "
                "the existing one.\n"
                "Please consider also sending the cache to the sbbdep author "
                "so that this problem can be analyzed." ;
        }


    }
  LogInfo() << "" ; // append new line
}



//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
}} // ns
