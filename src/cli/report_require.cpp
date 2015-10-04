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


#include "report.hpp"

#include <sbbdep/cache.hpp>
#include <sbbdep/elffile.hpp>
#include <sbbdep/ldconf.hpp>
#include <sbbdep/log.hpp>
#include <sbbdep/pkg.hpp>
#include <sbbdep/pkgname.hpp>


#include <sq3/command.hpp>
#include <sq3/dataset.hpp>
#include <sq3/dbvalues.hpp>
#include <sq3/types.hpp>

#include <algorithm>
#include <string>
#include <vector>
#include <set>


namespace sbbdep {
namespace cli{



namespace
{

  using Dataset = sq3::Dataset ;

  using StringVec = std::vector<std::string> ;
  using StringSet = std::set<std::string> ;
  using StringMap = std::map<std::string,std::string> ;

  using NotFoundMap = std::map<std::string, StringSet>; // from file, so names

  // ds(pkgname, filename , soname) and NotFound
  using RequiredInfo = std::tuple<Dataset, NotFoundMap> ;


}



/**
 * this is top down, get ldd info and pack it
 */
RequiredInfo
getRequiredInfosLDD(Cache& cache, const Pkg& pkg)
{

  using namespace sq3;

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
      return soInDestDir.find (soname) != soInDestDir.end () ;
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
      StringMap ldd_map = Ldconf::lddMap (elf.getName()) ;
      for(const auto& ld_line : ldd_map)
        { // is 'not found', but search for on "/"
          if( ld_line.second.find("/") == std::string::npos )
            {
              if(not isSoInDestDir (ld_line.first))
                { // bookmark for later delete from map
                  notFound.insert(ld_line.first);
                }
            }
        }

      if (not notFound.empty ())
        {
          const auto filename = elf.getName ().str () ;
          notFoundMap.insert (NotFoundMap::value_type (filename, notFound));
          for(const auto& so : notFound)
            {
              ldd_map.erase (so);
            }
        }


      if (ldd_map.empty ())
        {
          continue;
        }

      for (const auto& so_needed : ldd_map)
        {
          const SymFileLinkArch  sym_flnk_arch =
              std::make_tuple (so_needed.first,
                               so_needed.second , elf.getArch ()) ;

          auto storepos = ldsym_byfile.find (sym_flnk_arch);

          if (storepos == ldsym_byfile.end ())
            {
              ldsym_byfile [sym_flnk_arch] = {elf.getName ().str ()};
            }
          else
            {
              storepos->second.insert ( elf.getName ().str () );
            }
        }

    }


  //Dataset ds {{ "pkgname", "filename", "soname" , "requiredby" } };
  Dataset ds {{ Type::Text,
                Type::Text,
                Type::Text ,
                Type::Text } };

  for (const auto& ldsym_files : ldsym_byfile )
    {

      const SymFileLinkArch& sfa = ldsym_files.first ;
      const auto& sym = std::get<0> (sfa);
      const auto& flnk = std::get<1> (sfa);
      const auto arch = std::get<2> (sfa);

      Path path (flnk);
      while(path.isLink ())
        path.makeRealPath ();

      Dataset dspkgs = utils::getPkgsOfFile (cache, path, arch);

      //need  "pkgname", "filename", "soname" , "requiredby"
      for(const auto& pkgval : dspkgs)
        {
          for(auto f : ldsym_files.second)
          {
            DbValues vals = {
              pkgval.at (0).getText (), /// is a variant so I need the type
              DbValue (path.str ()),
              DbValue (sym) ,
              DbValue (f)
            };

            ds.merge (vals);
          }
        }
    }

  return std::make_tuple (ds, notFoundMap);


}
//------------------------------------------------------------------------------

sq3::Dataset
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
      std::string pathname = replaceORIGIN (val, fromfile.dir ());
      if (pathname.empty ())
        {
          return "'no/where'";
        }
      Path p (pathname);
      p.makeRealPath () ;
      return  quote(p.str ()) ;
  };

  const std::string insonames = utils::joinToString(needed, ",", quote);
  const std::string archstr = std::to_string (arch);
  const std::string unionrunpath = rrnupaths.empty() ? std::string () :
      " UNION SELECT lddir FROM rrunpath WHERE lddir IN ("
      + utils::joinToString (rrnupaths , ",", quoterpath) + ") ";


  const std::string sql=
    "SELECT pkgs.fullname as pkgname,  dynlinked.filename , dynlinked.soname, "
    "'" + fromfile.str () + "' as  requiredby"
    " FROM pkgs INNER JOIN dynlinked ON pkgs.id = dynlinked.pkg_id"
    " WHERE dynlinked.soname IN ( "
    + insonames +
    " ) AND dynlinked.arch = "
    + archstr +
    " AND  dynlinked.dirname IN  "
    " ( SELECT dirname FROM lddirs UNION SELECT dirname FROM ldlnkdirs "
    + unionrunpath +
    " );" ;

  using namespace sq3;
  Types tps { {Type::Text,
                      Type::Text,
                      Type::Text,
                      Type::Text} } ;


  return  cache.select (sql, tps) ;

}
//------------------------------------------------------------------------------

RequiredInfo
getRequiredInfos(Cache& cache, const Pkg& pkg)
{
  // no idea anymore how this works in detail,
  // I think I should write some docu

  using namespace sq3;

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
      return soInDestDir.find (soname) != soInDestDir.end () ;
    };

  // return data
  //Dataset ds {{ "pkgname", "filename", "soname" , "requiredby" } };
  Dataset ds {{ Type::Text,
                Type::Text,
                Type::Text ,
                Type::Text } };
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
              auto iter =
                  std::find_if (deps.begin (), deps.end () ,
                           [&val] (const DbValues& row)
                             {
                                return row.at (2).getText () == val ;
                             }) ;

              return iter != deps.end();
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
          not_found [elf.getName ().str ()] = notFounds;
        }

      ds.merge(deps); // put this record to the report


    }


  return std::make_tuple(ds, not_found);

}

//------------------------------------------------------------------------------
void
printRequired(Cache& cache,
              const Pkg& pkg,
              bool shortNames,
              bool xdl ,
              bool ldd)
{

  RequiredInfo requiredinfo = ldd ?
       getRequiredInfosLDD (cache, pkg) : getRequiredInfos (cache, pkg);

  utils::ReportTree reptree;

  //pkgname,  filename , soname
  const auto& rs = std::get<0> (requiredinfo);
  NotFoundMap& notFounds = std::get<1> (requiredinfo);

  for (const auto& row : rs)
    {
      if (xdl)
        {
          reptree.add (
              { row.at (3).getText (), row.at (2).getText (),
                  row.at (1).getText (), row.at (0).getText () });
        }
      else
        {
          reptree.add (
              { row.at (2).getText (), row.at (1).getText (),
                  row.at (0).getText () });
        }
    }

  if (pkg.getType () == PkgType::BinLib)
    {
      const sq3::Dataset ds = utils::getPkgsOfFile (cache,
                                                 pkg.getPath (),
                                                 pkg.getArch ());
      auto msgChannel = LogMsg ();

      msgChannel << "check " << pkg.getPath () << ", "
                 << pkg.getElfFiles ().begin ()->getArch () << "bit ";

      if (pkg.getElfFiles ().begin ()->getType () == ElfFile::Binary)
        {
          msgChannel << "binary ";
        }
      else if (pkg.getElfFiles ().begin ()->getType () == ElfFile::Library)
        {
          msgChannel << "library (" << pkg.getElfFiles ().begin ()->soName ()
              << ")";
        }

      msgChannel << std::endl;

      if (ds.size () == 0)
        {
          msgChannel << " .. not in a known package" << std::endl;
        }
      else
        {
          for (const auto& flds : ds)
            {
              msgChannel << " .. from package " << flds.at (0).getText ()
                  << std::endl;
            }
        }
    }

  auto makename = [shortNames, xdl](const std::string val) -> const std::string
    {
      // in xdl mode, not using the slapt-get style of 'name >= version'
      if(xdl)
        {
          return shortNames ?
              PkgName (val).Name () : val;
        }

      PkgName pknam (val);
      std::string retval = pknam.Name ();
      if (not shortNames)
        {
          retval+= " = " + pknam.FullName ().substr (pknam.Name ().size () + 1,
                                                     std::string::npos);
          // see
          // http://software.jaos.org/git/slapt-get/plain/FAQ.html#slgFAQ19
        }
      return retval;

    };

  if (xdl)
    {
      for (auto requiredby_sos : reptree.node)
        {
          LogMsg () << "file " << requiredby_sos.first << " needs:";
          for (auto so_files : requiredby_sos.second.node)
            {
              LogMsg () << "  " << so_files.first << " found in:";
              for (auto file_pkgs : so_files.second.node)
                {
                  const auto names = getKeySet (file_pkgs.second.node) ;
                  LogMsg () << "    "
                      << file_pkgs.first
                      << " ("
                      << utils::joinToString (names, " | ", makename)
                      << ")";
                }
            }
        }
    }
  else
    {
      utils::StringSet pkglist;
      const auto ignore_name =
          pkg.getType () == PkgType::Installed ?
                    pkg.getPath ().base () : std::string();

      for (const auto& so_files : reptree.node) // filename so
        {
          utils::StringSet pkgsofso;
          for (const auto& file_pkgs : so_files.second.node)
            {
              const auto& pkgnames = file_pkgs.second.node;
              if (not ignore_name.empty ()
                  && pkgnames.find (ignore_name) != pkgnames.end ())
                { // avoid own package in the list
                  pkgsofso.clear ();
                  break;
                }
              for (const auto& so_pkg : file_pkgs.second.node)
                {
                  pkgsofso.insert (so_pkg.first);
                }
            }

          if (not pkgsofso.empty ())
            {
              pkglist.insert (utils::joinToString (pkgsofso, " | ", makename));
            }
        }

      LogMsg () << utils::joinToString (pkglist, shortNames ? ", " : "\n");

    }

  if (not notFounds.empty ())
    {
      LogMsg () << std::endl;
      LogMsg () << "sonames not found via "
          << (ldd ? "ldd\n" : "standard paths: ");

      for (auto val : notFounds)
        {
          LogMsg () << " for " << val.first << ": "
              << utils::joinToString (val.second, ", ");
        }

      LogMsg () << "this does not necessarily mean there is a problem ";
      if (not ldd)
        {
          LogMsg ()
              << "the application can either have its own environment "
                  " or the soname is resolved via a link name";
          LogMsg () << "you can re-check the affected files with --ldd";
        }
      else
        {
          LogMsg () << "but it's very likely that there is one";
        }

    }

}
//------------------------------------------------------------------------------

}} // ns
