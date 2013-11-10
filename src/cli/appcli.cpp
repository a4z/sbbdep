/*
 --------------Copyright (c) 2010-2013 H a r a l d  A c h i t z---------------
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
#include "lookup_fileinpackages.hpp"
// and that is this one.. TODO continue refactoring
#include "report.hpp"
#include "report_sync.hpp"

#include "featurex.hpp"

#include <iostream>
#include <fstream>
#include <string>


#include <sbbdep/config.hpp> // generated 
#include <sbbdep/path.hpp>
#include <sbbdep/log.hpp>

// should possible not be required here
#include <sbbdep/pkg.hpp>
#include <sbbdep/error.hpp>



namespace sbbdep {

AppCli::AppCli()
{

}
//--------------------------------------------------------------------------------------------------

AppCli::~AppCli()
{

}
//--------------------------------------------------------------------------------------------------

namespace {
bool
prepairCache(bool syncflag)
{ // TODO name this nosync ? or add docu other wise its always hard to read first time comming here
  bool chache_was_new{false};
  if( Cache::get().DB().isNew() )
    {
      if( syncflag )
        {
          LogInfo() << "Cache is new, overrule nosync\n";
          syncflag = false;
        }
      chache_was_new = true;
    }

  try // TODO in new implementation this will be automatically called at open db
    { //us major minor combination to see if schema has changed in an way that it needs to be re-created
      Cache::get().DB().checkVersion(
          sbbdep::MAJOR_VERSION,
          sbbdep::MINOR_VERSION ,
          sbbdep::PATCH_VERSION );
    }
  catch (const Error& e)
    {
      LogError() << e << std::endl;
      return false;
    }

  if( !syncflag )
    {
      try
        {
          cli::printSyncReport( Cache::get().doSync(), chache_was_new );
        }
      catch (const Error& e)
        {
          LogError() << e << std::endl;
           return false;
        }
      catch (...)
        { // TODO, cou
          LogError() << "Unknown exception" << std::endl;
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

  std::ofstream outfile;
  if( appargs.getOutFile().size() )
    {
      outfile.open(appargs.getOutFile().c_str(), std::ofstream::out | std::ofstream::trunc);
      LogSetup::create(outfile, appargs.getQuiet()) ;
    }
  else
    {
      LogSetup::create(std::cout, appargs.getQuiet()) ;
    }


  try
    {
      Cache::open( appargs.getDBName() );

      if( !prepairCache( appargs.getNoSync() ) )
        return -2;
    }
  catch (const Error& e)
    {
      std::cerr << e << std::endl;
      return -1;
    }
  catch (...)
    {
      std::cerr << "Unknown error" << std::endl;
      return -1;
    }

  Path querypath( appargs.getQuery() );


  if(appargs.getFeatureX())
    {
      sbbdep::cli::runFx() ;
      return 0  ;
    }


  if( querypath.isEmpty() )
    return 0; // was a sync only call ....

  querypath.makeAbsolute();
  querypath.makeRealPath();

  Pkg pkg = Pkg::create(querypath);
  if(pkg.getType() == PkgType::Unknown)
    {
      try
        {
          if(querypath.isValid())
            {
              LogInfo() << "not a file with binary dependencies: " ;
              LogInfo() << querypath  << "\n try to find information in package list:\n";
              lookup::fileInPackages(querypath);
            }
          else
            {
              LogInfo() << "not a file path: '" << appargs.getQuery() ;
              LogInfo() << "', use name as filename and ";
              LogInfo() << " try to find filename in package list:\n" ;
              lookup::fileInPackages(Path(appargs.getQuery()));
            }

          return 0;
        }
      catch (const Error& e)
        {
          LogError() << e << std::endl;
          return -3;
        }
      catch (...)
        {
          LogError() << "Unknown error" << std::endl;
          return -3;
        }
    }

  try
    {
      pkg.Load(); // TODO , check the return value
    }
  catch (const Error& e)
    {
      LogError() << e << std::endl;
      return -4;
    }
  catch (...)
    {
      LogError() << "Unknown error" << std::endl;
      return -4;
    }


  if( appargs.getWhoNeeds() )
    { // TODO test what happens if a DESTDIR is given as pkg :-)
      try
        {
          cli::printWhoNeed( pkg , appargs.getAppendVersions(), appargs.getXDL() ) ;
        }
      catch (const Error& e)
        {
          LogError() << e << std::endl;
          return -5;
        }
      catch (...)
        {
          LogError() << "Unknown error" << std::endl;
          return -5;
        }
    }
  else if( !appargs.getWhoNeeds()  )
    {
      cli::printRequired( pkg ,  appargs.getAppendVersions(), appargs.getXDL(), appargs.getLdd() ) ;
    }
  else // TODO , this is more or less obsolete
    {
      LogError() << "Can not run given combination of arguments \n";
      LogError() << "(could possible, but I do not want)\n";
      LogError() << "Please run just one QUERY.\n" << std::endl;
      return -6;
    }

  Cache::close(); // would auto clean but call it
  return 0;
}
//--------------------------------------------------------------------------------------------------

}
