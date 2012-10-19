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

#include <a4sqlt3/sqlparamcommand.hpp>
#include <a4sqlt3/parameters.hpp>
#include <a4sqlt3/dataset.hpp>

#include <iostream>

namespace sbbdep
{


namespace{

class XdlCmd : public a4sqlt3::SqlParamCommand
{

public:
  XdlCmd(const std::string& sql) : a4sqlt3::SqlParamCommand(sql)
  {
  }//-----------------------------------------------------------------------------------------------

};
//--------------------------------------------------------------------------------------------------


}

bool
handleXDLrequest(Pkg* pkg)
{


  PkgOneBinLib* pkbl = dynamic_cast<PkgOneBinLib*>(pkg);

  if(not pkbl) // maybe some better message ...
    return false;

  std::cout << "Absolute path: " << pkbl->getPathName() << std::endl ;
  std::string fname = pkbl->getPathName().getBase();

  XdlCmd pkgnamequery(
      "SELECT fullname FROM pkgs INNER JOIN dynlinked ON pkgs.id = dynlinked.pkg_id "
      "WHERE dynlinked.basename = ? AND dynlinked.arch=?;");
  Cache::getInstance()->DB().CompileCommand(&pkgnamequery);

  pkgnamequery.Parameters()->at(0)->set(fname);
  if (pkbl->getArch() == Arch32)
    pkgnamequery.Parameters()->at(1)->set(32);
  else if (pkbl->getArch() == Arch64)
    pkgnamequery.Parameters()->at(1)->set(64);
  else
    {
      LogError()<< "unknow arch flag, end here " << pkbl->getArch() << std::endl;
      return false;
    }
  a4sqlt3::Dataset pkgnameds;
  Cache::getInstance()->DB().Execute(&pkgnamequery,&pkgnameds);


  if(pkgnameds.getRowCount()==0)
    {
      std::cout << " from unknown package" << std::endl ;
    }
  else
    {
      do
        {
          std::cout << " from package:" << pkgnameds.getField(0).asString() << std::endl ;
        }while(pkgnameds.moveNext());
    }

  return true;
}



} // ns
