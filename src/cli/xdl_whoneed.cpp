/*
--------------Copyright (c) 2012-2012 H a r a l d  A c h i t z---------------
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


#include <sbbdep/pkgonebinlib.hpp>
#include <sbbdep/log.hpp>

#include <sbbdep/cache.hpp>
#include <sbbdep/cachedb.hpp>
#include <sbbdep/cachesql.hpp>
#include <sbbdep/dynlinked.hpp>
#include <sbbdep/dynlinkedinfo.hpp>

#include <a4sqlt3/sqlparamcommand.hpp>
#include <a4sqlt3/parameters.hpp>
#include <a4sqlt3/dataset.hpp>

#include <boost/algorithm/string.hpp>

//#include <iostream>
#include <memory>

namespace sbbdep
{

namespace{

int getArch(PkgOneBinLib* pkbl)
{
  if(pkbl->getDynLinkedInfos().size() != 1)
    return 0;

  return pkbl->getDynLinkedInfos().begin()->arch;
}//-------------------------------------------------------------------------------------------------


} // ano ns



bool
handleXDLwhoneed(Pkg* pkg)
{


  PkgOneBinLib* pkbl = dynamic_cast<PkgOneBinLib*>(pkg);

  if(not pkbl)
    {
      LogError()<< "Can not handle " << pkg->getPathName()  << std::endl;
      return false;
    }


  WriteAppMsg() << "Absolute path: " << pkbl->getPathName() << std::endl ;


  if ( getArch(pkbl) == 0 )
    {
      LogError()<< "unable to get ARCH of " << pkbl->getPathName() << std::endl;
      return false;
    }



  return true;
}



} // ns
