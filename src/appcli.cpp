/*
--------------Copyright (c) 2010-2011 H a r a l d  A c h i t z---------------
-----------------< a g e dot a 4 z at g m a i l dot c o m >------------------
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


#include <iostream>
#include <fstream>

#include "sbbdep/singles.hpp"
//#include "sbbdep/filemagic.hpp"
//#include "sbbdep/cache.hpp"
//#include "sbbdep/pkfab.hpp"
//#include "a4z/singlecollector.hpp"
#include "sbbdep/path.hpp"
#include "sbbdep/depfilewriter.hpp"
#include "sbbdep/pkg.hpp"
//#include "sbbdep/error.hpp"
#include "a4z/err.hpp"





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

  // create the cache  
  try
    {
      if( !appargs.getDBName().size() )
        {
          // TODO , think about default val to set here and not in all singles
          init_all_singles( ) ;
        }
      else 
        {
          init_all_singles( appargs.getDBName().c_str() ) ; 
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
          std::cerr << "Cache is new, overrule nosync\n" ;
          nosync = false;
        }
      std::cerr << "create cache " << Cache::get()->DB().Name() << "\n";
    }

 if ( !nosync )
   {
      try
        {
          Cache::get()->doSync(); 
        }
      catch( const a4z::Err& e )
        {
          std::cerr  << e << std::endl;
          return -2 ; 
        }
   }
  
   // TODO; here I could reopen the cache read only!!
 
  

   Path pn( appargs.getQuery() ) ;
      
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
        std::cerr  << e << std::endl;
        return -3 ; 
      }     
   
    try
      {
        pkg->Load() ;
      }
    catch( const a4z::Err& e )
      {
        std::cerr  << e << std::endl;
        return -4 ; 
      }    
      
  
    // TODO remove indirect from appargs
    
    DepFileWriter dfw(  appargs.getAppendVersions());
    
    if (!appargs.getRequiredBy() )
      {
      if( appargs.getOutFile().size() )
        {
          std::ofstream outfile;
          outfile.open (appargs.getOutFile().c_str() , std::ofstream::out | std::ofstream::trunc) ;
          // TODO, check file.... or think what happens in case of error
          dfw.generate(*pkg , outfile ) ;
        }
      else
        {
          dfw.generate(*pkg , std::cout ) ;
          std::cout << "\n--------"<< std::endl;
        }
      }
    else
      {
        dfw.who_requires(*pkg , std::cout ) ;    
      }
  

    return 0; 
}
//--------------------------------------------------------------------------------------------------


}
