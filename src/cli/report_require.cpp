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
#include <sbbdep/cache.hpp>

#include <a4sqlt3/dataset.hpp>


#include <sbbdep/cache.hpp>
#include <sbbdep/elffile.hpp>
#include <sbbdep/lddmap.hpp>
#include <sbbdep/log.hpp>
#include <sbbdep/pkg.hpp>
#include <sbbdep/pkgname.hpp>


#include <a4sqlt3/sqlcommand.hpp>
#include <a4sqlt3/dataset.hpp>

#include <algorithm>
#include <string>
#include <vector>
#include <set>


namespace sbbdep {
namespace cli{



namespace
{

  using Dataset = a4sqlt3::Dataset ;

  using StringVec = std::vector<std::string> ;
  using StringSet = std::set<std::string> ;
  using StringMap = std::map<std::string,std::string> ;

  using NotFoundMap = std::map<std::string, StringSet>; // from file, so names

  // ds(pkgname, filename , soname) and NotFound
  using RequiredInfo = std::tuple<Dataset, NotFoundMap> ;


}

a4sqlt3::Dataset
getPkgsOfFile (Cache& cache,const PathName& fname, int arch)
{

  const char* sql = R"~(
  SELECT fullname 
  FROM pkgs INNER JOIN dynlinked ON pkgs.id = dynlinked.pkg_id
  WHERE dynlinked.filename=?  AND dynlinked.arch=? ; 
  )~";

  const std::string cmdname = "getPkgsOfFilebyFile" ;

  auto& cmd = cache.namedCommand(cmdname, sql) ;

  return cmd.run( { fname.Str (), arch }) ;

}
//------------------------------------------------------------------------------

/**
 * this is top down, get ldd info and pack it
 */
RequiredInfo
getRequiredInfosLDD(Cache& cache, const Pkg& pkg)
{

  using namespace a4sqlt3;

  // not found collection
  // filename, so symbols
  NotFoundMap notFoundMap;

  // if I check a DESTDIR,
  // the included libs  they will not be found in the system,
  // but they are not not found :-)
  StringSet soInDestDir;
  if(pkg.getType () == PkgType::DestDir)
    {
      for(auto& f : pkg.getElfFiles ())
        {
          if(f.soName () != "")
            {
              soInDestDir.insert (f.soName ());
            }
        }
    }

  auto isSoInDestDir = [&soInDestDir](const std::string& soname)
    {
      return soInDestDir.find (soname) == soInDestDir.end () ;
    };

  // for each elf file in dl I need to get the ld map
  //        (symbol -> file (possible link))
  // something to do for not founds ....
  // than the redundant stuff,
  // so -> need symbol -> file (list who needs this)
  // this will reduce the lookups...

  // soName , file , arch
  using SymFileLinkArch = std::tuple<std::string, std::string, int> ;

  // takes the sym, filelink, arch , and a list of files that need this ...
  std::map<SymFileLinkArch, StringSet> ldsym_byfile;
  // go trough all elf files
  for(const ElfFile& elf : pkg.getElfFiles())
    {
      StringSet notFound; // for delete an insert as problems
      // soname - filename
      StringMap ldd_map = getLddMap (elf.getName()) ;
      for(const auto& ld_line : ldd_map)
        { // is 'not found', but search for on "/"
          if( ld_line.second.find("/") == std::string::npos )
            {
              if(not isSoInDestDir (ld_line.first))
                { // bookmark for later delete from map
                  notFound.insert(ld_line.first);
                }
              // ich glaub ich brauch hier ein else mit einer  notiz
              // das required im eigengn pkg ist ...
            }
        }

      if(not notFound.empty())
        {
          const auto filename = elf.getName ().Str () ;
          notFoundMap.insert (NotFoundMap::value_type (filename, notFound));

          for(const auto& so : notFound)
            {
              ldd_map.erase (so);
            }
          // das scheint nicht zu stimmen,
          // wenn ein not found im selben pkg ist,
          // wird das anscheinend nicht behandelt
          // wie bekomm ich das in die ausgab

          // anscheinend muss ich dann einen
          //Dataset ds {{ "pkgname", "filename", "soname" , "requiredby" } };
          // schon anlegen und dann mergen
        }


      if(ldd_map.empty ())
        {
          continue;
        }

      for(const auto& so_needed : ldd_map)
        {
          const SymFileLinkArch  sym_flnk_arch =
              std::make_tuple( so_needed.first,
                               so_needed.second , elf.getArch () ) ;

          auto storepos = ldsym_byfile.find ( sym_flnk_arch );

          if( storepos == ldsym_byfile.end ())
            {
              ldsym_byfile [sym_flnk_arch] = { elf.getName ().Str () } ;
            }
          else
            {
              storepos->second.insert ( elf.getName ().Str () ) ;
            }
        }

    }


  //Dataset ds {{ "pkgname", "filename", "soname" , "requiredby" } };
  Dataset ds {{ DbValueType::Text,
                DbValueType::Text,
                DbValueType::Text ,
                DbValueType::Text } };

  for(const auto& ldsym_files : ldsym_byfile )
    {

      const SymFileLinkArch& sfa = ldsym_files.first ;
      const auto& sym = std::get<0> (sfa);
      const auto& flnk = std::get<1> (sfa);
      const auto arch = std::get<2> (sfa);

      Path path (flnk);
      while(path.isLink ())
        path.makeRealPath ();

      Dataset dspkgs = getPkgsOfFile (cache, path, arch);

      //need  "pkgname", "filename", "soname" , "requiredby"
      for(const auto& pkgval : dspkgs)
        {
          for(auto f : ldsym_files.second)
          {
            DbValues vals = {
              pkgval.at (0) ,
              DbValue (path.getURL()),
              DbValue (sym) ,
              DbValue (f)
            };

            ds.merge (vals);
          }
        }
    }

  return std::make_tuple(ds, notFoundMap);


}
//------------------------------------------------------------------------------

a4sqlt3::Dataset
elfdeps( Cache& cache
        , const PathName& fromfile
        , const StringVec& needed
        , int arch
        , const StringVec& rrnupaths)
{

  auto quote = [](const std::string& val) -> std::string{
    return  std::string ("'") + val + std::string ("'") ;
  };

  auto quoterpath = [&fromfile, &quote](const std::string val) -> std::string
    {
      std::string pathname = replaceORIGIN (val, fromfile.getDir ());
      if (pathname.empty ())
        {
          return "'no/where'";
        }
      Path p (pathname);
      p.makeRealPath () ;
      return  quote(p.getURL ()) ;
  };

  const std::string insonames = utils::joinToString(needed, ",", quote);
  const std::string archstr = std::to_string (arch);
  const std::string unionrunpath = rrnupaths.empty() ? std::string () :
      " UNION SELECT lddir FROM rrunpath WHERE lddir IN ("
      + utils::joinToString (rrnupaths , ",", quoterpath) + ") ";


  const std::string sql=
    "SELECT pkgs.fullname as pkgname,  dynlinked.filename , dynlinked.soname, "
    "'" + fromfile.Str () + "' as  requiredby"
    " FROM pkgs INNER JOIN dynlinked ON pkgs.id = dynlinked.pkg_id"
    " WHERE dynlinked.soname IN ( "
    + insonames +
    " ) AND dynlinked.arch = "
    + archstr +
    " AND  dynlinked.dirname IN  "
    " ( SELECT dirname FROM lddirs UNION SELECT dirname FROM ldlnkdirs "
    + unionrunpath +
    " );" ;

  return cache.select (sql) ;

}
//------------------------------------------------------------------------------

RequiredInfo
getRequiredInfos(Cache& cache, const Pkg& pkg)
{
  // no idea anymore how this works in detail,
  // I think I should write some docu

  using namespace a4sqlt3;

  // not found collection
  // filename, so symbols
  NotFoundMap notFoundMap;

  // if I check a DESTDIR,
  // the included libs  they might not be found in the system,
  // but they are not not found :-)
  // but it might also be in the system !!

  StringSet soInDestDir;
  if(pkg.getType () == PkgType::DestDir)
    {
      for(auto& f : pkg.getElfFiles ())
        {
          if(f.soName () != "")
            {
              soInDestDir.insert (f.soName ());
            }
        }
    }

  auto isSoInDestDir = [&soInDestDir](const std::string& soname)
    {
      return soInDestDir.find (soname) == soInDestDir.end () ;
    };

  // return data
  //Dataset ds {{ "pkgname", "filename", "soname" , "requiredby" } };
  Dataset ds {{ DbValueType::Text,
                DbValueType::Text,
                DbValueType::Text ,
                DbValueType::Text } };
  NotFoundMap not_found;

//  StringSet known_needed ;
  for (const ElfFile& elf : pkg.getElfFiles ())
    {
      StringVec needed = elf.getNeeded();
      if( needed.empty() )
        {
          continue;
        }

      Dataset deps = elfdeps (cache, elf.getName (),
                               needed,
                               elf.getArch (),
                               elf.getRRunPaths ());

      auto notfound_end = std::remove_if(std::begin(needed), std::end(needed),
          [&deps](const std::string& val) ->bool
            {
              for (auto& row : deps)
                { // TODO I should add some way to find things in Dataset
                  if (row.at (2).getText () == val)
                    {
                      return true;
                    }
                }
              return false;
            });

      StringSet notFounds;

      auto nfiter = needed.begin ();
      while (nfiter != notfound_end)
        {
          if (not isSoInDestDir (*nfiter))
            {
              notFounds.insert (*nfiter);
            }

          ++nfiter;
        }

      if(not notFounds.empty ())
        {
          not_found [elf.getName ().Str ()] = notFounds;
        }

      ds.merge(deps); // put this record to the report


    }


  return std::make_tuple(ds, not_found);

}

//------------------------------------------------------------------------------
void
printRequired(Cache& cache,
              const Pkg& pkg,
              bool addversion,
              bool xdl ,
              bool ldd)
{

  RequiredInfo requiredinfo =
       ldd ? getRequiredInfosLDD(cache, pkg) :   getRequiredInfos(cache, pkg) ;




  utils::ReportTree reptree;

  //pkgname,  filename , soname
  auto& rs = std::get<0>(requiredinfo) ;
  NotFoundMap& notFounds = std::get<1>(requiredinfo) ;


  for(auto& row : rs)
    {
      if(xdl)
        {
        reptree.add( {row.at(3).getText(),
          row.at(2).getText(),
          row.at(1).getText() ,
          row.at(0).getText() } );
        }
      else
        {
          reptree.add( {
            row.at(2).getText(),
            row.at(1).getText() ,
            row.at(0).getText() } );
        }
    }

  if( pkg.getType() == PkgType::BinLib )
    {
      a4sqlt3::Dataset ds = getPkgsOfFile(cache
                                          , pkg.getPath ()
                                          , pkg.getArch () ) ;
      WriteAppMsg() << "\n" ;
      WriteAppMsg() << "check " << pkg.getPath() ;
      WriteAppMsg() << ", " << pkg.getElfFiles().begin()->getArch() << "bit " ;
      if(pkg.getElfFiles().begin()->getType() == ElfFile::Binary)
        {
          WriteAppMsg() << "binary " ;
        }
      else if(pkg.getElfFiles().begin()->getType() == ElfFile::Library)
        {
          WriteAppMsg() << "library ("
              << pkg.getElfFiles().begin()->soName()
              << ")" ;
        }

      WriteAppMsg() << std::endl ;

      if(ds.size()==0)
        {
          WriteAppMsg() << " .. not in a known package" << std::endl;
        }
      else
        {
          for(auto flds : ds)
            WriteAppMsg() << " .. from package "
                          <<  flds.at(0).getText() <<std::endl;

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
          WriteAppMsg() << "file " << requiredby_sos.first << " needs:";
          for(auto so_files : requiredby_sos.second.node)
            {
              WriteAppMsg() << "  " << so_files.first << " found in:";
              for(auto file_pkgs : so_files.second.node)
                {
                  WriteAppMsg() << "    " << file_pkgs.first << "( "
                      << utils::joinToString
                          (getKeySet (file_pkgs.second.node), " | ", makename)
                      << " )";
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
              const auto& pkgnames =  file_pkgs.second.node ;
              if(not ignore_name.empty() &&
                  pkgnames.find(ignore_name) != pkgnames.end() )
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



    if(not notFounds.empty())
      {
        WriteAppMsg() << std::endl;
        WriteAppMsg() << "sonames not found via " <<
            (ldd ? "ldd\n" : "standard paths: ") ;

        for(auto val : notFounds){
            WriteAppMsg() << " for " << val.first
                << ": "<<utils::joinToString(val.second, ", ")  ;
        }

        WriteAppMsg() << "this does not necessarily mean there is a problem ";
        if(not ldd)
          {
            WriteAppMsg() <<
                "the application can either have its own environment "
                " or the soname is resolved via a link name" ;
            WriteAppMsg() <<
                "you can re-check the affected files with --ldd" ;
          }
        else
          {
            WriteAppMsg() << "but it's very likely that there is one";
          }

      }




}
//------------------------------------------------------------------------------

}} // ns
