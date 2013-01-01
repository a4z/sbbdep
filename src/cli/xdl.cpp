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

class XdlCmd : public a4sqlt3::SqlParamCommand
{

public:
  XdlCmd(const std::string& sql) : a4sqlt3::SqlParamCommand(sql)
  {
  }//---------------iostream--------------------------------------------------------------------------------

};
//--------------------------------------------------------------------------------------------------


int getArch(PkgOneBinLib* pkbl)
{
  if(pkbl->getDynLinkedInfos().size() != 1)
    return 0;


  return pkbl->getDynLinkedInfos().begin()->arch;

}

// lets see if and how we pass this around
typedef std::shared_ptr<a4sqlt3::Dataset> DatasetPtr;


void
printRequiredBy(DynLinkedInfo& dlinfos)
{

  std::string query = CacheSQL::SearchRequiredByLib();
  std::string toreplace = ";";
  std::string replacewith = " ORDER BY pkg, dynlinked.filename;";
  boost::replace_last(query, toreplace,replacewith);

  XdlCmd cmd(query);
  Cache::getInstance()->DB().CompileCommand(&cmd);

  cmd.Parameters()->at(0)->set(dlinfos.soName);
  cmd.Parameters()->at(1)->set(dlinfos.arch);

  DatasetPtr ds = std::make_shared<a4sqlt3::Dataset>();
  Cache::getInstance()->DB().Execute(&cmd, ds.get());

  for( auto& flds : *ds ){
      WriteAppMsg() << "required by pgk " << flds.getField("pkgname").asString() ;
      WriteAppMsg() << " file " << flds.getField("dynlinked").asString() << "\n";
  }


}


void
printRequires(DatasetPtr dspkg, PkgOneBinLib* pkbl)
{

  std::string sql =
  "SELECT pkgs.fullname as pkg, dynlinked.filename as file, dynlinked.soname as soname"
  " FROM pkgs INNER JOIN dynlinked ON pkgs.id = dynlinked.pkg_id"
  " WHERE dynlinked.soname=? AND dynlinked.arch=? "
  " AND dirname IN (SELECT dirname FROM lddirs "
  " UNION SELECT dirname FROM ldlnkdirs UNION SELECT dirname FROM ldusrdirs"
  " UNION SELECT replaceOrigin( ldpath, dynlinked.dirname) from rrunpath "
  " WHERE  dynlinked_id =  dynlinked.id)"
  ";"
  ;

  auto isInOwnPkgDataSet = [dspkg](const std::string& pkgname){
    for(auto& flds : *dspkg)
      {
        if (flds.getField(0).getString() == pkgname)
          return true;
      }
    return false;
  };

  XdlCmd cmd(sql);
  Cache::getInstance()->DB().CompileCommand(&cmd);

  DynLinkedInfo dlinfos = *pkbl->getDynLinkedInfos().begin() ;
  typedef std::map<std::string , std::string> DepMapType;
  DepMapType depmap , ownmap , similarmap;
  StringList notfound  ;
  StringSet ownsonames;

  auto addToMap = [](DepMapType& map, const std::string& fname, const std::string& pkgname )
    {
      auto findIter = map.find(fname);
      if(findIter==map.end())
        map.insert(DepMapType::value_type(fname, pkgname));
      else
        findIter->second+=",  " + pkgname;
    };

  for( auto& soname : dlinfos.Needed )
    {
      cmd.Parameters()->at(0)->set( soname );
      cmd.Parameters()->at(1)->set( dlinfos.arch );
      a4sqlt3::Dataset ds;
      Cache::getInstance()->DB().Execute(&cmd, &ds);

      if(ds.npos())
        notfound.push_back(soname);

      for(auto& flds : ds)
        {
          if (isInOwnPkgDataSet(flds.getField(0).getString()))
            {
              addToMap( ownmap,flds.getField(1).getString(), flds.getField(0).getString() );
              ownsonames.insert(flds.getField(2).getString());
            }
        }
      for(auto& flds : ds)
        {
          if (!isInOwnPkgDataSet(flds.getField(0).getString()))
            {
              if(ownsonames.find(flds.getField(2).getString()) == ownsonames.end())
                addToMap( depmap,flds.getField(1).getString(), flds.getField(0).getString() );
              else
                addToMap( similarmap,flds.getField(1).getString(), flds.getField(0).getString() );
            }
        }


    }

    if(ownmap.size() > 0 )
      WriteAppMsg() << "dependencies from own package:\n" ;
    for(auto iter : ownmap)
      {
        WriteAppMsg() << "  " << iter.first << " : " << iter.second << "\n";
      }
    if(similarmap.size() > 0 )
    WriteAppMsg() << "dependencies in own and others:\n" ;
    for(auto iter : similarmap)
      {
        WriteAppMsg() << "  "<< iter.first << " : " << iter.second << "\n";
      }
    WriteAppMsg() << "dependencies from others:\n" ;
    for(auto iter : depmap)
      {
        WriteAppMsg() << "  " << iter.first << " : " << iter.second << "\n";
      }

    if(notfound.size() > 0)
      WriteAppMsg() << "possible problems:\n" ;
    for(auto iter : notfound)
      {
        WriteAppMsg() << "  NOT FOUND in standard link paths: " << iter << "\n";
      }
}

DatasetPtr
printPackageInfo(PkgOneBinLib* pkbl)
{
  XdlCmd cmd(
      "SELECT fullname FROM pkgs INNER JOIN dynlinked ON pkgs.id = dynlinked.pkg_id "
      "WHERE dynlinked.filename = ? AND dynlinked.arch=?;");
  Cache::getInstance()->DB().CompileCommand(&cmd);
  cmd.Parameters()->at(0)->set( pkbl->getPathName() );
  cmd.Parameters()->at(1)->set( getArch(pkbl) );

  DatasetPtr ds = std::make_shared<a4sqlt3::Dataset>();
  Cache::getInstance()->DB().Execute(&cmd, ds.get());

  if(ds->getRowCount()==0)
    {
      WriteAppMsg() << pkbl->getPathName() << " not in a known package" << std::endl ;
    }
  else
    {
      for( auto& flds : *ds )
        {
          WriteAppMsg() << " from package:" << flds.getField(0).asString() << std::endl ;
        }
    }

  return ds;

}

void printSoInfo(DatasetPtr dspkg, const DynLinkedInfo& dlinfos )
{
  if(dlinfos.soName.empty())
    return;

  WriteAppMsg() << "link name is: " << dlinfos.soName << std::endl;

  std::string pkgname = dspkg->npos() ? "" : dspkg->getField(0).asString();

  std::string sql = R"~(
SELECT pkgs.fullname, dynlinked.filename 
FROM pkgs INNER JOIN dynlinked ON pkgs.id = dynlinked.pkg_id 
WHERE dynlinked.soname=? AND dynlinked.arch=?  
  AND dirname IN (SELECT dirname FROM lddirs  
    UNION SELECT dirname FROM ldlnkdirs UNION SELECT dirname FROM ldusrdirs 
    UNION SELECT lddir from rrunpath  
    WHERE  dynlinked_id =  dynlinked.id) 
  AND pkgs.fullname != ? 
 ; 
  )~" ;


  XdlCmd cmd(sql) ;
  Cache::getInstance()->DB().CompileCommand(&cmd);
  cmd.Parameters()->at(0)->set( dlinfos.soName );
  cmd.Parameters()->at(1)->set( dlinfos.arch );
  cmd.Parameters()->at(2)->set( pkgname );
  DatasetPtr ds = std::make_shared<a4sqlt3::Dataset>();
  Cache::getInstance()->DB().Execute(&cmd, ds.get());

  if(!ds->npos())
    {
      WriteAppMsg()<<  "  also provided from:\n" ;
      for( auto& flds : *ds )
        {
          WriteAppMsg() << "   " << flds.getField(0).asString() << ":"<<  flds.getField(1).asString() <<std::endl ;
        }
    }

}

} // ano ns


bool
handleXDLrequest(Pkg* pkg)
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


  DatasetPtr ds_package = printPackageInfo(pkbl);

  //binlib pkg must have exactly one info ....
  DynLinkedInfo dlinfos = *pkbl->getDynLinkedInfos().begin() ;

  printSoInfo(ds_package, dlinfos); // if nothing to do, func will do nothing
  printRequires(ds_package, pkbl);

  // TODO , can I change the dyn linked info list to a vector without side effects ?
  // should be possible ?

  //printRequiredBy(dlinfos);

  return true;
}



} // ns
