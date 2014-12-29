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


#include <sbbdep/cache.hpp>


#include <sbbdep/config.hpp>

#include <sbbdep/dircontent.hpp>
#include <sbbdep/path.hpp>
#include <sbbdep/pkg.hpp>
#include <sbbdep/pkgname.hpp>
#include <sbbdep/error.hpp>
#include <sbbdep/log.hpp>
#include <sbbdep/error.hpp>
#include <sbbdep/lddirs.hpp>


#include "cachesql.hpp"
#include "backgroundjob.hpp"


#include <a4sqlt3/error.hpp>

#include <vector>
#include <set>
#include <algorithm>
#include <thread>



namespace sbbdep {


namespace
{


auto waitfor = [](std::thread& th){ if(th.joinable()) th.join(); };

}



using a4sqlt3::Dataset;
using a4sqlt3::SqlCommand;
using a4sqlt3::DbValue;
using a4sqlt3::DbValueType;


Cache::Cache(const std::string& dbname)
:Database(dbname)
,_name(dbname)
{
  sql::register_own_functions(_connection->db()) ;
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

  return countStar.getInt64() == 0;

}
//------------------------------------------------------------------------------


void
Cache::createDbSchema()
{
  auto transaction = beginTransaction();
  sql::createSchema(*this);

  // ensure we have this, for future ensureDefaults ... ?
  //execute("INSERT INTO keyvalstore (key, value) VALUES ('ldsoconf', 0);");
  // this is now in create sheam, TODO clean up

  transaction.commit();
}
//------------------------------------------------------------------------------


/*
 * update db for the current version,
 * this code will change form time to time, old version will be dropped
 * for the future, think about, if there is an old cache if not simply
 * re-creating would be better, could be less and less complex code
 */
void
Cache::checkDbSchemaVersion()
{

  { // ensure there is a version table
    // sooner or later this code will go away, no support for old db or so ...
    const auto sql =
        "select count(*) from sqlite_master where name='version';";
    DbValue rc = this->selectValue(sql);
    if(rc.getInt64() == 0)
      {
        sql::addVersionTable(*this);
      }
  }

  // helpers to get a useful version number that can be compared
  auto calcMajorMinorVersion = [](int ma, int mi ) noexcept -> int
    {
      return (100000 + ma * 10000) + (1000 + mi * 100);
    };
  auto calcVersion = [](int ma, int mi, int pl ) noexcept -> int
    {
      return (100000 + ma * 10000) + (1000 + mi * 100) + pl;
    };


  auto getDbMajorMinorVersion = [calcMajorMinorVersion, this]() -> int
    {
      Dataset ds( {DbValueType::Int64, DbValueType::Int64} );
      execute("SELECT major, minor FROM version", ds);
      // TODO assert ds.size() == 1
      return calcMajorMinorVersion(ds[0][0].getInt64(), ds[0][1].getInt64());
    };
  auto getDbVersion = [calcVersion, this]() -> int
    {
      Dataset ds({DbValueType::Int64, DbValueType::Int64, DbValueType::Int64});
      execute("SELECT major, minor , patchlevel FROM version", ds);
      // TODO assert ds.size() == 1
      return calcVersion(ds[0][0].getInt64(),
                         ds[0][1].getInt64(),
                         ds[0][2].getInt64());
    };

  const auto app_version = calcVersion(MAJOR_VERSION,
                                       MINOR_VERSION,
                                       PATCH_VERSION);

  const auto db_version = getDbVersion();

  if( app_version == db_version )
    { // nothting to do
      return ;
    }


  if( db_version < app_version )
    {
      if( getDbMajorMinorVersion() ==  calcMajorMinorVersion(0, 1) )
        {
          LogError() << "existing cache db was build with an old version of "
                        "sbbdep.\n"
                        "please create a new cache by using the -c option "
                        "or removing " << _name << ".\n"
                        "Sorry for the inconvenience caused.\n\n" ;
          throw ErrGeneric("old databaes version in use");
        }

      auto trans = beginTransaction();
      if(db_version < calcVersion(0, 2, 1))
        {
          auto transaction = beginTransaction() ;
          const char* sql =
          "CREATE TABLE keyvalstore (key  NOT NULL,  value  NOT NULL); "
          "create unique index  idx_keyvalstore_key on keyvalstore (key); "
          "insert into keyvalstore (key, value) values ('ldsoconf', 0);" ;
          execute(sql) ;
          transaction.commit();
        }


      std::string sqcmd =
          "UPDATE version set major = " + std::to_string(MAJOR_VERSION) +
          ", minor = " + std::to_string(MINOR_VERSION) +
          ", patchlevel = " + std::to_string(MINOR_VERSION) + ";" ;
      execute(sqcmd);
      trans.commit();
   }


  { // should this be in ensureDefault ..  TODO
    const std::string sql=
      "SELECT COUNT(*) FROM keyvalstore WHERE key='ldsoconf';";

    if(selectValue(sql).getInt64() == 0)
      {
        execute("INSERT INTO keyvalstore (key, value) "
                "VALUES ('ldsoconf', 0);");
      }
  }


#ifdef DEBUG
  const auto calcver = calcVersion(MAJOR_VERSION, MINOR_VERSION, MINOR_VERSION);
  if( calcver != getDbVersion() )
    throw ErrUnexpected("version update incorrect");
#endif


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
          createDbSchema ();
          std::thread t (initld);
          syncdata = createNewSyncData ();
          waitfor (t);
          updateLdDirInfo ();
          createIndex (syncdata);
        }
      else
        {
          checkDbSchemaVersion ();
          std::thread t (initld);
          LogInfo () << "sync cache " << getName () ;
          // TODO this ^^ should not be here
          // but where the cache create message is
          syncdata = createUpdateSyncData ();
          waitfor (t);
          updateLdDirInfo ();
          updateIndex (syncdata);
        }

    }
  catch (const a4sqlt3::Error& e)
    {
      LogError () << e ;
      throw;
    }



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

  PkgAdmDir.forEach (newfiles_cb) ;

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

  // todo i use them later in threads, so they have to be const
  // check if the next thing could return something, 2 values
  // to init this will work with c++17 but has to tie for now
  StringSet allpkgfiles; // all pks in file system
  StringSet newpkgs; // // all pks in file system with a newer date

  const auto val = selectValue (sql::maxPkgTimeStamp ());
  const time_t maxknownftime = val.isNull () ? 0 : val.getInt64 () ;


  // callback for pkgadm dir, check pkg files add to all and new, if new
  auto fillAllAndNew = [&allpkgfiles, &newpkgs, &maxknownftime]
                              (const std::string& dirname,
                               const std::string& filename) -> bool
        {
          Path path(dirname + "/" + filename);
          if (path.isRegularFile ()) // this could/should? be an assert, TODO
            { // insert filename in all existing file_pkgs,
              allpkgfiles.insert (filename);
              // and if newer as the latest known file time
              if (path.getLastModificationTime () > maxknownftime)
                { // insert into new pkgs
                  newpkgs.insert (filename);
                }
            }
          return true ;
        };



  std::thread checkFS(&Dir::forEach, &PkgAdmDir,
                      fillAllAndNew, Dir::defaultFilter);
 // TODO default arg does not work, shall I change the interface?


// while this runs, get all from the db
  StringSet allpkgindb; // all pks in the db
  // get all in the database
  auto rh = [&allpkgindb](a4sqlt3::SqlQueryRow& qrow) -> bool
        {
          allpkgindb.insert (qrow [0].getText ());
          return true ;
        } ;

  execute ("SELECT fullname FROM pkgs;", rh);

  waitfor (checkFS);

  // TODO allpkgfiles , newpkgs create const here cause of shared access

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

  // ensure sorted, should not be required ... TODO
  std::sort(std::begin(installed), std::end(installed));

  // new pkgs that are not in the installed list are re-installed
  std::set_difference(newpkgs.begin(), newpkgs.end(),
      installed.begin(), installed.end(),
      std::inserter(retval.reinstalled, retval.reinstalled.begin()));

  waitfor(searchdeleted);

  // TODO debug asserts
  if(not std::is_sorted(installed.begin(), installed.end()))
    throw ErrUnexpected("installed not sorted") ;

  if(not std::is_sorted(removed.begin(), removed.end()))
    throw ErrUnexpected("removed not sorted") ;

  if(not std::is_sorted(retval.reinstalled.begin(), retval.reinstalled.end()))
    throw ErrUnexpected("retval.reinstalled not sorted") ;

// find updated, sort them out
// updated are in removed and in insert, different version but same name

  // filter removed and installed, get update ,real removed, real installed

  StringVec updataOld , updateNew ;

  auto isIn = [](const StringVec& allnames, const string& name) -> bool
    {
        PkgName pkn(name) ;

        auto samename = [&pkn](const string& n)
        {
          return PkgName(n).Name() == pkn.Name() ;
        };
        auto fiter = find_if(begin(allnames), end(allnames),  samename) ;

        return fiter != end(allnames) ;
    };


  for (auto&& name : removed)
    {
      if (isIn(installed, name))
        {
          updataOld.emplace_back(name);
        }
      else
        {
          retval.removed.emplace_back(name);
        }
    }

  for (auto&& name : installed)
    {
      if (isIn(removed, name))
        {
          updateNew.emplace_back(name);
        }
      else
        {
          retval.installed.emplace_back(name);
        }
    }


  if(updataOld.size() != updateNew.size()) // TODO assert
    throw ErrUnexpected("filter update did not work") ;

  for(size_t i = 0; i < updataOld.size(); ++i)
    {
      auto p = make_pair(PkgName(updataOld[i]),PkgName(updateNew[i])) ;

      if(p.first.Name() != p.second.Name()) // TODO assert
        throw ErrUnexpected("name old new does not match") ;

      retval.updated.emplace_back(p);
    }


  return retval;
}
//------------------------------------------------------------------------------



void
Cache::createIndex(const SyncData& data)
{
// TODO das muss was anderes werden
  if(data.removed.size() != 0 || data.reinstalled.size() != 0 )
    {
      throw ErrUnexpected("Invalid argument in createIndex") ;
    }
  if(not data.wasNewCache)
    {
      throw ErrUnexpected("Invalid argument, SyncData not new") ;
    }



  BackgroundJob<Pkg> dbjob(
      [this](const Pkg& pkg)
      {  LogDebug() << "index " << pkg.getPath().getBase() ;
        this->indexPkg(pkg) ;
      });


  ConcurrentPeek<std::string> picker(data.installed) ;

  auto indexWorker = [&dbjob, &picker]()
    {
      std::string todo = picker();
      while(not todo.empty())
        {
          // TODO new log system, msg based
          const auto filename = PkgAdmDir.getName() + "/" + todo ;
          auto pkg = Pkg::create(filename, PkgType::Installed);
          LogDebug() << "load " << pkg.getPath().getBase() ;
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

  // next re-factoring,  create this earlier ...
  vector<pair<string, string>> todos;

  for(auto&& v : data.removed)
    todos.emplace_back(v, string());

  for(auto&& v : data.updated)
    todos.emplace_back(v.first.FullName(), v.second.FullName());

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
      [this](const DbAction& action)
      {
        if(not action.rem.empty())
          { LogInfo() << "clear " << action.rem ;
            auto& delcmd =  getCommand(sqlid::del_byfullname) ;
            delcmd .execute( { {action.rem} } );
          }

        if(action.inst.isLoaded())
          { LogInfo() << "index " << action.inst.getPath().getBase() ;
            this->indexPkg(action.inst) ;
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
              const auto filename = PkgAdmDir.getName () + "/" + todo.second;
              auto pkg = Pkg::create (filename, PkgType::Installed);
              LogInfo() << "load " << pkg.getPath ().getBase ();
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


}
//------------------------------------------------------------------------------


a4sqlt3::SqlCommand&
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
      throw ErrUnexpected("create command failed") ; // TODO add id to message
    }

  return init.first->second ;


}
//------------------------------------------------------------------------------


void
Cache::indexPkg(const Pkg& pkg)
{
  // TODO assert(pkg.isLoaded()) ;

  // todo might change and use worker, runBlocked to
  // but i must think about this
  try
    {
      const auto pkgname = PkgName(pkg.getPath().getBase()) ;
      const auto timestamp = pkg.getPath().getLastModificationTime() ;

      getCommand(sqlid::insert_pkg).execute( {
                 { pkgname.FullName() } ,
                 { pkgname.Name() } ,
                 { pkgname.Version() } ,
                 { pkgname.Arch() } ,
                 { pkgname.Build().Num() } ,
                 { pkgname.Build().Tag() } ,
                 { timestamp }
       });

      const auto pkgid = getLastInsertRowid() ;

      for(const ElfFile& elf : pkg.getElfFiles())
        {
          getCommand(sqlid::insert_dynlinked).execute( {
                   { pkgid } ,
                   { elf.getName().Str() } ,
                   { elf.getName().getDir() } ,
                   { elf.getName().getBase() } ,
                   (elf.soName().size()> 0 ?
                           DbValue(elf.soName()) :
                           DbValue(a4sqlt3::DbValueType::Null) ) ,
                   { elf.getArch() }
          });

         const auto dynlinked_id = getLastInsertRowid() ;

         for(const auto& needed : elf.getNeeded())
           {
             getCommand(sqlid::insert_required).execute(
                 { {dynlinked_id}, {needed} }
             );
           }

         for(const auto& rrunpaht : elf.getRRunPaths())
           {
             getCommand(sqlid::insert_rrunpath).execute(
                 { {dynlinked_id}, {rrunpaht} , {elf.getName().getDir()} }
             );
           }
        }

    }
  catch (const a4sqlt3::Error& e)
    {
      LogError () << e;
      throw;
    }

}
//------------------------------------------------------------------------------

void
Cache::updateLdDirInfo()
{
  auto ldinfos = getLDDirs() ;

  auto transaction = beginTransaction();

  execute("DELETE FROM lddirs;");
  execute("DELETE FROM ldlnkdirs;");

  getCommand(sqlid::set_keyval).execute({  {"ldsoconf"},
                                           {ldinfos.getLdSoConfTime()}  });


  for (auto&&  d:  ldinfos.getLdDirs())
    {
      getCommand(sqlid::insert_ldDir).execute({{d}});
    }

  for (auto&&  d:  ldinfos.getLdLnkDirs())
    {
      getCommand(sqlid::insert_ldLnkDir).execute({{d}});
    }

  transaction.commit();


}
//------------------------------------------------------------------------------

int64_t // TODO is this  worth a own memmber function ...
Cache::latesPkgTimeStampInDb()
{

  a4sqlt3::DbValue val = selectValue(sql::maxPkgTimeStamp());

   return val.isNull() ? 0 : val.getInt64() ;

}
//------------------------------------------------------------------------------



a4sqlt3::SqlCommand&
Cache::namedCommand(const std::string& name,
                    const char* sql)
{

  auto f = _nameCommands.find(name) ;
  if (f == _nameCommands.end())
    {
      auto in = _nameCommands.emplace(name, command(sql)) ;
      // assert in.second  TOOD
      f=  in.first;
    }

  return f->second ;

}


} // ns
