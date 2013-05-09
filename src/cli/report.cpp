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


#include "report.hpp"

#include <sbbdep/log.hpp>
#include <sbbdep/pkgname.hpp>
#include <sbbdep/pkg.hpp>
#include <sbbdep/cache.hpp>
#include <sbbdep/elffile.hpp>
#include <sbbdep/cachesql.hpp>


#include <a4sqlt3/sqlcommand.hpp>
#include <a4sqlt3/dataset.hpp>
#include <a4sqlt3/columns.hpp>

#include <set>
#include <algorithm>
#include <tuple>
#include <iterator>

namespace sbbdep {
namespace cli{



//--------------------------------------------------------------------------------------------------


void printSyncReport(Cache::SyncData syncdata)
{
  using StringSet = std::set<std::string> ; //make it sortd

  StringSet deleted, reinstalled, installed, upgraded;


  // take removed, than check inserted, if inserted is also in deleted, its an update
  deleted.insert(syncdata.removed.begin(),syncdata.removed.end() );
  // so make all inserted to deleted,
  // take all installed and search if deleted,
  // if found , remove from deleted and add to upgraded, else it is inserted

  for(auto& val : syncdata.installed)
    {   // need package name, if the name is in removed and in installed, than
      PkgName pkgname(val);
      auto compair = [&pkgname](const std::string& fullname)->bool
        {
          return pkgname.Name() == PkgName(fullname).Name();
        };

      auto isdeleted = std::find_if(deleted.begin(), deleted.end(), compair);
      if( isdeleted == deleted.end() )
        {
          installed.insert(val);
        }
      else
        {
          PkgName delname(*isdeleted);
          std::string updateinfo = val +
              "  (was " + delname.FullName().substr( delname.Name().size()+1 ) +")";
          upgraded.insert(updateinfo);
          deleted.erase(isdeleted);
        }

    }

  // finally transfer the re installed and then print all
  reinstalled.insert(syncdata.reinstalled.begin(), syncdata.reinstalled.end());


  if(not deleted.empty()
      or not upgraded.empty()
      or not installed.empty()
      or not reinstalled.empty())
    LogInfo()    << "\n---synchronisation summary:\n" ;


  for(auto& val : deleted) LogInfo()    << "deleted: " << val << std::endl;
  for(auto& val : upgraded) LogInfo()   << "upgraded: " << val  << std::endl;
  for(auto& val : installed) LogInfo()  << "installed: " << val  << std::endl;
  for(auto& val : reinstalled) LogInfo()<< "reinstalled: " << val  << std::endl;

  LogInfo() << "\n";
}

//--------------------------------------------------------------------------------------------------


namespace
{

class ReportSet : public a4sqlt3::Dataset
{
public:
  ReportSet(const std::vector<std::string> fieldnames)
  {
    for(std::size_t i = 0; i < fieldnames.size(); ++i)
        m_namemap.insert( NameMap::value_type(fieldnames[i], i) ) ;
  }

  void merge( const a4sqlt3::Dataset& other )
  {
    a4sqlt3::Dataset::merge(other) ;
  }

} ;



auto no_conversion = [](const std::string& val) -> std::string { return val;};

template<typename T>
std::string joinToString(T container,  const std::string join,
    std::function<std::string(const std::string&)> converter = no_conversion )
{

  std::string retval;

  if( container.empty() )
    return retval;

  auto pos = container.begin();
  retval+=converter(*pos);

  while( ++pos != container.end())
    {
      retval += join  + converter (*pos) ;
    }

  return retval;

}

using StringVec = std::vector<std::string>;
using StringSet = std::set<std::string>;
using NotFoundMap = std::map<std::string, StringSet>; // from file, so names

template<typename T>
StringSet getKeySet(const T& keyvalmap){
  StringSet retval;
  for(auto val: keyvalmap) retval.insert(val.first);
  return retval;
}




struct ReportElement{

  using Node =  std::map< std::string, ReportElement >  ;

  Node node ;

  ReportElement() = default;

  ReportElement( std::string s, ReportElement e ) : node {{s,e}}
  {
  }

  void add(StringVec path)
  {
    if(not path.empty())
      node[*(path.begin())].add( StringVec(  ++(path.begin()), path.end() ) ) ;
  }

};


struct ReportTree
{
  ReportElement::Node node ;

  void add(StringVec path)
  {
    if(not path.empty())
      node[*(path.begin())].add( StringVec(  ++(path.begin()), path.end() ) ) ;
  }
};


void printTree(ReportTree& tree)
{

  std::function<void(ReportElement, int)> printChild = [&printChild]( ReportElement elem , int level ){

    for(auto node: elem.node){
       for(int i = 0; i < level; ++i)
            std::cout << " " ;

      std::cout << node.first << "\n";
      printChild(node.second, level+2);
    }

  } ;

  for( auto elem : tree.node )
  {
    std::cout << elem.first << std::endl;
    printChild(elem.second, 2) ;
  }
}
//--------------------------------------------------------------------------------------------------


bool isRRunPath(const std::string& dirname)
{
  using namespace a4sqlt3;
  SqlCommand* cmd = Cache::getInstance()->DB().getCommand("isRRunpathDirectory");
  if( cmd == nullptr )
    {
      std::string sql = "SELECT count(*)  FROM rrunpath WHERE rrunpath.lddir NOT IN "
          " (SELECT dirname FROM lddirs UNION SELECT dirname FROM ldlnkdirs) "
          " AND rrunpath.lddir = ? ;" ;

      cmd = Cache::getInstance()->DB().createStoredCommand(
          "isRRunpathDirectory" ,  sql );
    }

  cmd->Parameters().Nr(1).set(dirname);

  Dataset ds;
  Cache::getInstance()->DB().Execute(cmd, &ds);
  return ds.getField(0).getInt() > 0 ;

}
//--------------------------------------------------------------------------------------------------

bool isLinkPath(const std::string& dirname)
{
  using namespace a4sqlt3;
  SqlCommand* cmd = Cache::getInstance()->DB().getCommand("isLinkPathDirectory");
  if( cmd == nullptr )
    {
      std::string sql =" SELECT count(*) FROM "
         " (SELECT dirname FROM lddirs WHERE dirname = ? "
            " UNION SELECT dirname FROM ldlnkdirs WHERE dirname = ?) ";

      cmd = Cache::getInstance()->DB().createStoredCommand(
          "isLinkPathDirectory" ,  sql );
    }

  cmd->Parameters().setValues( {DbValue(dirname), DbValue(dirname)} ) ;

  Dataset ds;
  Cache::getInstance()->DB().Execute(cmd, &ds);
  return ds.getField(0).getInt() > 0 ;

}
//--------------------------------------------------------------------------------------------------

a4sqlt3::Dataset
getPkgsFromFile(const PathName& fname, int arch)
{
  using namespace a4sqlt3;
  SqlCommand* cmd = Cache::getInstance()->DB().getCommand("SearchPgkOfFile");
  if( cmd == nullptr )
    cmd = Cache::getInstance()->DB().createStoredCommand(
        "SearchPgkOfFile" ,CacheSQL::SearchPgkOfFile());


  cmd->Parameters().setValues( DbValueList{
    DbValue(fname.getDir()),
    DbValue(fname.getBase()),
    DbValue(arch) });

  Dataset ds;
  Cache::getInstance()->DB().Execute(cmd, &ds);
  return ds;
}


} // ano ns
//--------------------------------------------------------------------------------------------------

a4sqlt3::Dataset elfdeps(const PathName& fromfile, const ElfFile::StringVec& needed,
    int arch, const ElfFile::StringVec& rrnupaths)
{

  auto quote = [](const std::string val) -> std::string{
    return  std::string("'") + val + std::string("'") ;
  };

/* todo after fixing the soname, use these for testing
 * select * from dynlinked where soname = 'libuno_sal.so.3' ;

  select * from dynlinked where basename = 'libcomphelpgcc3.so'  ;

 /usr/lib64/libreoffice/program/libcomphelpgcc3.so and othe rlibreoffice files...
 *
 */

  auto quoterpath = [&fromfile, &quote](const std::string val) -> std::string{
    std::string pathname = CacheSQL::replaceORIGIN(val, fromfile.getDir());
    if(pathname.empty()) return "'no/where'";
    Path p(pathname);
    p.makeRealPath() ;
    return  quote(p.getURL()) ;
  };
  // TODO , check len  needed or sql , there is are SQLite limits

  std::string insonames = joinToString(needed, ",", quote);
  std::string archstr = std::to_string( arch );
  std::string unionrunpath;

  if(not rrnupaths.empty())
    {
      std::string inrunpath = joinToString(rrnupaths , ",", quoterpath);
      unionrunpath = " UNION SELECT lddir FROM rrunpath WHERE lddir IN (" + inrunpath + ") ";
    }

  /*
    SELECT pkgs.fullname as pkgname,  dynlinked.filename , dynlinked.soname
    FROM pkgs INNER JOIN dynlinked ON pkgs.id = dynlinked.pkg_id
    WHERE dynlinked.soname IN ( ... ) AND dynlinked.arch = ...
    AND  dynlinked.dirname IN (
      SELECT dirname FROM lddirs UNION SELECT dirname FROM ldlnkdirs
      UNION SELECT UNION SELECT lddir FROM rrunpath WHERE lddir IN ( ... ) // optional
      );
  */
  std::string sql= "SELECT pkgs.fullname as pkgname,  dynlinked.filename , dynlinked.soname,  " ;
  sql+= "'" + fromfile.Str() + "' as  requiredby" ;
  sql+=" FROM pkgs INNER JOIN dynlinked ON pkgs.id = dynlinked.pkg_id"
  " WHERE dynlinked.soname IN ( " ;
  sql+=insonames ;
  sql+= " ) AND dynlinked.arch = ";
  sql+= archstr ;
  sql+= " AND  dynlinked.dirname IN  " ;
  sql+= " ( SELECT dirname FROM lddirs UNION SELECT dirname FROM ldlnkdirs ";
  sql+= unionrunpath ;
  sql+= " );" ;


  using namespace a4sqlt3;
  Dataset ds ;
  Cache::getInstance()->DB().Execute(sql, &ds) ;
  return ds;



}
//--------------------------------------------------------------------------------------------------

std::tuple<ReportSet, NotFoundMap> // ds(pkgname, filename , soname) and NotFound
getRequiredInfos(const Pkg& pkg)
{

  using namespace a4sqlt3;

  StringSet known_needed;

  NotFoundMap not_found;

  ReportSet rs {{ "pkgname", "filename", "soname" , "requiredby" } };

  for(const ElfFile& elf : pkg.getDynLinked())
    {

      StringVec needed = elf.getNeeded();

      // but already looked up to the end
      auto knownbegin = std::remove_if(std::begin(needed), std::end(needed),
          [&known_needed](const std::string& val) ->bool
            {
              return known_needed.find(val) != known_needed.end();
            });

      if( needed.empty() )
        continue;
      else // remember already looked up
        known_needed.insert(knownbegin, needed.end());
      //LogDebug() << "HERE2: " << elf.getName().Str() << std::endl;
      Dataset ds = elfdeps(elf.getName(), needed, elf.getArch(), elf.getRRunPaths());
      // pkgs.fullname as pkgname,  dynlinked.filename , dynlinked.soname

      // move found stuff to the end
      auto notfound_end = std::remove_if(std::begin(needed), std::end(needed),
          [&ds](const std::string& val) ->bool
            {
              for(auto& row : ds)
              if(row.getField(2).getString() == val )
              return true;

              return false;
            });

      // if something was not found, it's now at the begin
      StringSet notFounds;
      notFounds.insert(needed.begin(), notfound_end);
      if(not notFounds.empty())
          not_found.insert(NotFoundMap::value_type(elf.getName().Str(), notFounds));


      rs.merge(ds); // put this record to the report

    }

  return std::make_tuple(rs, not_found);

}
//--------------------------------------------------------------------------------------------------





void printRequired( const Pkg& pkg, bool addversion, bool xdl )
{

  std::tuple<ReportSet, NotFoundMap> requiredinfo = getRequiredInfos(pkg);
  ReportTree reptree;

  //pkgname,  filename , soname
  ReportSet& rs = std::get<0>(requiredinfo) ;
  NotFoundMap& notFounds = std::get<1>(requiredinfo) ;


  for(auto& row : rs)
    {
      if(xdl)
        {
        reptree.add( {row.getField(3).getString(),
          row.getField(2).getString(),
          row.getField(1).getString() ,
          row.getField(0).getString() } );
        }
      else
        {
          reptree.add( {
            row.getField(2).getString(),
            row.getField(1).getString() ,
            row.getField(0).getString() } );
        }
    }

  if( pkg.getType() == PkgType::BinLib )
    {

      a4sqlt3::Dataset ds = getPkgsFromFile( pkg.getPath(), pkg.getArch() ) ;
      // what to do with this , just a report summary ?

    }

  auto makename = [addversion, xdl](const std::string val)
    {

      if(xdl)
        {
          if(addversion)
            return val;

          return PkgName(val).Name();
        }

      PkgName pknam(val);
      std::string retval = pknam.Name();
      if (addversion) retval+= " >= " + pknam.Version();
      return retval;


    };

  if( xdl )
    {
      for(auto requiredby_sos : reptree.node)
        {
          Log::AppMessage() << "file " << requiredby_sos.first << " needs:\n";
          for(auto so_files : requiredby_sos.second.node)
            {
              Log::AppMessage() << "  " << so_files.first << " found in:\n";
              for(auto file_pkgs : so_files.second.node)
                {
                  Log::AppMessage() << "    " << file_pkgs.first << "( "
                      << joinToString(getKeySet(file_pkgs.second.node), " | ", makename) << " )\n";
                }
            }
        }
    }
  else
    {
      StringSet pkglist;
      std::string ignore_name ;
      if(pkg.getType() == PkgType::Installed)
        ignore_name = pkg.getPath().getBase();


      for(auto so_files: reptree.node) // filename so, just what the pkgs...
        {
          StringSet pkgsofso;
          for(auto file_pkgs : so_files.second.node )
            {
              if(not ignore_name.empty() &&
                  file_pkgs.second.node.find(ignore_name) != file_pkgs.second.node.end() )
                { // avoid own package in the list
                  pkgsofso.clear();
                  break;
                }
              for (auto so_pkg : file_pkgs.second.node)
                pkgsofso.insert( so_pkg.first ) ;
            }

          if(not pkgsofso.empty())
            pkglist.insert( joinToString(pkgsofso, " | ", makename ) ) ;
        }

      Log::AppMessage() << joinToString(pkglist, addversion ? "\n" : ", " ) ;

    }
    Log::AppMessage() << std::endl;


    if(not notFounds.empty())
      {
        Log::AppMessage() << std::endl;
        Log::AppMessage() << "sonames not found via standard paths: \n" ;
      }

    for(auto val : notFounds)
        Log::AppMessage() << " for " << val.first << ": "<<joinToString(val.second, ", ") << "\n" ;

    if(not notFounds.empty())
      {
        Log::AppMessage() << "this does not necessarily there is a problem\n";
        Log::AppMessage() << "the application can either have its own environment or the soname is resolved via a link name \n" ;
        Log::AppMessage() << "you can re-check the affected file with ldd \n" ;
      }

    Log::AppMessage() << std::endl;


}















a4sqlt3::Dataset // { "pkg", "file" , "soname" }
getWhoNeeds(const ElfFile& elf)
{
  if( elf.getType() != ElfFile::Library )
    return a4sqlt3::Dataset();

  std::string dirname = elf.getName().getDir();
  std::string fname = elf.getName().getBase();

  using namespace a4sqlt3;

  const std::string spname = "get_whoneed_file";
  SqlCommand* cmd = Cache::getInstance()->DB().getCommand(spname);
  if( cmd == nullptr )
    {
      std::string sql = "SELECT pkgname as pkg, dldir || '/' || dlname as file,  needed as soname "
          " FROM sbbdep_deps_sys_and_rpath"
          " WHERE ndir = ? AND nname = ?";

      cmd = Cache::getInstance()->DB().createStoredCommand(spname, sql);
    }

  cmd->Parameters().setValues({ dirname, fname });

  Dataset ds;
  Cache::getInstance()->DB().Execute(cmd, &ds);
  return ds ;
}
//--------------------------------------------------------------------------------------------------


a4sqlt3::Dataset  // {"dynlinked", "pkg", "file" , "soname" }
getWhoNeedsPkg(const std::string& name)
{

  using namespace a4sqlt3;

  const std::string spname = "get_whoneed_pkgname";
  SqlCommand* cmd = Cache::getInstance()->DB().getCommand(spname);
  if( cmd == nullptr )
    {
      std::string sql = "SELECT ndir || '/' || nname as dynlinked ,pkgname as pkg, dldir || '/' || dlname as file,  needed as soname"
          " FROM sbbdep_deps_sys_and_rpath"
          " WHERE sbbdep_deps_sys_and_rpath.npkg = ?";

      cmd = Cache::getInstance()->DB().createStoredCommand(spname, sql);
    }

  cmd->Parameters().setValues({ name });

  Dataset ds;
  Cache::getInstance()->DB().Execute(cmd, &ds);
  return ds ;
}
//--------------------------------------------------------------------------------------------------



void printWhoNeed( const Pkg& pkg, bool addversion, bool xdl )
{

  ReportTree reptree;
  if(pkg.getType() == PkgType::BinLib  )
    {
      // should be exact one 1 , but..
      for(auto elf: pkg.getDynLinked())
        {
          if(elf.getType() != ElfFile::Library)
            continue;

          ReportSet rs{ {"pkg", "file" , "soname"} };
          rs.merge( getWhoNeeds(elf) ) ;

          for(auto& row : rs)
            {
              reptree.add( {
                elf.getName().Str() + " (" + row.getField(2).getString() + ")" ,
                addversion ? row.getField(0).getString() : PkgName(row.getField(0).getString()).Name() ,
                row.getField(1).getString()
              } );
            }

        }

    }
  if( pkg.getType() == PkgType::Installed)
    {
      ReportSet rs{ {"dynlinked", "pkg", "file" , "soname"} };
      rs.merge( getWhoNeedsPkg( PkgName(pkg.getPath().getBase()).FullName() ) ) ;

            for(auto& row : rs)
              {
                reptree.add( {
                  addversion ? row.getField(1).getString() : PkgName(row.getField(1).getString()).Name() ,
                  row.getField(2).getString() ,
                  "needs "+row.getField(0).getString() + " (" + row.getField(3).getString() + ")"
                } );
              }
    }

  if(xdl)
    {

      std::function<void(ReportElement, int)> printChild = [&printChild]( ReportElement elem , int level ){
        for(auto node: elem.node){
           for(int i = 0; i < level; ++i)
             Log::AppMessage() << " " ;

           Log::AppMessage() << node.first << "\n";
          printChild(node.second, level+2);
        }
      };

      for( auto elem : reptree.node )
      {
        Log::AppMessage() << elem.first << std::endl;
        printChild(elem.second, 2) ;
      }
      Log::AppMessage() << std::endl;
      //printTree(reptree) ; // TODO, better format
    }
  else
    {
      StringSet pkgnames ;
      for(auto soinfos : reptree.node )
        {
          for (auto pgkinfos : soinfos.second.node )
            pkgnames.insert(pgkinfos.first) ;
        }
      Log::AppMessage()<< joinToString( pkgnames, addversion ? "\n": ", " ) << std::endl ;
    }


}




//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
}} // ns
