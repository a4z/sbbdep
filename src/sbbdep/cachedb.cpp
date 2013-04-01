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


#include <sbbdep/cachedb.hpp>
#include <sbbdep/cachesql.hpp>
#include <sbbdep/path.hpp>
#include <sbbdep/log.hpp>
#include <sbbdep/pkg.hpp>
#include <sbbdep/pkgname.hpp>
#include <sbbdep/config.hpp> // generated
#include <sbbdep/cachecmds.hpp>

#include <a4sqlt3/sqlcommand.hpp>
#include <a4sqlt3/dataset.hpp>
#include <a4sqlt3/onevalresult.hpp>
#include <a4sqlt3/error.hpp>

#include <a4z/err.hpp>

#include <cstdlib>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>

namespace sbbdep {


class DbAction
{

  InsertPkg m_cmdpkg;
  InsertDynLinked m_cmddynlinked;
  InsertRequired m_cmdrequired;
  InsertRRunPath m_cmdrrunpath;
  DeletePkgByFullName m_cmddelpkg;
  CacheDB& m_dbref;


public:
  DbAction( CacheDB& dbref ) :
    m_cmdpkg(), m_cmddynlinked(), m_cmdrequired() , m_cmddelpkg(), m_dbref(dbref)
  {
    m_dbref.CompileCommand(&m_cmdpkg);
    m_dbref.CompileCommand(&m_cmddynlinked);
    m_dbref.CompileCommand(&m_cmdrequired);
    m_dbref.CompileCommand(&m_cmdrrunpath);
    m_dbref.CompileCommand(&m_cmddelpkg);
  }//-----------------------------------------------------------------------------------------------



  void
  Store( const PkgName& pkgname, const Pkg::DynLinkedFiles& dllist, const int64_t& timestamp)
  {
    using a4sqlt3::DbValue ;

    LogInfo()<< "store " << pkgname.FullName() << std::endl;

    // what's going on here:
    // store the pkg,
    // to the pkg store all dynlinked infos
    // to each dynlinked info store Needed list and RunRPaths info

    // instead of capture this and writing always this->bla , for now ....
    InsertPkg& m_cmdpkg = this->m_cmdpkg;
    InsertDynLinked& m_cmddynlinked = this->m_cmddynlinked;
    InsertRequired& m_cmdrequired = this->m_cmdrequired;
    InsertRRunPath& m_cmdrrunpath = this->m_cmdrrunpath;
    CacheDB& m_dbref = this->m_dbref;


    auto store_pkg = [&m_dbref, &m_cmdpkg](const PkgName& pkgname, const int64_t& timestamp) -> int64_t {
        a4sqlt3::DbValueList param_vals = {
           DbValue( pkgname.FullName() ) ,
           DbValue( pkgname.Name() ) ,
           DbValue( pkgname.Version() ) ,
           DbValue( pkgname.Arch() ) ,
           DbValue( pkgname.Build().Num() ) ,
           DbValue( pkgname.Build().Tag() ) ,
           DbValue( timestamp )
        };
        m_cmdpkg.Parameters().setValues(std::move(param_vals)) ;
        m_dbref.Execute(&m_cmdpkg);
        return m_dbref.getLastInsertRowid() ;
      };


    auto store_dynlinked = [&m_dbref, &m_cmddynlinked](int64_t pkgid, const ElfFile& elf) -> int64_t {
      a4sqlt3::DbValueList param_vals = {
         DbValue( pkgid ) ,
         DbValue( elf.getName().Str() ) ,
         DbValue( elf.getName().getDir() ) ,
         DbValue( elf.getName().getBase() ) ,
          (elf.soName().size()> 0 ? DbValue(elf.soName()) : DbValue(a4sqlt3::DbValueType::Null) ) ,
         DbValue( elf.getArch() )
      };
      m_cmddynlinked.Parameters().setValues(std::move(param_vals)) ;;
      m_dbref.Execute(&m_cmddynlinked);
      return m_dbref.getLastInsertRowid() ;

    };

    auto store_needed = [&m_dbref, &m_cmdrequired](int64_t dynlinked_id, const std::string& needed) -> void {
      m_cmdrequired.Parameters().setValues({DbValue(dynlinked_id), DbValue(needed)}) ;
      m_dbref.Execute(&m_cmdrequired);
    };

    auto store_rrunpath = [&m_dbref, &m_cmdrrunpath](int64_t dynlinked_id,
        const std::string& rrunpath, const std::string& dynlinked_homedir ) -> void {
      m_cmdrrunpath.Parameters().setValues(
          {DbValue(dynlinked_id), DbValue(rrunpath), DbValue(dynlinked_homedir)}) ;
      m_dbref.Execute(&m_cmdrrunpath);
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
    LogInfo()<< "delete " << fullname << std::endl;
    m_cmddelpkg.setFullName(fullname) ;
    m_cmddelpkg.Run();
  }

};







CacheDB::CacheDB()
: a4sqlt3::Database(std::string(std::getenv("HOME")) + std::string("/sbbdep.cache")) //TODO wenn test vorbei nach .config/.. oder sonst wo
{
 
  Path p( m_name );
  m_isNew = !p.isValid();

}
//--------------------------------------------------------------------------------------------------

CacheDB::CacheDB( const std::string& name )
: a4sqlt3::Database( name )
// , m_isNew( !Path(name).isValid() ) better readable in body
{

  Path p( m_name );
  m_isNew = !p.isValid();

}
//--------------------------------------------------------------------------------------------------



CacheDB::~CacheDB()
{
 
}
//--------------------------------------------------------------------------------------------------

bool
CacheDB::Open()
{
  
  if(  a4sqlt3::Database::Open() )
    {
      CacheSQL::register_own_sql_functions(m_sql3db);

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

  LogInfo()<< "create cache " << m_name << std::endl;

  if (a4sqlt3::Database::Create() )
    {
      try
        {
          Transaction transact(*this);
          Execute(CacheSQL::CreateSchemaSQL());
          transact.commit();
        }
      catch ( const a4z::Err& e )
        {
          LogError() << e << "\n ^ at creating db schema\n";
          A4Z_THROW_NESTED("");
        }

      CacheSQL::register_own_sql_functions(m_sql3db);

      // call sync here ..

      try
        {
          Transaction transact(*this);
          Execute(CacheSQL::CreateIndexes());
          transact.commit();
        }
      catch ( const a4z::Err& e )
        {
          LogError() << e << "\n ^ at creating db indexes\n";
          A4Z_THROW_NESTED("");
        }

      m_isNew = false ;
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
      a4sqlt3::OneValResult<int> rc;
      Execute(sql, &rc);
      if(!rc.isValid())
        throw a4z::ErrorTodo();

      if( rc.Val() != 1 )
        {
          if( rc.Val() > 1 )
            {
              LogError() << "more than one entry in version table, confused and can not continue\n";
              throw a4z::ErrorTodo();
            }
          Execute(CacheSQL::CreateVersion(0, 1, 0)); // set default to 1, so update steps gi from one.
        }
    }

  // get db schema version from v
  auto calcDbVersion = [](int ma, int mi ) noexcept -> int
    {
      return (100000 + ma * 10000) + (1000 + mi * 100);
    };
  auto calcFullVersion = [](int ma, int mi, int pl ) noexcept -> int
    {
      return (100000 + ma * 10000) + (1000 + mi * 100) + pl;
    };
  using namespace a4sqlt3;
  auto getDbVersion = [calcDbVersion, this]() -> int
    {
      Dataset ds( {DbValueType::Int, DbValueType::Int} );
      this->Execute("SELECT major, minor FROM version", &ds);
      return calcDbVersion(ds.getField(0).getInt(), ds.getField(1).getInt());
    };
  auto getDbAppVersion = [calcFullVersion, this]() -> int
    {
      Dataset ds({DbValueType::Int, DbValueType::Int, DbValueType::Int});
      this->Execute("SELECT major, minor , patchlevel FROM version", &ds);
      return calcFullVersion(ds.getField(0).getInt(),
          ds.getField(1).getInt(),ds.getField(2).getInt());
    };

  if( calcFullVersion(major, minor, patchlevel) == getDbAppVersion() )
    return ;



  int app_dbversion = calcDbVersion(major, minor);
  int db_dbversion = getDbVersion();

  while( db_dbversion < app_dbversion )
    {
      if( db_dbversion ==  calcDbVersion(0, 1) )
        {
          LogInfo()<< "update db schema form 0.1 to 0.2" << std::endl;
          Transaction transaction(*this);
          Execute("ALTER TABLE rrunpath ADD COLUMN lddir TEXT;");
          std::string sql="update rrunpath "
              "set lddir = mkRealPath("
              "replaceOrigin(ldpath,(SELECT dirname FROM dynlinked WHERE id=dynlinked_id))"
              ") ;" ;
          Execute(sql);

          std::string idxnew =
              "create index  idx_pkgs_fullname on pkgs(fullname); "
              "create index  idx_dynlinked_pkg_id on dynlinked(pkg_id);  "
              "create index  idx_required_dynlinked_id on required(dynlinked_id); "
              "create index  idx_rrunpath_dynlinked_id on rrunpath(dynlinked_id); ";
          Execute(idxnew);
          Execute("UPDATE version set major=0, minor=2, patchlevel=0;");
          transaction.commit();
        }
//      // for the future
//      else if( db_dbversion ==  calcDbVersion(0, 2) )
//        {
//          // update form 0.2.x to 0.3.x
//        }
      else
        throw a4z::ErrorTodo("Cache version update failed in compare version number");


      db_dbversion = getDbVersion();
   }


  if( calcFullVersion(major, minor, patchlevel) > getDbAppVersion() )
    {
      std::string sqcmd = "UPDATE version set patchlevel=" + std::to_string(patchlevel) + ";" ;
      Execute(sqcmd);
    }

#ifdef DEBUG
  if( calcFullVersion(major, minor, patchlevel) != getDbAppVersion() )
    throw a4z::ErrorNeverReach("version update incorrect");
#endif

}
//--------------------------------------------------------------------------------------------------

void
CacheDB::updateData(const std::vector<std::string>& toremove, const std::vector<std::string>& toinsert)
{

  struct MqMonitor
  {
    std::mutex mtx;
    std::condition_variable condition;

    std::atomic_bool running{false};
    std::atomic_bool newdata{false} ;
  };

  struct MqData
  { // must be protected via mqmonitor
    std::vector<Pkg> pkgs;
  };

  auto persiter =[this](MqMonitor& monitor, MqData& mqdata, DbAction& dbaction){

    while ( monitor.running )
      {
        std::unique_lock<std::mutex> lock(monitor.mtx);
        while (not monitor.newdata)
            monitor.condition.wait( lock );

        std::vector<Pkg> pkgs;
        std::swap(pkgs, mqdata.pkgs);
        monitor.newdata = false;
        lock.unlock();

        for(Pkg& pkg : pkgs)
          {
            dbaction.Store(
                PkgName(pkg.getPath().getBase()),
                pkg.getDynLinked(),
                pkg.getPath().getLastModificationTime()) ;
          }
      }

      // check if something left to do
  };

  struct WorkerData
  {
    std::mutex mtx;
    std::queue<std::string> pkgnames;
  };

  auto indexer = [this](WorkerData& wkdata, MqData& mqdata, MqMonitor& monitor){

    for(;;)
      {
        std::unique_lock<std::mutex> lock(wkdata.mtx);
        if(wkdata.pkgnames.size()==0)
          break;

        std::string pkgpath = wkdata.pkgnames.front();
            wkdata.pkgnames.pop();
        lock.unlock();

        Path path(pkgpath);
        Pkg pkg  = Pkg::create( path );
        LogInfo()<< "index " << path.getBase() << std::endl;

        if (pkg.getType() != PkgType::Installed || not pkg.Load() )
          continue;

        std::unique_lock<std::mutex> monitorlock(monitor.mtx);
        mqdata.pkgs.push_back(pkg);
        monitor.newdata = true;
        monitor.condition.notify_one();

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


  MqMonitor monitor;
  monitor.running = true ;
  MqData mqdata;
  WorkerData wkdata;
  for(auto& val : toinsert) wkdata.pkgnames.emplace(val);

  std::thread indexer1(indexer , std::ref(wkdata), std::ref(mqdata), std::ref(monitor) );
  std::thread indexer2(indexer,  std::ref(wkdata), std::ref(mqdata), std::ref(monitor) );

  if(delwork.joinable())
    delwork.join();

  std::thread storework(persiter, std::ref(monitor), std::ref(mqdata), std::ref(dbaction));

  indexer1.join();
  indexer2.join();


  {
    std::unique_lock<std::mutex> monitorlock(monitor.mtx);
    monitor.newdata = true;
    monitor.condition.notify_one();
    monitor.running = false;
  }

  storework.join();

  transaction.commit();

}
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
} // ns
