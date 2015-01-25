/*
 --------------Copyright (c) 2010-2015 H a r a l d  A c h i t z---------------
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


#include "report.hpp"

#include "featurex.hpp"

#include <iostream>
#include <fstream>
#include <string>


#include <sbbdep/config.hpp> // generated 
#include <sbbdep/cache.hpp>
#include <sbbdep/path.hpp>
#include <sbbdep/log.hpp>

// should possible not be required here
#include <sbbdep/pkg.hpp>
#include <sbbdep/error.hpp>



namespace sbbdep {

AppCli::AppCli()
{

}
//------------------------------------------------------------------------------

AppCli::~AppCli()
{

}
//------------------------------------------------------------------------------

namespace
{

  Cache
  openCache(const std::string& name)
  {
    try
      {
        return Cache (name);
      }
    catch (...)
      {
        LogError () << "can not open cache " << name;
        throw;
      }
   }//--------------------------------------------------------------------------

} // ns ------------------------------------------------------------------------

int
AppCli::Run(const AppArgs& appargs)
{
  // TODO if no cache access in search. -l no bin file, no cache update...

  if (appargs.getPrintVersions ())
    {
      std::cout << "sbbdep version "
          << sbbdep::MAJOR_VERSION << "."
          << sbbdep::MINOR_VERSION << "."
          << sbbdep::PATCH_VERSION << std::endl;
      return 0;
    }

  std::ofstream outfile;
  if (appargs.getOutFile ().size ())
    {
      outfile.open(appargs.getOutFile().c_str(),
                   std::ofstream::out | std::ofstream::trunc);
      LogSetup::create (outfile, appargs.getQuiet ()) ;
    }
  else
    {
      LogSetup::create (std::cout, appargs.getQuiet ()) ;
    }

  if(appargs.getLookup())
    {
      if (appargs.getQuery ().empty ())
        {
          LogInfo () << "error: lookup query missing, no argument provided" ;
          return  1;
        }
       cli::lookupInPackages(appargs.getQuery ()) ;
       return 0;
    }

  Path dbpath (appargs.getDBName ()) ;

  if (not dbpath.isRegularFile () and appargs.getNoSync ())
    {
      LogInfo () << appargs.getDBName () << " does not exist and need to "
          "be created. nosync makes therefore no sense. I exit now." ;
      return 2 ;
    }

  Cache cache = openCache (appargs.getDBName ()) ;
  if (cache.isNewDb () and appargs.getNoSync ())
    { // could also have an empty db ...
      LogInfo () << appargs.getDBName () << " db is empty and needs to "
          "be created. nosync makes therefore no sense. I exit now." ;
      return 2 ;
    }


  if (appargs.getFeatureX ())
    {
      cli::runFx (appargs.getFeatureXArgs ()) ;
      return 0  ;
    }


  if (not appargs.getNoSync ())
    {
      auto syncdata = cache.doSync () ;
      cli::printSyncReport (cache, syncdata) ;
    }


  if (appargs.getQuery ().empty ())
    { // was a sync only call ....
      return 0;
    }

  Path querypath (appargs.getQuery ());

  querypath.makeRealPath ();

  Pkg pkg = Pkg::create (querypath);
  if(pkg.getType () == PkgType::Unknown)
    {
      try
        {
          if(querypath.isValid())
            {
              LogInfo () << "not a file with binary dependencies: "
                  << querypath
                  << "\n try to find information in package list:";
              cli::fileInPackages (querypath);
            }
          else
            { // TODO  , possible this should be the -l option ?
              LogInfo () << "not a file path: '" << appargs.getQuery () ;
              LogInfo () << " try to find some info in package list:" ;
              cli::fileInPackages (Path (appargs.getQuery ()));
            }

          return 0;
        }
      catch (const Error& e)
        {
          LogError () << e ;
          return -3;
        }
      catch (...)
        {
          LogError () << "Unknown error" ;
          return -3;
        }
    }

  try
    {
      SBBASSERT (pkg.Load ()) ;
    }
  catch (const Error& e)
    {
      LogError () << e ;
      return -4;
    }
  catch (...)
    {
      LogError () << "Unknown error" ;
      return -4;
    }


  if (appargs.getWhoNeeds ())
    { // TODO test what happens if a DESTDIR is given as pkg :-)
      try
        {
          cli::printWhoNeed (cache, pkg ,
                             appargs.getAppendVersions (),
                             appargs.getXDL ()) ;
        }
      catch (const Error& e)
        {
          LogError () << e ;
          return -5;
        }
      catch (...)
        {
          LogError () << "Unknown error";
          return -5;
        }
    }
  else if (not appargs.getWhoNeeds ())
    {
      cli::printRequired (cache, pkg ,
                          appargs.getAppendVersions (),
                          appargs.getXDL (),
                          appargs.getLdd ()) ;
    }
  else
    {
      LogError () << "Can not run given combination of arguments";
      LogError () << "(could possible, but I do not want)";
      LogError () << "Please run just one QUERY." << std::endl;
      return -6;
    }


  return 0;
}
//------------------------------------------------------------------------------

}
