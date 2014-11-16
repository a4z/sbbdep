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
#include <sbbdep/pkgname.hpp>
#include <sbbdep/error.hpp>
#include <sbbdep/log.hpp>
#include <sbbdep/error.hpp>

#include "cachesql.hpp"
#include "backgroundjob.hpp"


#include <a4sqlt3/error.hpp>

#include <vector>
#include <set>
#include <algorithm>
#include <thread>



namespace sbbdep {


using a4sqlt3::Dataset;
using a4sqlt3::SqlCommand;
using a4sqlt3::DbValue;
using a4sqlt3::DbValueType;


Cache::Cache(const std::string& dbname)
:Database(dbname)
,_name(dbname)
{
  
  // TODO register own functions
  sql::register_own_functions(_sql3db.get()) ;
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
  auto transaction = transactionGuard();
  sql::createSchema(*this);

  // ensure we have this, for future ensureDefaults ... ?
  execute("INSERT INTO keyvalstore (key, value) VALUES ('ldsoconf', 0);");

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

      auto trans = transactionGuard();
      if(db_version < calcVersion(0, 2, 1))
        {
          auto transaction = transactionGuard() ;
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


Cache::SyncData
Cache::doSync()
{

  SyncData syncdata;


  if(isNewDb())
    {
      createDbSchema();
      syncdata = createNewSyncData() ; // here, ld dirs im background
      createIndex(syncdata);
     }
  else
    {
      checkDbSchemaVersion() ;
      syncdata = createUpdateSyncData() ; // here, ld dirs im background
                                          // was nicht geht, da ich da auf die db zugreif

      updateIndex(syncdata);
    }


  return syncdata ;



}
//------------------------------------------------------------------------------


Cache::SyncData
Cache::createNewSyncData()
{
  Cache::SyncData retval;
  retval.wasNewCache = true ;

  // add all files to installed,
  auto newfiles_cb = [&retval]( const std::string& dirname,
                                const std::string& filename ) -> bool
    {
      (void)(dirname);
      retval.installed.emplace_back(filename);
      return true ;
    };

  PkgAdmDir.apply(newfiles_cb) ;

  return retval ;


}
//------------------------------------------------------------------------------

Cache::SyncData
Cache::createUpdateSyncData()
{

  // get diff from filesystem and db, remove old stuff from db and insert new
  // get what is not present in fs but in db for remove
  // get what is not present in db but in fs for insert
  // get files with newer date and existing name for handling reinstalled pkgs

  LogInfo() << "search for changes" << std::endl;

  using StringSet  = std::set<std::string> ;

  Cache::SyncData retval;
  retval.wasNewCache = false ;

  // todo i use them later in threads, so they have to be const
  // check if the next thing could return something, 2 values
  // to init this will work with c++17 but has to tie for now
  StringSet allpkgfiles; // all pks in file system
  StringSet newpkgs; // // all pks in file system with a newer date
  const time_t maxknownftime = latesPkgTimeStampInDb() ;

  // helper to wait for a thread
  auto waitfor = [](std::thread& th){ if(th.joinable()) th.join(); };

  // cakback for pkgadm dir, check pkg files add to all and new, if new
  auto fillAllAndNew = [&allpkgfiles, &newpkgs, &maxknownftime]
                              (const std::string& dirname,
                               const std::string& filename) -> bool
        {
          Path path(dirname + "/" + filename);
          if ( path.isRegularFile() ) // this could/should? be an assert, TODO
            { // insert filename in all existing file_pkgs,
              allpkgfiles.insert(filename);
              // and if newer as the latest known file time
              if ( path.getLastModificationTime() > maxknownftime )
                { // insert into new pkgs
                  newpkgs.insert(filename);
                }
            }
          return true ;
        };



  std::thread checkFS(&Dir::apply, &PkgAdmDir,
                      fillAllAndNew, Dir::defaultFilter);
 // TODO default arg does not work, shall I change the interface?


// while this runs, get all from the db
  StringSet allpkgindb; // all pks in the db
  // get all in the database
  auto rh = [&allpkgindb](a4sqlt3::SqlQueryRow& qrow) -> bool
        {
          allpkgindb.insert(qrow[0].getString());
          return true ;
        } ;

  execute("SELECT fullname FROM pkgs;", rh);

  waitfor(checkFS);

  // TODO allpkgfiles , newpkgs create const here cause of shared access


  //get the removed ones, these are not in the filesystem but in the db
  std::thread searchdeleted ( [&allpkgindb, &allpkgfiles , &retval]()->void{
    std::set_difference(allpkgindb.begin(), allpkgindb.end(),
        allpkgfiles.begin(), allpkgfiles.end(),
        std::inserter(retval.removed, retval.removed.begin()));
  });


  // all that are in filesystem but not in db
  // are new and need to be added to installed
  std::set_difference(allpkgfiles.begin(), allpkgfiles.end(),
      allpkgindb.begin(), allpkgindb.end(),
      std::inserter(retval.installed, retval.installed.begin() ));

  // ensure sorted, should not be required ... TODO
  std::sort(std::begin(retval.installed), std::end(retval.installed));

  // new pkgs that are not in the installed list are re-installed
  std::set_difference(newpkgs.begin(), newpkgs.end(),
      retval.installed.begin(), retval.installed.end(),
      std::inserter(retval.reinstalled, retval.reinstalled.begin()));

  waitfor(searchdeleted);

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

  auto transaction = transactionGuard() ;

  struct dbmsg {
    sql_id id ;
    a4sqlt3::DbValues args;
  };


  BackgroundJob<dbmsg> dbjob(
      [this](const dbmsg& msg)
      {
        this->getCommand(msg.id).execute(msg.args) ;
      });


  // t could be a type.. encapsulate ...
  std::mutex mtx;
  StringVec::const_iterator pos;
  auto picker = [&]() -> std::string
      {
        std::unique_lock<std::mutex> lock(mtx);
        return (pos not_eq data.installed.end()) ? *pos++ : std::string() ;
      };


  auto indexer = [&dbjob, picker]()
    {
      auto todo = picker();
      while(not todo.empty())
        {
          // stroe...
          //dbjob.push() .. what ..
          todo = picker();
        }
    };

  // fuers update muss das anders aussehen .... , wie


  // so soll der neue output sein
  // update index package name
  // create index package name
  // remove index package name
  // doch nicht, remove is schlecht


  // what is basically to do, go through all installed,
  // index and push to the db.
  // q: does a thread accelerate this or not
  // well, shoul run in paralle, something ..

  // TODO , schau da die timigs an, ob die sb action

  transaction.commit() ;


}
//------------------------------------------------------------------------------

void
Cache::updateIndex(const SyncData& data)
{
  if(data.wasNewCache)
    {
      // fail !
    }
}
//------------------------------------------------------------------------------


a4sqlt3::SqlCommand&
Cache::getCommand(sql_id id)
{
  auto fi = _commands.find(id);
  if(fi != _commands.end())
    return fi->second;


  auto init = _commands.emplace(id, sql::makeCommand(*this,id)) ;

  if(not init.second)
    throw ErrUnexpected("create command failed") ; // TODO add id to message

  return init.first->second ;


}

int64_t
Cache::latesPkgTimeStampInDb()
{

  a4sqlt3::DbValue val = selectValue(sql::maxPkgTimeStamp());

   return val.isNull() ? 0 : val.getInt64() ;

}

} // ns
