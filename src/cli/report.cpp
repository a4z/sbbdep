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

namespace sbbdep {
namespace cli{



class ReportSet : public a4sqlt3::Dataset
{
public:
  ReportSet(const std::vector<std::string> fieldnames)
  {
    for(std::size_t i = 0; i < fieldnames.size(); ++i)
        m_namemap.insert( NameMap::value_type(fieldnames[i], i) ) ;
  }

  void merge( a4sqlt3::Dataset& other )
  {
    a4sqlt3::Dataset::merge(other) ;
  }

} ;




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
      and not upgraded.empty()
      and  not installed.empty()
      and not reinstalled.empty())
    LogInfo()    << "\n---synchronisation summary:\n" ;


  for(auto& val : deleted) LogInfo()    << "deleted: " << val << std::endl;
  for(auto& val : upgraded) LogInfo()   << "upgraded: " << val  << std::endl;
  for(auto& val : installed) LogInfo()  << "installed: " << val  << std::endl;
  for(auto& val : reinstalled) LogInfo()<< "reinstalled: " << val  << std::endl;

  LogInfo() << "\n";
}

//--------------------------------------------------------------------------------------------------

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


a4sqlt3::Dataset elfdeps(const ElfFile::StringVec& needed,
    int arch, const ElfFile::StringVec& rrnupaths)
{

  auto quote = [](const std::string val) -> std::string{
    return  std::string("'") + val + std::string("'") ;
  };

  std::string insonames = joinToString(needed, ",", quote);
  std::string archstr = std::to_string( arch );
  std::string unionrunpath;

  if(not rrnupaths.empty())
    {
      std::string inrunpath = joinToString(rrnupaths , ",", quote);
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
  std::string sql= "SELECT pkgs.fullname as pkgname,  dynlinked.filename , dynlinked.soname"
  " FROM pkgs INNER JOIN dynlinked ON pkgs.id = dynlinked.pkg_id"
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


void printRequiredPkgs2( const Pkg& pkg, bool addversion )
{

  using namespace a4sqlt3;
  using StringVec = std::vector<std::string>;
  using StringSet = std::set<std::string>;

  StringSet known_needed;

  StringSet notFounds;

  ReportSet rs{ {"pkgname", "filename", "soname"} };

  for(const ElfFile& elf : pkg.getDynLinked())
    {

      StringVec needed = elf.getNeeded();

      auto knownbegin = std::remove_if( std::begin(needed), std::end(needed),
        [&known_needed](const std::string& val) ->bool {
          return known_needed.find(val) != known_needed.end() ;
        } );

      if(needed.empty())
        continue;
      else
        known_needed.insert(knownbegin, needed.end()) ;

      Dataset ds  = elfdeps( needed , elf.getArch() , elf.getRRunPaths() );
// pkgs.fullname as pkgname,  dynlinked.filename , dynlinked.soname

      auto notfound_end  = std::remove_if( std::begin(needed), std::end(needed),
        [&notFounds, &ds](const std::string& val) ->bool {
          for(auto& row : ds)
            {
              if(row.getField(2).getString() == val )
                return true;

            }
          return false;
        } );


      notFounds.insert(needed.begin(), notfound_end) ;
       rs.merge(ds);

    }

// from here just report stuff ..

    using SummaryMap = std::map<std::string, StringSet>;
    SummaryMap summary;

    StringSet soinpkg ;
    for(const ElfFile& elf : pkg.getDynLinked())
      {
        if(not elf.soName().empty())
          soinpkg.insert(elf.soName());
      }

    // fullname as pkgname, filename , soname
    for( auto& fr : rs )
      {
        std::string soname = fr.getField(2).getString() ;

        if(soinpkg.find(soname)!= soinpkg.end())
          continue;

        auto finder = summary.find( soname );
        if( finder == summary.end() )
          {
            auto insert = summary.insert( SummaryMap::value_type(soname, StringSet()) );
            finder = insert.first;
          }
        std::string pkgname = fr.getField(0).getString() ;

	finder->second.insert(pkgname) ;

       }


    StringSet deps;

    auto makename = [addversion](const std::string val){
      PkgName pknam(val) ;
      std::string retval =  pknam.Name() ;
      if (addversion) retval+= " >= " + pknam.Version()  ;
      return retval;
    };


    for(auto pair : summary )
      {
        std::string joinednames = joinToString( pair.second , " | " ,  makename ) ;
        deps.insert(joinednames) ;
      }


    Log::AppMessage() << joinToString( deps , (addversion ? "\n" : ", ") ) ;
    Log::AppMessage() << std::endl  ;

    for ( auto& val : notFounds )
      {
        Log::AppMessage() << "!!!not found: " << val << "\n" ;
      }

}


void printRequiredPkgs( const Pkg& pkg, bool addversion )
{

  using LibInfoType = std::pair<std::string, int>;

  struct LibInfoCompair {
    bool operator() (const LibInfoType& lhs, const LibInfoType& rhs) const
      {
        int cmp = lhs.first.compare( rhs.first ) ;
        if (cmp < 0 ) return true;
        else if ( cmp > 0 ) return false;
        else return lhs.second < rhs.second ;
      }
  };
  using LibInfoSet = std::set<LibInfoType, LibInfoCompair> ;


  LibInfoSet requiresInfo , providesInfo ;

  for(const ElfFile& elf : pkg.getDynLinked())
    {
      for(const std::string& needed : elf.getNeeded())
          requiresInfo.insert(std::make_pair(needed,elf.getArch())) ;

      if ( elf.soName().size() )
        providesInfo.insert(std::make_pair(elf.soName(),elf.getArch())) ;
    }


  // remove libs that are within this pkg
  for (auto pos = providesInfo.begin(); pos != providesInfo.end(); ++pos)
      requiresInfo.erase(*pos) ;


  using StringSet = std::set<std::string> ;

  StringSet deps;
  StringSet notfound;

  using namespace a4sqlt3;

  SqlCommand* searcher = Cache::getInstance()->DB().getCommand("SearchPgkOfSoNameSQL");
  if(not searcher)
    searcher = Cache::getInstance()->DB().createStoredCommand("SearchPgkOfSoNameSQL",
        CacheSQL::SearchPgkOfSoNameSQL(), { DbValueType::Text,  DbValueType::Int});


  for( LibInfoSet::iterator pos = requiresInfo.begin(); pos!= requiresInfo.end(); ++pos )
    {
      searcher->Parameters().setValues( {DbValue(pos->first), DbValue(pos->second)}) ;
      Dataset ds;
      Cache::getInstance()->DB().Execute(searcher, &ds) ;

      if( ds.getRowCount()== 0 ) notfound.insert(pos->first);
      else
        {
          StringSet pkgsrequired;
          for(auto& flds : ds)
              pkgsrequired.insert(flds.getField(0).getString() );

          auto makename = [addversion](const std::string val){
            PkgName pknam(val) ;
            std::string retval =  pknam.Name() ;
            if (addversion) retval+= " >= " + pknam.Version()  ;
            return retval;
          };

          std::string joinednames = joinToString( pkgsrequired , " | " ,  makename ) ;
          deps.insert(joinednames) ;

        }
    }


  LogInfo() << joinToString( deps , (addversion ? "\n" : ", ") ) ;
  LogInfo() << std::endl  ;

  for ( auto& val : notfound )
    {
      LogInfo() << pkg.getPath() <<" ! not found: " << val << "\n" ;
    }

  LogInfo() << "-----------------------------------\n";

  printRequiredPkgs2(pkg, addversion) ;

}



//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
}} // ns
