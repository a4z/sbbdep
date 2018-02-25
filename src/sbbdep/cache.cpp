/*
--------------Copyright (c) 2010-2016 H a r a l d  A c h i t z---------------
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


#include <sbbdep/cache.hpp>

#include <sbbdep/config.hpp>
#include <sbbdep/dircontent.hpp>
#include <sbbdep/error.hpp>
#include <sbbdep/ldconf.hpp>
#include <sbbdep/log.hpp>
#include <sbbdep/path.hpp>
#include <sbbdep/pkg.hpp>
#include <sbbdep/pkgname.hpp>
#include <sbbdep/elffile.hpp>
#include <sbbdep/utils/backgroundjob.hpp>
#include <sbbdep/utils/concurrentpeek.hpp>

#include "cachesql.hpp"

#include <sl3/error.hpp>
#include <sl3/columns.hpp>

#include <vector>
#include <set>
#include <algorithm>
#include <thread>



namespace sbbdep {

  namespace
  {
    auto waitfor = [](std::thread& th) { if (th.joinable ()) th.join ();};
  }



  SyncData::StringVec
  SyncData::pkgNameDiff (const StringVec& a, const StringVec& b)
  {

    using namespace std;
    auto samePkgName = [] (const string& n1, const string& n2) -> bool
      {
        return PkgName (n1).name () < PkgName (n2).name ();
      };

    StringVec ret;
    auto retInserter = inserter (ret, begin (ret));

    set_difference (begin (a), end (a),
                    begin (b), end (b),
                    retInserter,
                    samePkgName);

    return ret;
  }



std::string
Cache::defaultDb = []()
{
  const std::string default_dir =
    std::getenv("HOME") + std::string{"/.cache"};

  const char* xdg_dir = std::getenv("XDG_CACHE_HOME");

  const std::string cache_dir =
    (xdg_dir == nullptr || xdg_dir[0] == '\0') ?
    default_dir :
    xdg_dir ;

  return cache_dir + std::string{"/sbbdep.db"};
}();



using sl3::Dataset;
using sl3::Command;
using sl3::DbValue;
using sl3::Type;


Cache::Cache(const std::string& dbname)
:Database(dbname)
,_name(dbname)
{
  sql::register_own_functions (db ()) ;
}
//------------------------------------------------------------------------------


Cache::~Cache()
{
  
}
//------------------------------------------------------------------------------


bool
Cache::isNewDb()
{
  const std::string sql =
      "SELECT COUNT (*) FROM sqlite_master WHERE name = 'pkgs' ;" ;

  const DbValue countStar = selectValue(sql) ;

  return countStar.getInt() == 0;

}
//------------------------------------------------------------------------------


bool
Cache::compatibleVersion ()
{
  // helpers to get a useful version number that can be compared
  auto calcMajorMinor = [](int ma, int mi ) noexcept -> int
  {
    return (100000 + ma * 10000) + (1000 + mi * 100);
  };

  Dataset ds =  select("SELECT major, minor , patchlevel FROM version",
                       {Type::Int, Type::Int, Type::Int});
  SBBASSERT (ds.size () == 1);

  const auto appMajorMinor = calcMajorMinor(MAJOR_VERSION, MINOR_VERSION);
  const auto dbMajorMinor = calcMajorMinor(ds[0][0].getInt (),
                                           ds[0][1].getInt ());

  if( appMajorMinor == dbMajorMinor )
    { // nothting to do
      return true ;
    }
  else if( appMajorMinor < dbMajorMinor )
    { // very strange
      LogError() << "app version < db version, using an old sbbdep version?" ;
    }

  return false ;

}



  void
Cache::createDbSchema()
{
  auto transaction = beginTransaction();
  sql::createSchema(*this);

  transaction.commit();
}
//------------------------------------------------------------------------------


SyncData
Cache::doSync()
{

  SyncData syncdata;

  auto initld = []() { getLDDirs(); } ;

  try
    {
      if(isNewDb ())
        {
          LogInfo () << "create cache " << getName () ;
          createDbSchema ();
          std::thread t (initld);
          syncdata = createNewSyncData ();
          waitfor (t);
          updateLdDirInfo ();
          createIndex (syncdata);
        }
      else
        {
          LogInfo () << "sync cache " << getName () ;
          std::thread t (initld);
          syncdata = createUpdateSyncData ();
          waitfor (t);
          updateLdDirInfo ();
          updateIndex (syncdata);
        }
    }
  catch (const sl3::Error& e)
    {
      LogError () << e ;
      throw;
    }


  // update ldds .....


  return syncdata ;



}
//------------------------------------------------------------------------------


SyncData
Cache::createNewSyncData()
{
  SyncData retval;
  retval.wasNewCache = true ;

  // add all files to installed,
  auto newfiles_cb = [&retval]( const std::string& dirname,
                                const std::string& filename ) -> bool
    {
      (void)(dirname);
      retval.installed.emplace_back (filename);
      return true ;
    };

  pkgAdmDir().forEach (newfiles_cb) ;

  return retval ;


}
//------------------------------------------------------------------------------

SyncData
Cache::createUpdateSyncData()
{
  // this is a very long function, but it is as it is, top down
  // lot of lookup, filter and sorting

  // get diff from filesystem and db, remove old stuff from db and insert new
  // get what is not present in fs but in db for remove
  // get what is not present in db but in fs for insert
  // get files with newer date and existing name for handling reinstalled pkgs
  // then sort the updated into the update slots so that SyncData contains
  // what is shall contain



  using namespace std;
  using StringSet  = std::set<std::string> ;

  SyncData retval;
  retval.wasNewCache = false ;


  StringSet allpkgfiles; // all pks in file system
  StringSet newpkgs; // // all pks in file system with a newer date

  const auto val = selectValue (sql::maxPkgTimeStamp ());
  const int64_t maxknownftime = val.isNull () ? 0 : val.getInt () ;


  // callback for pkgadm dir, check pkg files add to all and new, if new
  auto fillAllAndNew = [&allpkgfiles, &newpkgs, &maxknownftime]
                              (const std::string& dirname,
                               const std::string& filename) -> bool
        {
          Path path(dirname + "/" + filename);
          if (path.isRegularFile ())
            { // insert filename in all existing file_pkgs,
              allpkgfiles.insert (filename);
              // and if newer as the latest known file time
              if (path.getLastModificationTime () > maxknownftime)
                { // insert into new pkgs
                  newpkgs.insert (filename);
                }
            }
          else
            {
              LogError() << "(fillAllAndNew) can't read file " << path.str() ;
            }
          return true ;
        };


  const Dir admdir = pkgAdmDir() ;
  std::thread checkFS(&Dir::forEach, &admdir,
                      fillAllAndNew, Dir::defaultFilter);


// while this runs, get all from the db
  StringSet allpkgindb; // all pks in the db
  // get all in the database
  auto rh = [&allpkgindb](sl3::Columns cols) -> bool
        {
          allpkgindb.insert (cols.getValue (0).getText ());
          return true ;
        } ;

  execute ("SELECT fullname FROM pkgs;", rh);

  waitfor (checkFS);

  // NOTE from here, allpkgindb allpkgfiles newpkgs have to be read only

  StringVec installed , removed ;
  // need temporary store to filter updated ...


  //get the removed ones, these are not in the filesystem but in the db
  std::thread searchdeleted ( [&allpkgindb, &allpkgfiles , &removed]()->void{
    std::set_difference(allpkgindb.begin(), allpkgindb.end(),
        allpkgfiles.begin(), allpkgfiles.end(),
        std::inserter(removed, removed.begin()));
  });


  // all that are in filesystem but not in db
  // are new and need to be added to installed
  std::set_difference(allpkgfiles.begin(), allpkgfiles.end(),
      allpkgindb.begin(), allpkgindb.end(),
      std::inserter(installed, installed.begin() ));

  // ensure sorted, even if it should not be required ...
  //std::sort(std::begin(installed), std::end(installed));
  SBBASSERT (std::is_sorted (installed.begin (), installed.end ())) ;

  // new pkgs that are not in the installed list are re-installed
  std::set_difference(newpkgs.begin(), newpkgs.end(),
      installed.begin(), installed.end(),
      std::inserter(retval.reinstalled, retval.reinstalled.begin()));

  waitfor(searchdeleted);

  // done above
  //SBBASSERT (std::is_sorted (installed.begin (), installed.end ())) ;

  SBBASSERT (std::is_sorted (removed.begin (), removed.end ())) ;

  SBBASSERT (std::is_sorted (retval.reinstalled.begin (),
                             retval.reinstalled.end ())) ;


// find updated, sort them out
// updated are in removed and in insert, different version but same name

  // filter removed and installed, get update ,real removed, real installed

  StringVec updataOld , updateNew ;

  auto isIn = [](const StringVec& allnames, const string& name) -> bool
    {
        PkgName pkn (name) ;

        auto samename = [&pkn] (const string& n)
        {
          return PkgName (n).name () == pkn.name () ;
        };
        auto fiter = find_if (begin (allnames), end (allnames),  samename) ;

        return fiter != end (allnames) ;
    };



  for (auto&& name : removed)
    {
      if (isIn(installed, name))
        {
          updataOld.emplace_back (name);
        }
      else
        {
          retval.removed.emplace_back( name);
        }
    }

    for (auto&& name : installed)
      {
        if (isIn (removed, name))
          {
            updateNew.emplace_back (name);
          }
        else
          {
            retval.installed.emplace_back (name);
          }
      }

    // this means something is strange, maybe a 2 packages with the same name
    // but different version have been installed/removed ....
    if (updataOld.size () != updateNew.size ())
      {
        StringVec diffOld = SyncData::pkgNameDiff (updataOld, updateNew);
        StringVec diffNew = SyncData::pkgNameDiff (updateNew, updataOld);

        if (not diffOld.empty ())
          {
            updataOld = SyncData::pkgNameDiff (updataOld, diffOld);
          }

        if (not diffNew.empty ())
          {
            updateNew = SyncData::pkgNameDiff (updateNew, diffNew);
          }

        retval.problemsOld = std::move (diffOld) ;
        retval.problemsNew = std::move (diffNew) ;
      }

  SBBASSERT( updataOld.size() == updateNew.size() ) ;
  //"filter update did not work"

  for(size_t i = 0; i < updataOld.size(); ++i)
    {
      auto p = make_pair(PkgName(updataOld[i]),PkgName(updateNew[i])) ;

      SBBASSERT (p.first.name () == p.second.name ()) ;

      retval.updated.emplace_back(p);
    }


  return retval;
}
//------------------------------------------------------------------------------



void
Cache::createIndex(const SyncData& data)
{
#ifdef DEBUG
  SBBASSERT (data.removed.empty() && data.reinstalled.empty());
  SBBASSERT (data.wasNewCache );
#endif


  BackgroundJob<Pkg> dbjob(
      [this](const Pkg& pkg)
      {  LogInfo() << "index " << pkg.getPath().base() ;
        this->indexPkg(pkg) ;
      });


  ConcurrentPeek<std::string> picker(data.installed) ;

  auto indexWorker = [&dbjob, &picker]()
    {
      std::string todo = picker();
      while(not todo.empty())
        {
          const auto filename = pkgAdmDir().getName() + "/" + todo ;
          auto pkg = Pkg::create(filename, PkgType::Installed);
          LogInfo() << "load " << pkg.getPath().base() ;
          pkg.Load() ;
          dbjob.push(std::move(pkg));
          todo = picker();
        }
    };

  auto transaction = beginTransaction() ;

  std::thread t1(indexWorker) ;
  std::thread t2(indexWorker) ;

  waitfor(t1);
  waitfor(t2);

  dbjob.stop(); // if there is something left, before commit

  transaction.commit() ;


}
//------------------------------------------------------------------------------

void
Cache::updateIndex(const SyncData& data)
{
  if(data.wasNewCache)
    {
      throw ErrUnexpected("Invalid argument, SyncData is new") ;
    }

  using namespace std;

  if (data.removed.empty() && data.updated.empty () &&
      data.reinstalled.empty() && data.installed.empty () )
    { // nothing to do
      return ;
    }

  // next re-factoring,  create this earlier ...
  vector<pair<string, string>> todos;

  for(auto&& v : data.removed)
    todos.emplace_back(v, string());

  for(auto&& v : data.updated)
    todos.emplace_back(v.first.fullName(), v.second.fullName());

  for(auto&& v : data.reinstalled)
    todos.emplace_back(v, v);

  for(auto&& v : data.installed)
     todos.emplace_back( string(), v);



  struct DbAction
  {
    std::string rem;
    Pkg inst;
  };


  //LogDebug() << "here with " << todos.size() ;

  BackgroundJob<DbAction> dbjob(
      [this] (const DbAction& action)
      {
        if(not action.rem.empty ())
          { LogInfo() << "clear " << action.rem ;
            auto& delcmd =  getCommand (sqlid::del_byfullname) ;
            delcmd .execute (sl3::parameters(action.rem));
          }

        if(action.inst.isLoaded ())
          { LogInfo () << "index " << action.inst.getPath ().base () ;
            this->indexPkg (action.inst) ;
          }
      });

  // delete , install
  ConcurrentPeek<pair<string, string>> picker(todos) ;

  auto indexWorker = [&dbjob, &picker]()
    {
      auto todo = picker ();

      // first must always exist
      while (not (todo.first.empty () && todo.second.empty ()))
        {
          if (not todo.second.empty ())
            {
              const auto filename = pkgAdmDir().getName () + "/" + todo.second;
              auto pkg = Pkg::create (filename, PkgType::Installed);
              LogInfo() << "load " << pkg.getPath ().base ();
              pkg.Load ();
              dbjob.push (DbAction {todo.first, move (pkg)});
            }
          else
            {
              dbjob.push (DbAction {todo.first, Pkg ()});
            }

          todo = picker ();
        }
    };


  auto transaction = beginTransaction() ;

  std::thread t1(indexWorker) ;
  std::thread t2(indexWorker) ;

  waitfor(t1);
  waitfor(t2);

  dbjob.stop(); // if there is something left, before commit

  transaction.commit() ;

  LogDebug () << "run db analyzer\n";
  execute ("ANALYZE ;");
}
//------------------------------------------------------------------------------


sl3::Command&
Cache::getCommand(sqlid id)
{
  auto fi = _commands.find(id);
  if(fi != _commands.end())
    {
      return fi->second;
    }

  auto init = _commands.emplace(id, sql::makeCommand(*this,id)) ;

  if(not init.second)
    {
      auto strid = std::to_string((int)id) ;
      throw ErrUnexpected("create command " + strid + "failed") ;
    }

  return init.first->second ;


}
//------------------------------------------------------------------------------


void
Cache::indexPkg(const Pkg& pkg)
{

  SBBASSERT (pkg.isLoaded ()) ;

  const auto dldirs = select ("SELECT DISTINCT dirname  as Prio FROM lddirs "
                      "UNION SELECT DISTINCT dirname as Prio FROM ldlnkdirs;");

  try
    {
      const auto pkgname = PkgName (pkg.getPath ().base ());
      const int64_t timestamp = pkg.getPath ().getLastModificationTime ();

      getCommand(sqlid::insert_pkg).execute( 
      sl3::parameters( pkgname.fullName()  ,
                      pkgname.name()  ,
                      pkgname.version()  ,
                      pkgname.arch()  ,
                      pkgname.build().Num()  ,
                      pkgname.build().Tag()  ,
                      timestamp 
                      ));

      const auto pkgid = getLastInsertRowid() ;

      for (const ElfFile& elf : pkg.getElfFiles ())
        {
              auto dir = elf.isLibrary ()?
                         findDirWithLink(elf, dldirs) :
                         elf.getName().dir()
              ;

          if (dir != elf.getName ().dir ())
            LogInfo () << elf.getName ().dir () << " " << dir ;

          getCommand(sqlid::insert_dynlinked).execute( 
                sl3::parameters(      
                   pkgid  ,
                   elf.getName().str()  ,
                    dir  ,
                    elf.getName().base()  ,
                   (elf.soName().size()> 0 ?
                           DbValue(elf.soName()) :
                           DbValue(sl3::Type::Null) ) ,
                    elf.getArch() 
          ));

         const auto dynlinked_id = getLastInsertRowid() ;

         for(const auto& needed : elf.getNeeded())
           {
             getCommand(sqlid::insert_required).execute(
                   sl3::parameters(dynlinked_id, needed)
             );
           }

         for(const auto& rrunpath : elf.getRRunPaths())
           {
             getCommand(sqlid::insert_rrunpath).execute(
               sl3::parameters(dynlinked_id, rrunpath , elf.getName().dir())
             );
           }
        }

    }
  catch (const sl3::Error& e)
    {
      LogError () << e;
      throw;
    }

}
//------------------------------------------------------------------------------

void
Cache::updateLdDirInfo()
{
  auto ldinfos = getLDDirs ();

  auto transaction = beginTransaction ();

  execute ("DELETE FROM lddirs;");
  execute ("DELETE FROM ldlnkdirs;");



  getCommand (sqlid::set_keyval).execute (
      sl3::parameters ("ldsoconf" , ldinfos.getLdSoConfTime ()));

  for (auto&& d : ldinfos.getLdDirs ())
    {
      getCommand (sqlid::insert_ldDir).execute (sl3::parameters (d));
    }

  for (auto&& d : ldinfos.getLdLnkDirs ())
    {
      getCommand (sqlid::insert_ldLnkDir).execute (sl3::parameters (d));
    }

  transaction.commit ();


}
//------------------------------------------------------------------------------


sl3::Command&
Cache::namedCommand(const std::string& name,
                    const char* sql)
{

  auto f = _nameCommands.find (name);
  if (f == _nameCommands.end ())
    {
      auto in = _nameCommands.emplace (name, prepare (sql));
      // assert in.second  TOOD
      f = in.first;
    }

  return f->second;

}



  // put this here until there is a better location
  std::string
  Cache::findDirWithLink(const ElfFile& elf, const sl3::Dataset& ldirs)
  {

    SBBASSERT (elf.getType () == ElfFile::Library);

    // LogDebug () << "Check " << elf.getName () ;

    const auto pn = elf.getName ().dir () ;

    auto d = std::find_if (ldirs.begin (), ldirs.end (), [&pn](const auto& val) {
      return val[0].getText() == pn;
    }) ;

    if (d == ldirs.end ())
      {
        for (const auto& dir : ldirs)
          {
            const std::string tocheck = dir[0].getText () + "/" + elf.soName ();

            Path p{tocheck};

            if (p.isValid () and p.isLink () and p.getRealPath () == elf.getName ())
              {
                //LogDebug () << "found " << p.dir ()  ;
                return p.dir () ;
              }
          }
      }

    // LogDebug () << "no link, resolve " << pn ;
    return pn;
  }




} // ns
