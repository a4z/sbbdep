/*
--------------Copyright (c) 2010-2014 H a r a l d  A c h i t z---------------
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
#include "report_utils.hpp"

#include <sbbdep/log.hpp>
#include <sbbdep/pkgname.hpp>
#include <sbbdep/pkg.hpp>
#include <sbbdep/cache.hpp>
#include <sbbdep/elffile.hpp>
#include <sbbdep/cachesql.hpp>
#include <sbbdep/lddmap.hpp>

#include <a4sqlt3/sqlcommand.hpp>
#include <a4sqlt3/dataset.hpp>


#include <set>
#include <algorithm>
#include <tuple>
#include <iterator>


namespace sbbdep {
namespace cli{

//--------------------------------------------------------------------------------------------------

namespace
{
  using NotFoundMap = std::map<std::string, utils::StringSet>; // from file, so names
}



a4sqlt3::Dataset
getPkgsOfFile(const PathName& fname, int arch)
{
  using namespace a4sqlt3;
  const std::string cmdname = "getPkgsOfFilebyFile" ;
  SqlCommand* cmd = Cache::get().DB().getCommand(cmdname);
  if( cmd == nullptr )
    {
      std::string sql = R"~(
    SELECT fullname FROM pkgs INNER JOIN dynlinked ON pkgs.id = dynlinked.pkg_id
     WHERE dynlinked.filename=?  AND dynlinked.arch=? ; 
    )~";
      cmd = Cache::get().DB().createStoredCommand(cmdname, sql);
    }

  cmd->Parameters().setValues( { fname.Str(), arch }) ;

  Dataset ds;
  Cache::get().DB().Execute(cmd, ds);
  return ds;
}


//--------------------------------------------------------------------------------------------------

utils::ReportSet elfdeps(const PathName& fromfile, const ElfFile::StringVec& needed,
    int arch, const ElfFile::StringVec& rrnupaths)
{

  auto quote = [](const std::string val) -> std::string{
    return  std::string("'") + val + std::string("'") ;
  };



  auto quoterpath = [&fromfile, &quote](const std::string val) -> std::string{
    std::string pathname = CacheSQL::replaceORIGIN(val, fromfile.getDir());
    if(pathname.empty()) return "'no/where'";
    Path p(pathname);
    p.makeRealPath() ;
    return  quote(p.getURL()) ;
  };
  // TODO , check len  needed or sql , there is are SQLite limits

  std::string insonames = utils::joinToString(needed, ",", quote);
  std::string archstr = std::to_string( arch );
  std::string unionrunpath;

  if(not rrnupaths.empty())
    {
      std::string inrunpath = utils::joinToString(rrnupaths , ",", quoterpath);
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


  utils::ReportSet ds{{}} ;
  Cache::get().DB().Execute(sql, ds) ;
  return ds;



}
//--------------------------------------------------------------------------------------------------

std::tuple<utils::ReportSet, NotFoundMap> // ds(pkgname, filename , soname) and NotFound
getRequiredInfosLDD(const Pkg& pkg)
{
  using namespace a4sqlt3;

  NotFoundMap not_found;

  utils::StringSet notFoundIgnore;
  if(pkg.getType() == PkgType::DestDir)
    {
        for(auto& f : pkg.getElfFiles())
          {
            if(f.soName()!="")
              notFoundIgnore.insert(f.soName());
          }
    }


  utils::ReportSet rs {{ "pkgname", "filename", "soname" , "requiredby" } };


  // for each elf file in dl I need to get the ld map (symbol -> file (possible link))
  // something to do for not founds ....
  // than the redundant stuff,
  // so -> need symbol -> file (list who needs this)
  // this will reduce the lookups...


  using SymFileLinkArch = std::tuple<std::string, std::string, int> ;

  SymFileLinkArch  sym_flnk_arch;

  // takes the sym, filelink, arch , and a list of files that need this ...
  std::map<SymFileLinkArch, utils::StringSet> ldsym_byfile;

  for(const ElfFile& elf : pkg.getElfFiles())
    {
      utils::StringMap ldmap = getLddMap(elf.getName()) ;

      utils::StringSet rem_so; // for delete an insert as problems
      for(auto so_needed : ldmap)
        { // is 'not found', but search for on "/"
          if( so_needed.second.find("/") == std::string::npos )
            {
              if( notFoundIgnore.find(so_needed.first) == notFoundIgnore.end() )
                rem_so.insert(so_needed.first); // bookmark for later delete from map
            }
        }

      if(not rem_so.empty())
        not_found.insert(NotFoundMap::value_type(elf.getName().Str(), rem_so));

      for(auto so : rem_so)
        ldmap.erase(so);

      if(ldmap.empty())
        continue;

      for(auto so_needed : ldmap)
        {
          sym_flnk_arch = std::make_tuple( so_needed.first, so_needed.second , elf.getArch() ) ;

          auto storepos = ldsym_byfile.find( sym_flnk_arch );

          if( storepos == ldsym_byfile.end())
            ldsym_byfile[sym_flnk_arch] = { elf.getName().Str() } ;
          else
            storepos->second.insert( elf.getName().Str() ) ;

        }

    }


  for(auto ldsym_files : ldsym_byfile )
    {

      const SymFileLinkArch & sfa = ldsym_files.first ;
      auto& sym = std::get<0>(sfa);
      auto& flnk = std::get<1>(sfa);
      const auto arch = std::get<2>(sfa);

      Path path(flnk);
      while( path.isLink() )
        path.makeRealPath();

      a4sqlt3::Dataset dspkgs = getPkgsOfFile( path, arch );

      //need  "pkgname", "filename", "soname" , "requiredby"
      for(auto pkgval : dspkgs)
        {
          for(auto f : ldsym_files.second)
          {

            DbValues vals = {
            pkgval.getField(0) ,
            DbValue( path.getURL()), DbValue(sym) ,
            DbValue(f)
            };

          rs.addFields(vals);
            }
        }


    }

  return std::make_tuple(rs, not_found);

}




std::tuple<utils::ReportSet, NotFoundMap> // ds(pkgname, filename , soname) and NotFound
getRequiredInfos(const Pkg& pkg)
{

  using namespace a4sqlt3;

  utils::StringSet known_needed;

  NotFoundMap not_found;

  utils::StringSet notFoundIgnore;
  if(pkg.getType() == PkgType::DestDir)
    {
        for(auto& f : pkg.getElfFiles())
          {
            if(f.soName()!="")
              notFoundIgnore.insert(f.soName());
          }
    }




  utils::ReportSet rs {{ "pkgname", "filename", "soname" , "requiredby" } };

  for(const ElfFile& elf : pkg.getElfFiles())
    {

      utils::StringVec needed = elf.getNeeded();

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

      // if ldd option, than here the branch

      utils::ReportSet ds = elfdeps(elf.getName(), needed, elf.getArch(), elf.getRRunPaths());
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
      utils::StringSet notFounds;

      auto nfiter = needed.begin();
      while(nfiter != notfound_end)
        {
          if(notFoundIgnore.find(*nfiter) == notFoundIgnore.end())
            notFounds.insert(*nfiter);
          ++nfiter;
        }



      if(not notFounds.empty())
          not_found.insert(NotFoundMap::value_type(elf.getName().Str(), notFounds));


      rs.merge(ds); // put this record to the report

    }

  return std::make_tuple(rs, not_found);

}
//--------------------------------------------------------------------------------------------------



void printRequired( const Pkg& pkg, bool addversion, bool xdl , bool ldd)
{

  std::tuple<utils::ReportSet, NotFoundMap> requiredinfo =
      ldd ? getRequiredInfosLDD(pkg) :   getRequiredInfos(pkg) ;

  utils::ReportTree reptree;

  //pkgname,  filename , soname
  utils::ReportSet& rs = std::get<0>(requiredinfo) ;
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
      a4sqlt3::Dataset ds = getPkgsOfFile( pkg.getPath(), pkg.getArch() ) ;
      WriteAppMsg() << "\n" ;
      WriteAppMsg() << "check " << pkg.getPath() ;
      WriteAppMsg() << ", " << pkg.getElfFiles().begin()->getArch() << "bit " ;
      if(pkg.getElfFiles().begin()->getType() == ElfFile::Binary)
        WriteAppMsg() << "binary " ;
      else if(pkg.getElfFiles().begin()->getType() == ElfFile::Library)
        WriteAppMsg() << "library (" << pkg.getElfFiles().begin()->soName()  << ")" ;

      WriteAppMsg() << std::endl ;

      if(ds.getRowCount()==0)
        {
          WriteAppMsg() << " .. not in a known package" << std::endl;
        }
      else
        {
          for(auto flds : ds)
            WriteAppMsg() << " .. from package " <<  flds.getField(0).getString() <<std::endl;

        }
      WriteAppMsg() << std::endl ;
    }

  auto makename = [addversion, xdl](const std::string val)
    {
      // in xdl mode, not using the slapt-get style of 'name >= version'
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
      // TODO , think about give up this format and use the normal long version for all
      // or change it to = , see http://software.jaos.org/git/slapt-get/plain/FAQ.html#slgFAQ19

    };

  if( xdl )
    {
      for(auto requiredby_sos : reptree.node)
        {
          WriteAppMsg() << "file " << requiredby_sos.first << " needs:\n";
          for(auto so_files : requiredby_sos.second.node)
            {
              WriteAppMsg() << "  " << so_files.first << " found in:\n";
              for(auto file_pkgs : so_files.second.node)
                {
                  WriteAppMsg() << "    " << file_pkgs.first << "( "
                      << utils::joinToString(getKeySet(file_pkgs.second.node), " | ", makename)
                      << " )\n";
                }
            }
        }
    }
  else
    {
      utils::StringSet pkglist;
      std::string ignore_name ;
      if(pkg.getType() == PkgType::Installed)
        ignore_name = pkg.getPath().getBase();


      for(auto so_files: reptree.node) // filename so, just what the pkgs...
        {
          utils::StringSet pkgsofso;
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
            pkglist.insert( utils::joinToString(pkgsofso, " | ", makename ) ) ;
        }

      WriteAppMsg() << utils::joinToString(pkglist, addversion ? "\n" : ", " ) ;

    }
    WriteAppMsg() << std::endl;



    if(not notFounds.empty())
      {
        WriteAppMsg() <<std::endl;
        WriteAppMsg() << "sonames not found via " <<
            (ldd ? "ldd\n" : "standard paths: \n") ;

        for(auto val : notFounds){
            WriteAppMsg() << " for " << val.first << ": "<<utils::joinToString(val.second, ", ")
            << "\n" ;
        }

        WriteAppMsg() << "this does not necessarily mean there is a problem\n";
        if(not ldd)
          {
            WriteAppMsg() << "the application can either have its own environment or the soname is resolved via a link name \n" ;
            WriteAppMsg() << "you can re-check the affected files with --ldd \n" ;
          }
        else
          {
            WriteAppMsg() << "but it's very likely that there is one\n";
          }

        WriteAppMsg() << std::endl;
      }

    WriteAppMsg() << std::endl;
}





//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
// from here whoneeds stuff, possible add this to own file
//--------------------------------------------------------------------------------------------------


std::string getWhoNeedFileQuery()
{
  return
  R"~(
  select pkgs.fullname as pkg, dynlinked.filename as filename , required.needed as soname , d2.filename as fromfile
   from  pkgs 
   inner join dynlinked on pkgs.id = dynlinked.pkg_id
   inner join required on dynlinked.id = required.dynlinked_id
  left join rrunpath on dynlinked.id = rrunpath.dynlinked_id 
  left join dynlinked d2 on required.needed = d2.soname 
   where  d2.arch = dynlinked.arch 
  AND 
  ( 
    ( rrunpath.lddir IS NOT NULL AND d2.dirname not in (  select distinct * from lddirs union select distinct * from ldlnkdirs )  and rrunpath.lddir = d2.dirname )
   OR 
    (
    d2.dirname in (  select distinct * from lddirs union select distinct * from ldlnkdirs ) 
    AND ( rrunpath.lddir IS  NULL OR rrunpath.lddir in (  select distinct * from lddirs union select distinct * from ldlnkdirs ) )
    )
  )
  AND d2.filename = ? 
  ;
  )~";
}

std::string
getWhoNeedPkgQuery()
{

  return R"~(
select pkgs.fullname as pkg, dynlinked.filename as filename , required.needed as soname , d2.filename as fromfile
 from  pkgs 
 inner join dynlinked on pkgs.id = dynlinked.pkg_id
 inner join required on dynlinked.id = required.dynlinked_id
left join rrunpath on dynlinked.id = rrunpath.dynlinked_id 
left join dynlinked d2 on required.needed = d2.soname 
inner join pkgs p2 on p2.id = d2.pkg_id
 where  d2.arch = dynlinked.arch 
AND 
( 
  ( rrunpath.lddir IS NOT NULL AND d2.dirname not in (  select distinct * from lddirs union select distinct * from ldlnkdirs )  and rrunpath.lddir = d2.dirname )
 OR 
  (
  d2.dirname in (  select distinct * from lddirs union select distinct * from ldlnkdirs ) 
  AND ( rrunpath.lddir IS  NULL OR rrunpath.lddir in (  select distinct * from lddirs union select distinct * from ldlnkdirs ) )
  )
)
AND p2.fullname  = ? 
;
)~";
}
//--------------------------------------------------------------------------------------------------




a4sqlt3::Dataset // { "pkg", "filename" , "soname", "fromfile" }
getWhoNeeds(const ElfFile& elf)
{
  using namespace a4sqlt3;

  Dataset ds;

  if( elf.getType() != ElfFile::Library )
    return ds;

  const std::string spname = "get_whoneed_file";
  SqlCommand* cmd = Cache::get().DB().getCommand(spname);
  if( cmd == nullptr )
    {
      cmd = Cache::get().DB().createStoredCommand(spname, getWhoNeedFileQuery());
    }

  cmd->Parameters().setValues({ elf.getName().Str() });

  Cache::get().DB().Execute(cmd, ds);
  return ds ;
}
//--------------------------------------------------------------------------------------------------


a4sqlt3::Dataset  // { "pkg", "filename" , "soname" , "fromfile" }
getWhoNeedsPkg(const std::string& name)
{

  using namespace a4sqlt3;

  const std::string spname = "get_whoneed_pkg";
  SqlCommand* cmd = Cache::get().DB().getCommand(spname);
  if( cmd == nullptr )
    { //select
      cmd = Cache::get().DB().createStoredCommand(spname, getWhoNeedPkgQuery());
    }

  cmd->Parameters().setValues({ name });

  Dataset ds;
  Cache::get().DB().Execute(cmd, ds);
  return ds ;

}
//--------------------------------------------------------------------------------------------------



void printWhoNeed( const Pkg& pkg, bool addversion, bool xdl )
{

  utils::ReportSet rs {{ "pkg", "filename" , "soname" , "fromfile"  } };


  if(pkg.getType() == PkgType::BinLib  )
    {
      if(pkg.getElfFiles().size()> 0) // should always be exact 1
        rs.merge ( getWhoNeeds( pkg.getElfFiles()[0]) ) ;
    }
  else if( pkg.getType() == PkgType::Installed)
    {
      rs.merge ( getWhoNeedsPkg(pkg.getPath().getBase()) ) ;
    }
  else
    return ;

  if(xdl)
    {
      utils::ReportTree reptree;
      for(auto& row : rs )
        {
          reptree.add( { row.getField(3).getString() + " (" + row.getField(2).getString() +")",
            row.getField(0).getString() ,
            row.getField(1).getString() } ) ;

        }

      std::function<void(utils::ReportElement, int)> printChild =
          [&printChild]( utils::ReportElement elem , int level ){
        for(auto node: elem.node){
           for(int i = 0; i < level; ++i)
             WriteAppMsg() << " " ;

           WriteAppMsg() << node.first << "\n";
          printChild(node.second, level+2);
        }
      };

      for( auto elem : reptree.node )
      {
        WriteAppMsg() << elem.first << " is used from:"<<std::endl;
        printChild(elem.second, 2) ;
      }
      WriteAppMsg() << std::endl;
      //printTree(reptree) ; // TODO, better format
    }
  else
    {
      utils::StringSet pkgnames ;
      for(auto& row : rs )
        {
          pkgnames.insert(addversion ?
              row.getField(0).getString() : PkgName(row.getField(0).getString()).Name() ) ;
        }

      WriteAppMsg()<< utils::joinToString( pkgnames, addversion ? "\n": ", " ) << std::endl ;
    }


}




//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
}} // ns
