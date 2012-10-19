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
#include "depfilewriter.hpp"
#include "xdl.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

#include <sbbdep/config.hpp> // generated 

#include <sbbdep/singles.hpp>
#include <sbbdep/path.hpp>

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



int
AppCli::Run(const AppArgs& appargs)
{

  if (appargs.getPrintVersions())
    {
      std::cout << "sbbdep version " 
          << sbbdep::MAJOR_VERSION << "."
          << sbbdep::MINOR_VERSION << "."
          << sbbdep::PATCH_VERSION << std::endl;
      return 0;
    }
  
  // create the cache  
  try
    {
      if( !appargs.getDBName().size() )
        {
          std::string db = std::string(std::getenv("HOME") + std::string("/sbbdep.cache"));
          init_all_singles( db ) ;
        }
      else 
        {
          init_all_singles( appargs.getDBName() ) ;
        }
    }
  catch( const a4z::Err& e )
    {
      std::cerr  << e << std::endl; 
      return -1; 
    }  

  // sync (or not) the cache
  bool nosync = appargs.getNoSync();
  if( Cache::get()->isNew() ) 
    {
      if ( nosync ) 
        {
          LogInfo() << "Cache is new, overrule nosync\n" ;
          nosync = false;
        }
      LogInfo() << "create cache " << Cache::get()->DB().Name() << "\n";
    }

 if ( !nosync )
   {
      try
        {
          Cache::get()->doSync(); 
        }
      catch( const a4z::Err& e )
        {
          LogError()  << e << std::endl;
          return -2 ; 
        }
   }
  
   // TODO; here I could reopen the cache read only!!
 
  // the more options I add the more spaghetti this becomes, so its subject to change..

   Path pn( appargs.getQuery() ) ;
   
   if ( pn.isEmpty() ) return 0; // was a sync only call ....

   // TODO, could be to early here, /var/log/packages = /var/amd/packages,...
     // also think about some validation... and exit stuff....
    pn.makeAbsolute();
    pn.makeRealPath();
    
    Pkg* pkg = 0;
    try
      {
        pkg = PkFab::get()->createPkg( pn ) ; 
      }
    catch( const a4z::Err& e )
      {
        LogError()  << e << std::endl;
        return -3 ; 
      }     
   
    // since xdl option was added this is to early here cause for XDL I do only want a file
    // so I possible load package info for just print an error
    // not a big deal or problem but can be done better, so -> TODO
    try
      {
        pkg->Load() ;
      }
    catch( const a4z::Err& e )
      {
        LogError()  << e << std::endl;
        return -4 ; 
      }    
      
    

    if (!appargs.getWhoNeeds() && !appargs.getXDL())
      {
        DepFileWriter dfw(appargs.getAppendVersions());
      if( appargs.getOutFile().size() )
        {
          std::ofstream outfile;
          outfile.open (appargs.getOutFile().c_str() , std::ofstream::out | std::ofstream::trunc) ;
          dfw.generate(*pkg , outfile ) ;
        }
      else
        {

          dfw.generate(*pkg , std::cout ) ;
          std::cout << "\n"<< std::endl;
        }
      }
    else if(appargs.getWhoNeeds())
      {
        DepFileWriter dfw(appargs.getAppendVersions());
        dfw.who_requires(*pkg , std::cout ) ;    
      }
    else if(appargs.getXDL())
      {

        if(!handleXDLrequest(pkg))
          LogError()<< "explain dynamic linked did not work\n"
          << "a better error message is in development\n";

      }
    else
      {
        LogError()  << "Can not run given combination of arguments \n" ;
        LogError()  << "(could possible, but I do not want)\n";
        LogError()  << "Please run just one QUERY.\n" << std::endl;
        return -6 ;
      }
  

    return 0; 
}
//--------------------------------------------------------------------------------------------------


}
