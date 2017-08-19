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

#include "appcli.hpp"
#include "appargs.hpp"


#include "report.hpp"

#include "featurex.hpp"

#include <iostream>
#include <fstream>
#include <string>


#include <sbbdep/config.hpp> // generated 
#include <sbbdep/cache.hpp>
#include <sbbdep/dircontent.hpp>
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
AppCli::run(const AppArgs& args)
{

  if (args.versions ())
    {
      std::cout << "sbbdep version "
          << sbbdep::MAJOR_VERSION << "."
          << sbbdep::MINOR_VERSION << "."
          << sbbdep::PATCH_VERSION << std::endl;
      return 0;
    }

  std::ofstream outfile;
  if (args.getOutFile ().size ())
    {
      outfile.open(args.getOutFile ().c_str (),
                   std::ofstream::out | std::ofstream::trunc);

      if (not outfile.good ())
        {
          std::cerr << "can not open logfile " << args.getOutFile () << '\n';
          return 1;
        }

      LogSetup::create (outfile, args.quiet ()) ;
    }
  else
    {
      LogSetup::create (std::cout, args.quiet ()) ;
    }


  PkgAdmDir::set (args.varAdmDir ()) ;


  if (args.lookup ())
    {
      if (args.query ().empty ())
        {
          LogInfo () << "error: lookup query missing, no argument provided" ;
          return  1;
        }
       cli::lookupInPackages(args.query ()) ;

       return 0;

    }

  Path dbpath (args.dbname ()) ;

  if (not dbpath.isRegularFile () and args.noSync ())
    {
      LogInfo () << args.dbname () << " does not exist and need to "
          "be created. nosync makes therefore no sense. I exit now." ;
      return 2 ;
    }



  Cache cache = openCache (args.dbname ()) ;
  if (cache.isNewDb () and args.noSync ())
    { // could also have an empty db ...
      LogInfo () << args.dbname () << " db is empty and needs to "
          "be created. nosync makes therefore no sense. I exit now." ;
      return 2 ;
    }


  if (args.featureX ())
    {
      cli::runFx (args.featureXArgs ()) ;
      return 0  ;
    }



  Path querypath (args.query ());
  if (not args.query ().empty())
    {
      querypath.makeRealPath ();
    }
  Pkg pkg = args.query ().empty() ?
      Pkg() : Pkg::create (querypath) ;

  if(not args.query ().empty() and pkg.getType () == PkgType::Unknown)
    {
      try
        {
          if(querypath.isValid ())
            {
              LogInfo () << "not a file with binary dependencies: "
                  << querypath
                  << "\n try to find information in package list:";
              cli::fileInPackages (querypath);
              return 0;
            }
          else
            {
              LogInfo () << "not a file path: '" << args.query () ;
              LogInfo () << "you might want to use the lookup option (-l)"
                        " to search for " << args.query ()
                         << " in the package database" ;
              return 33;
            }


        }
      catch (const Error& e)
        {
          LogError () << e ;
          return 3;
        }
      catch (...)
        {
          LogError () << "Unknown error" ;
          return 3;
        }
    }



  if (not args.noSync ())
    {
      auto syncdata = cache.doSync () ;
      cli::printSyncReport (cache, syncdata) ;
    }


  if (args.query ().empty ())
    { // was a sync only call ....
      return 0;
    }



  try
    {
      SBBASSERT (pkg.Load ()) ;
    }
  catch (const Error& e)
    {
      LogError () << e ;
      return 4;
    }
  catch (...)
    {
      LogError () << "Unknown error" ;
      return 4;
    }


  if (args.whoNeeds ())
    {
      try
        {
          cli::printWhoNeed (cache, pkg ,
                             args.ingore () ,
                             args.shortNames (),
                             args.xdl ()) ;
        }
      catch (const Error& e)
        {
          LogError () << e ;
          return 5;
        }
      catch (...)
        {
          LogError () << "Unknown error";
          return 5;
        }
    }
  else if (args.bdtree ())
    {
      cli::bdTree(cache, pkg, args.shortNames ()) ;
    }
  else // if no other option, than it is required...
    {
      cli::printRequired (cache, pkg ,
                          args.ingore () ,
                          args.shortNames  (),
                          args.xdl (),
                          args.ldd ()) ;
    }
//  else
//    {
//      LogError () << "Can not run given combination of arguments";
//      LogError () << "(could possible, but I do not want)";
//      LogError () << "Please run just one QUERY." << std::endl;
//      return 6;
//    }


  return 0;
}
//------------------------------------------------------------------------------

}
