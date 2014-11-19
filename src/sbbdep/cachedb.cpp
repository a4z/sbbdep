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


#include <sbbdep/cachedb.hpp>
#include <sbbdep/cachesql.hpp>
#include <sbbdep/path.hpp>
#include <sbbdep/log.hpp>
#include <sbbdep/pkg.hpp>
#include <sbbdep/pkgname.hpp>
#include <sbbdep/pkgadmdir.hpp>
#include <sbbdep/config.hpp> // generated
#include <sbbdep/lddirs.hpp>
#include <sbbdep/error.hpp>

#include <a4sqlt3/sqlcommand.hpp>
#include <a4sqlt3/dataset.hpp>

#include <a4sqlt3/error.hpp>

#include <sqlite3.h>

#include <cstdlib>
#include <algorithm>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <map>

#include "conststr.hpp"

namespace sbbdep {

using a4sqlt3::Database;
using a4sqlt3::SqlCommand;
using a4sqlt3::DbValue;
using a4sqlt3::DbValueType ;


namespace {



void replace_origin_func(sqlite3_context *context,
                                int argc, sqlite3_value **argv)
{
  if (argc != 2)
    { // TODO this needs a test
      conststr errmsg { "incorrect count of arguments, should be 2" };
      sqlite3_result_error(context,errmsg.c_str(), errmsg.size() ) ;
    }

  std::string filepath = (const char*)sqlite3_value_text(argv[0]);
  std::string homepath = (const char*)sqlite3_value_text(argv[1]);

  std::string result= replaceORIGIN(filepath, homepath);

  sqlite3_result_text(context, result.c_str(), -1, SQLITE_TRANSIENT);

}//-----------------------------------------------------------------------------


void make_realpath_func(sqlite3_context *context,
                               int argc, sqlite3_value **argv)
{
  if (argc != 1)
    { // TODO this needs a test
      conststr errmsg {"incorrect count of arguments, should be 1"} ;
      sqlite3_result_error(context,errmsg.c_str(), errmsg.size() ) ;
    }

  Path p((const char*)sqlite3_value_text(argv[0]));
  p.makeAbsolute();
  p.makeRealPath();

  if ( p.isValid() )
    sqlite3_result_text(context, p.getURL().c_str(), -1, SQLITE_TRANSIENT);
  else
    sqlite3_result_null(context);
}//-----------------------------------------------------------------------------
} // anno ns

void register_own_functions(sqlite3* db)
{
//todo fuer parameter 4 ?? re check docu
//auto flag = SQLITE_UTF8 | SQLITE_DETERMINISTIC ;
sqlite3_create_function(db, "replaceOrigin", 2, 0,0,
                        &replace_origin_func , 0 , 0 );

sqlite3_create_function(db, "mkRealPath", 1, 0,0,
                        &make_realpath_func , 0 , 0 );
// mkRealPath( replaceOrigin("$ORIGIN/../where/ever", dirOfFile) )

}




class DbAction
{

  a4sqlt3::SqlCommand* m_cmdpkg;
  a4sqlt3::SqlCommand* m_cmddynlinked;
  a4sqlt3::SqlCommand* m_cmdrequired;
  a4sqlt3::SqlCommand* m_cmdrrunpath;
  a4sqlt3::SqlCommand* m_cmddelpkg;
  CacheDB& m_dbref;


public:
  DbAction( CacheDB& dbref )
  : m_cmdpkg(nullptr)
  , m_cmddynlinked(nullptr), m_cmdrequired(nullptr)
  , m_cmdrrunpath(nullptr),  m_cmddelpkg(nullptr)
  , m_dbref(dbref)
  {
    using namespace a4sqlt3;

    if(!(m_cmdpkg=m_dbref.getCommand("InsertPkgSQL"))) {
      m_cmdpkg = m_dbref.createStoredCommand("InsertPkgSQL", CacheSQL::InsertPkgSQL(),
          { DbValueType::Text, DbValueType::Text, DbValueType::Text,
              DbValueType::Text, DbValueType::Int64,
              DbValueType::Text, DbValueType::Int64 } ) ;
    }

    if(!(m_cmddynlinked=m_dbref.getCommand("InsertDynLinkedSQL"))) {
        m_cmddynlinked = m_dbref.createStoredCommand("InsertDynLinkedSQL", CacheSQL::InsertDynLinkedSQL(),
            {DbValueType::Int64,DbValueType::Text,
                DbValueType::Text, DbValueType::Text
                ,DbValueType::Text, DbValueType::Int64 } );
    }

    if(!(m_cmdrequired=m_dbref.getCommand("InsertRequiredSQL"))) {
        m_cmdrequired = m_dbref.createStoredCommand("InsertRequiredSQL", CacheSQL::InsertRequiredSQL(),
            {DbValueType::Int64,DbValueType::Text});
    }

    if(!(m_cmdrrunpath=m_dbref.getCommand("InsertRRunPathSQL"))) {
        m_cmdrrunpath = m_dbref.createStoredCommand("InsertRRunPathSQL", CacheSQL::InsertRRunPathSQL(),
            {DbValueType::Int64,DbValueType::Text, DbValueType::Text});
    }

    if(!(m_cmddelpkg=m_dbref.getCommand("DeletePkgByFullnameSQL"))) {
        m_cmddelpkg = m_dbref.createStoredCommand("DeletePkgByFullnameSQL", CacheSQL::DeletePkgByFullnameSQL(),
            {DbValueType::Text});
    }


  }//-----------------------------------------------------------------------------------------------



  void
  Store( const PkgName& pkgname, const Pkg::ElfFiles& dllist, const int64_t& timestamp)
  {
    using a4sqlt3::DbValue ;

    LogInfo()<< "store " << pkgname.FullName() << std::endl;

    // what's going on here:
    // store the pkg,
    // to the pkg store all dynlinked infos
    // to each dynlinked info store Needed list and RunRPaths info

    // instead of capture this and writing always this->bla , for now ....


    auto store_pkg = [this](const PkgName& pkgname, const int64_t& timestamp) -> int64_t {
        a4sqlt3::DbValues param_vals = {
           DbValue( pkgname.FullName() ) ,
           DbValue( pkgname.Name() ) ,
           DbValue( pkgname.Version() ) ,
           DbValue( pkgname.Arch() ) ,
           DbValue( pkgname.Build().Num() ) ,
           DbValue( pkgname.Build().Tag() ) ,
           DbValue( timestamp )
        };
        this->m_cmdpkg->Parameters().setValues(std::move(param_vals)) ;
        this->m_dbref.Execute(this->m_cmdpkg);
        return this->m_dbref.getLastInsertRowid() ;
      };


    auto store_dynlinked = [this](int64_t pkgid, const ElfFile& elf) -> int64_t {
      a4sqlt3::DbValues param_vals = {
         DbValue( pkgid ) ,
         DbValue( elf.getName().Str() ) ,
         DbValue( elf.getName().getDir() ) ,
         DbValue( elf.getName().getBase() ) ,
          (elf.soName().size()> 0 ?
              DbValue(elf.soName()) : DbValue(a4sqlt3::DbValueType::Null) ) ,
         DbValue( elf.getArch() )
      };
      this->m_cmddynlinked->setParameters(param_vals) ;
      this->m_dbref.Execute(this->m_cmddynlinked);
      return this->m_dbref.getLastInsertRowid() ;

    };

    auto store_needed = [this](int64_t dynlinked_id, const std::string& needed) -> void {
      this->m_cmdrequired->Parameters().setValues({DbValue(dynlinked_id), DbValue(needed)}) ;
      this->m_dbref.Execute(this->m_cmdrequired);
    };

    auto store_rrunpath = [this](int64_t dynlinked_id,
        const std::string& rrunpath, const std::string& dynlinked_homedir ) -> void {
      this->m_cmdrrunpath->Parameters().setValues(
          {DbValue(dynlinked_id), DbValue(rrunpath), DbValue(dynlinked_homedir)}) ;
      this->m_dbref.Execute(this->m_cmdrrunpath);
    };


    int64_t pkgid = store_pkg(pkgname, timestamp) ;
    for(const ElfFile& elf : dllist)
      {
        int64_t dynlinked_id = store_dynlinked(pkgid, elf) ;

        for( const std::string& needed : elf.getNeeded() )
          {
            store_needed(dynlinked_id, needed);
          }

        for( const std::string& rrunpaht : elf.getRRunPaths() )
          {
            store_rrunpath(dynlinked_id, rrunpaht, elf.getName().getDir());
          }
      }


  }//-----------------------------------------------------------------------------------------------

  void Remove(const std::string& fullname)
  {
    LogInfo()<< "clear " << fullname << std::endl;
    m_cmddelpkg->Parameters().Nr(1).set(fullname) ;
    m_cmddelpkg->Run();
  }

};






CacheDB::CacheDB( const std::string& name )
: a4sqlt3::Database( name )

{
  sql::register_own_functions(_sql3db);

  //if I do not have pkgs table, than I am new, or something is horrible wrong
  // later I can use select count from pkgs to find out ...
  // i think this could be a good idea

  // if i am new, than i need to create me and sett the new flag

  // if i am not new, check the version for possible updates ..

  // question , shall I take the new flag from extern,
  // could be found if file exist



}
//--------------------------------------------------------------------------------------------------


bool
CacheDB::Open()
{
  
  if(  a4sqlt3::Database::Open() )
    {

      checkVersion(
                sbbdep::MAJOR_VERSION,
                sbbdep::MINOR_VERSION ,
                sbbdep::PATCH_VERSION );

    }
  else
    return false;
  
  return true;
}
//--------------------------------------------------------------------------------------------------

bool
CacheDB::Create()
{

  LogInfo()<< "create cache " << _name << std::endl;

  if (a4sqlt3::Database::Create() )
    {
      try
        {
          Transaction transact(*this);
          Execute(CacheSQL::CreateSchemaSQL());

          transact.commit();
        }
      catch ( const a4sqlt3::Error& e )
        {
          LogError() << e << "\n ^ at creating schema\n";
          throw;
        }
      catch ( ... )
        {
          LogError() << "Unknown exception" << " at creating  schema\n";
          throw;
        }

      CacheSQL::register_own_sql_functions(_sql3db);

      // possible call first sync/creation here and create indexes later..

      try
        {
          Transaction transact(*this);
          Execute(CacheSQL::CreateIndexes());
          Execute(CacheSQL::CreateViews());
          transact.commit();
        }
      catch ( const a4sqlt3::Error& e )
        {
          LogError() << e << "\n ^ at creating indexes\n";
          throw;
        }
      catch ( ... )
        {
          LogError() << "Unknown exception " << " at creating indexes\n";
          throw;
        }

    }
  else
    return false;

  return true;
}
//--------------------------------------------------------------------------------------------------



void
CacheDB::checkVersion( int major, int minor, int patchlevel )
{
    {
      std::string sql = "select count(*) from sqlite_master where name='version';";

      a4sqlt3::DbValue rc = this->selectSingleValue(sql);

      if(rc.isNull())
        throw ErrToDo();



      if( rc.getInt64() != 1 )
        {
          if( rc.getInt64() > 1 )
            {
              LogError() << "more than one entry in version table, confused and can not continue\n";
              throw ErrToDo();
            }
          Execute(CacheSQL::CreateVersion(0, 1, 0)); // set default to 1, so update steps gi from one.
        }
    }

  // get db schema version from v
  auto calcMajorMinorVersion = [](int ma, int mi ) noexcept -> int
    {
      return (100000 + ma * 10000) + (1000 + mi * 100);
    };
  auto calcVersion = [](int ma, int mi, int pl ) noexcept -> int
    {
      return (100000 + ma * 10000) + (1000 + mi * 100) + pl;
    };
  using namespace a4sqlt3;
  auto getDbMajorMinorVersion = [calcMajorMinorVersion, this]() -> int
    {
      Dataset ds( {DbValueType::Int64, DbValueType::Int64} );
      this->Execute("SELECT major, minor FROM version", ds);
      return calcMajorMinorVersion(ds.getField(0).getInt64(), ds.getField(1).getInt64());
    };
  auto getDbVersion = [calcVersion, this]() -> int
    {
      Dataset ds({DbValueType::Int64, DbValueType::Int64, DbValueType::Int64});
      this->Execute("SELECT major, minor , patchlevel FROM version", ds);
      return calcVersion(ds.getField(0).getInt64(),
          ds.getField(1).getInt64(),ds.getField(2).getInt64());
    };

  if( calcVersion(major, minor, patchlevel) == getDbVersion() )
    return ;



  int app_version = calcVersion(major, minor, patchlevel);
  int db_version = getDbVersion();

  if( db_version < app_version )
    {
      if( getDbMajorMinorVersion() ==  calcMajorMinorVersion(0, 1) )
        {
          LogError() << "existing cache db was build with an old version of sbbdep.\n";
          LogError() << "please create a new cache by using the -c option or removing " <<
              _name << ".\n";
          LogError() << "Sorry for the inconvenience caused.\n\n" ;
          throw ErrGeneric("old db version in use");
        }

      auto trans = beginTransaction();
      if(db_version < calcVersion(0, 2, 1))
        {
          execute("CREATE TABLE keyvalstore (key  NOT NULL,  value  NOT NULL);");
          execute("create unique index  idx_keyvalstore_key on keyvalstore (key);");
          execute("insert into keyvalstore (key, value ) values ('ldsoconf', 0);");
        }


      std::string sqcmd =
          "UPDATE version set major = " + std::to_string(major) +
          ", minor = " + std::to_string(minor) +
          ", patchlevel = " + std::to_string(patchlevel) + ";" ;
      execute(sqcmd);
      trans.commit();
   }


#ifdef DEBUG
  if( calcVersion(major, minor, patchlevel) != getDbVersion() )
    throw ErrUnexpected("version update incorrect");
#endif

}
//--------------------------------------------------------------------------------------------------

void
CacheDB::updateData(const StringVec& toremove, const StringVec& toinsert)
{

  LDDirs lddirs;

  a4sqlt3::Dataset dsldtime ({ a4sqlt3::DbValueType::Int64 });
  Execute("SELECT value FROM keyvalstore WHERE key='ldsoconf';", dsldtime);

  if(dsldtime.getRowCount()!= 1)
    {
      Execute("INSERT INTO keyvalstore (key, value) VALUES ('ldsoconf', 0);");
      dsldtime.reset();
      Execute("SELECT value FROM keyvalstore WHERE key='ldsoconf';", dsldtime);

      if(dsldtime.getRowCount()!= 1)
        throw ErrGeneric("keyval ldsoconf count != 1");

    }


  if(lddirs.getLdSoConfTime() > dsldtime.at(0).getInt64())
      {
        Transaction trans(*this);
        updateLdDirOnly(  StringVec(lddirs.getLdDirs().begin(), lddirs.getLdDirs().end()) ) ;
        trans.commit();
      }

  if(toremove.size()== 0 && toinsert.size()==0)
    {
      LogInfo() << "cache is up to date "<< std::endl;
      return ;
    }


  LogInfo() << "apply changes "<< std::endl;

// todo this needs only to be done if toinsert has data, if we just remove, this could be ignored

  // TODO ich brauch das sowieso, fertig
  // daher sollt das immer sein waerend man die aenderungen sucht
  std::thread loadlddirs( [&lddirs] (){
    lddirs.getLdDirs();
    lddirs.getLdLnkDirs();
  });




  struct ToStoreData
  {
    std::mutex mtx;
    std::condition_variable condition;
    bool newdata{false} ;

    std::atomic_bool running{false};

    std::vector<Pkg> pkgs;


    void add(const Pkg pkg) {
      std::unique_lock<std::mutex> lock(mtx);
      pkgs.push_back(pkg);
      newdata = true;
      condition.notify_one();
    }

  };

  auto persiter =[this](ToStoreData& tostore, DbAction& dbaction){

    while ( tostore.running.load(std::memory_order_relaxed) )
      {
        std::unique_lock<std::mutex> lock(tostore.mtx);
        while (not tostore.newdata)
          tostore.condition.wait( lock );

        std::vector<Pkg> pkgs;
        std::swap(pkgs, tostore.pkgs);
        tostore.newdata = false;
        lock.unlock();

        for(Pkg& pkg : pkgs)
          {
            dbaction.Store(
                PkgName(pkg.getPath().getBase()),
                pkg.getElfFiles(),
                pkg.getPath().getLastModificationTime()) ;
          }
      }

      // check if something left to do
      { // here no lock should be required  cause if monitor is not running, no producers..
        std::vector<Pkg> pkgs;
        std::swap(pkgs, tostore.pkgs);
        for(Pkg& pkg : pkgs)
          {
            dbaction.Store(
                PkgName(pkg.getPath().getBase()),
                pkg.getElfFiles(),
                pkg.getPath().getLastModificationTime()) ;
          }
      }
  };


  class ToIndexData{
    std::mutex mtx;
    StringVec::const_iterator current;
    StringVec::const_iterator endpos;

  public:
    ToIndexData(const StringVec& data): current(data.begin()), endpos(data.end()) {}

    // as long as string not empty we can do something
    std::string pop(){

      std::unique_lock<std::mutex> lock(mtx);

      if( current == endpos )
        return std::string();

      std::string retval(*current);
      ++current;
      return retval;
    }
  };

  auto indexer = [this](ToIndexData& toindex, ToStoreData& tostore){

    for(;;)
      {
        std::string pkgpath = toindex.pop();

        if(pkgpath.empty())
          break;

        Path path(pkgpath);
        Pkg pkg  = Pkg::create( path );
        LogInfo()<< "index " << path.getBase() << std::endl;

        if (pkg.getType() != PkgType::Installed || not pkg.Load() )
          continue;

        tostore.add(pkg);

      }

  };


  DbAction dbaction(*this);
  Transaction transaction(*this);

  std::thread delwork;
  if(toremove.size() > 0)
    delwork = std::thread([&dbaction, &toremove](){
      for(auto& val : toremove)
        dbaction.Remove(val);
      });


  ToStoreData tostore;
  tostore.running.store(true,std::memory_order_relaxed) ;

  // for indexing I need full path, so do this
  StringVec fullnames = toinsert;
  PkgAdmDir pkg_adm_dir;
  auto makefullpathname = [&pkg_adm_dir](std::string& s) ->std::string
      { return pkg_adm_dir.getDirName() + "/" + s ; } ;

  std::transform(fullnames.begin(), fullnames.end(),
       std::begin(fullnames), makefullpathname );

  ToIndexData toindex(fullnames);

  auto waitfor = [](std::thread& th){ if(th.joinable()) th.join(); };

  std::thread indexer1(indexer , std::ref(toindex), std::ref(tostore) );
  std::thread indexer2(indexer,  std::ref(toindex), std::ref(tostore) );

  waitfor(delwork);
  // important to wait here, reinstalled could have same name so must be deleted first

  std::thread storework(persiter, std::ref(tostore), std::ref(dbaction));



  waitfor(indexer1);
  waitfor(indexer2);


  {
    std::unique_lock<std::mutex> monitorlock(tostore.mtx);
    tostore.newdata = true;
    tostore.condition.notify_one();
    tostore.running.store(false, std::memory_order_relaxed);  // might possible be a release cause publishing
    tostore.condition.notify_one(); // just to be sure to not run into troubles
  }

  waitfor(storework);


  LogInfo() << "wait for resolving symbolic links"<< std::endl;
  waitfor(loadlddirs);


  updateLdDirs(StringVec(lddirs.getLdDirs().begin(), lddirs.getLdDirs().end()) ,
        StringVec(lddirs.getLdLnkDirs().begin(), lddirs.getLdLnkDirs().end())
  );



  LogInfo() << "persist new information to disk\n";

  transaction.commit();

  LogInfo() << "run db analyzer\n";
  Execute("ANALYZE ;");

  m_isNew = false ;

}
//--------------------------------------------------------------------------------------------------

void
CacheDB::updateLdDirOnly(const StringVec& lddirs)
{
  using namespace a4sqlt3;


  SqlCommand* cmdlddir = getCommand("cmdlddir");
  if(not cmdlddir){
      cmdlddir = createStoredCommand("cmdlddir",
          CacheSQL::InsertLdDirSQL(),{DbValueType::Text});
  }

  Execute("DELETE FROM lddirs;");

  for(const std::string val : lddirs)
    {
      cmdlddir->Parameters().Nr(1).set(val);
      Execute(cmdlddir);
    }

  persistLdSoTime();

}
//--------------------------------------------------------------------------------------------------

void
CacheDB::persistLdSoTime()
{
  LDDirs lddirs; // TODO cache sql
  execute("update keyvalstore set value = " +
      std::to_string(lddirs.getLdSoConfTime()) +
      " where key = 'ldsoconf' ;") ;
  // TODO das wird ein command,

}
//--------------------------------------------------------------------------------------------------

void
CacheDB::updateLdDirs(const StringVec& lddirs, const StringVec& ldlinkdirs)
{
  using namespace a4sqlt3;


  SqlCommand* cmdlddir = getCommand("cmdlddir");
  if(not cmdlddir){
      cmdlddir = createStoredCommand("cmdlddir",
          CacheSQL::InsertLdDirSQL(),{DbValueType::Text});
  }

  SqlCommand* cmdldlnkdir = getCommand("cmdldlnkdir");
  if(not cmdldlnkdir){
      cmdldlnkdir = createStoredCommand("cmdldlnkdir",
          CacheSQL::InsertLdLnkDirSQL(), {DbValueType::Text});
  }


  Execute("DELETE FROM lddirs;");
  Execute("DELETE FROM ldlnkdirs;");


  for(const std::string val : lddirs)
    {
      cmdlddir->Parameters().Nr(1).set(val);
      Execute(cmdlddir);
    }

  for(const std::string val : ldlinkdirs)
    {
      cmdldlnkdir->Parameters().Nr(1).set(val);
      Execute(cmdldlnkdir);
    }

  persistLdSoTime();
}
//--------------------------------------------------------------------------------------------------

int64_t
CacheDB::getLatestPkgTimeStamp()
{

  a4sqlt3::DbValue val = selectValue(sql::maxPkgTimeStamp());

  return val.isNull() ? 0 : val.getInt64() ;

}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
} // ns
