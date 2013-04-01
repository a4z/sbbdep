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

#include "appcli.hpp"
#include "appargs.hpp"

// should become one
#include "depfilewriter.hpp"
#include "xdl.hpp"
#include "lookup.hpp"

#include <iostream>
#include <fstream>
#include <string>


#include <sbbdep/config.hpp> // generated 
#include <sbbdep/singles.hpp>
#include <sbbdep/path.hpp>

// should possible not be required here
#include <sbbdep/pkg.hpp>

#include <a4z/err.hpp>

namespace sbbdep {

AppCli::AppCli()
{

}
//--------------------------------------------------------------------------------------------------

AppCli::~AppCli()
{
  //clear all singletons
  destroy_all_singles();
}
//--------------------------------------------------------------------------------------------------

namespace {
bool
prepairCache(bool syncflag)
{
  if( Cache::get()->isNew() )
    {
      if( syncflag )
        {
          LogInfo() << "Cache is new, overrule nosync\n";
          syncflag = false;
        }
    }

  try // TODO in new implementation this will be automatically called at open db
    { //us major minor combination to see if schema has changed in an way that it needs to be re-created
      Cache::get()->checkVersion(
          sbbdep::MAJOR_VERSION,
          sbbdep::MINOR_VERSION ,
          sbbdep::PATCH_VERSION );
    }
  catch (const a4z::Err& e)
    {
      LogError() << e << std::endl;
      return false;
    }

  if( !syncflag )
    {
      try
        {
          Cache::get()->doSync();
        }
      catch (const a4z::Err& e)
        {
          LogError() << e << std::endl;
           return false;
        }
    }
  // TODO; here I could reopen the cache read only!!

  return true;
}
}

int
AppCli::Run(const AppArgs& appargs)
{

  if( appargs.getPrintVersions() )
    {
      std::cout << "sbbdep version " << sbbdep::MAJOR_VERSION << "." << sbbdep::MINOR_VERSION << "."
          << sbbdep::PATCH_VERSION << std::endl;
      return 0;
    }

  try
    {
      init_all_singles(appargs.getDBName());

      if( !prepairCache(appargs.getNoSync()) )
        return -2;
    }
  catch (const a4z::Err& e)
    {
      std::cerr << e << std::endl;
      return -1;
    }

  std::ofstream outfile;
  if( appargs.getOutFile().size() )
    {
      outfile.open(appargs.getOutFile().c_str(), std::ofstream::out | std::ofstream::trunc);
      Log::getInstance()->addChannel(Log::ChannelId::AppMessage, outfile, "AppMessage");
    }
  else
    {
      Log::getInstance()->addChannel(Log::ChannelId::AppMessage, std::cout, "AppMessage");
    }

  Path querypath(appargs.getQuery());

  if( querypath.isEmpty() )
    return 0; // was a sync only call ....

  querypath.makeAbsolute();
  querypath.makeRealPath();

  Pkg pkg = Pkg::create(querypath);
  if(pkg.getType() == PkgType::Unknown)
    {
      LogInfo() << "not a file with binary dependencies: " << appargs.getQuery()
          << "\n try to find other information:\n";
      try
        {
          lookup::fileInPackages(querypath);
          return 0;
        }
      catch (const a4z::Err& e)
        {
          LogError() << e << std::endl;
          return -3;
        }
    }

  try
    {
      pkg.Load();
    }
  catch (const a4z::Err& e)
    {
      LogError() << e << std::endl;
      return -4;
    }



  if( !appargs.getWhoNeeds() && !appargs.getXDL() )
    {
      DepFileWriter dfw(appargs.getAppendVersions());
      Log::ChannelType lc = WriteAppMsg();
      dfw.generate_log( pkg, lc);
    }
  else if( appargs.getWhoNeeds() && !appargs.getXDL() )
    {
      // todo , if file is give, message that file is ignored or implement this
      DepFileWriter dfw(appargs.getAppendVersions());
      Log::ChannelType lc = WriteAppMsg();
      dfw.who_requires(pkg, lc);
    }
  else if( appargs.getXDL() )
    {
      if( appargs.getWhoNeeds() )
        {
          if( !lookup::explain_who_needs(pkg, Log::AppMessage() ) )
            LogError() << "explain dynamic linked (whoneeds) did not work\n"
                << "a better error message is in development\n";
        }
      else
        {
          if( !handleXDLrequest(pkg) )
            LogError() << "explain dynamic linked did not work\n"
                << "a better error message is in development\n";
        }

    }
  else
    {
      LogError() << "Can not run given combination of arguments \n";
      LogError() << "(could possible, but I do not want)\n";
      LogError() << "Please run just one QUERY.\n" << std::endl;
      return -6;
    }

  return 0;
}
//--------------------------------------------------------------------------------------------------

}
