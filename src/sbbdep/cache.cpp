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


#include <sbbdep/cache.hpp>
#include <sbbdep/cachesql.hpp>
#include <sbbdep/dircontent.hpp>
#include <sbbdep/path.hpp>
#include <sbbdep/pathname.hpp>
#include <sbbdep/pkgname.hpp>
#include <sbbdep/pkg.hpp>
#include <sbbdep/error.hpp>
#include <sbbdep/stringlist.hpp>

#include <sbbdep/lddirs.hpp>
#include <sbbdep/cachecmds.hpp>
#include <sbbdep/pkgadmdir.hpp>
#include <sbbdep/log.hpp>

#include <a4sqlt3/sqlcommand.hpp>
#include <a4sqlt3/error.hpp>
#include <a4sqlt3/onevalresult.hpp>
#include <a4sqlt3/dataset.hpp>
#include <a4sqlt3/error.hpp>

#include <list>
#include <vector>
#include <set>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <iostream>



namespace sbbdep {

namespace {
// some helpers
//--------------------------------------------------------------------------------------------------

using namespace sbbdep;

struct Transaction{
  CacheDB& m_db;   bool m_commited;
  Transaction(CacheDB& db): m_db(db), m_commited(false){m_db.Execute("BEGIN TRANSACTION");}
  ~Transaction(){ if(!m_commited) m_db.Execute("ROLLBACK TRANSACTION");}
  void commit(){ m_db.Execute("COMMIT TRANSACTION"); m_commited = true ; }
};


struct StoreEntry
{

  PkgName m_pkname;
  Pkg::DynLinkedFiles m_dllist;
  int64_t m_timestamp; 

  StoreEntry( const PkgName& pkname, const Pkg::DynLinkedFiles& dllist, const int64_t&timestamp )
  :  m_pkname(pkname), m_dllist(dllist) , m_timestamp(timestamp)
  {
  }//-----------------------------------------------------------------------------------------------  
  

};
//--------------------------------------------------------------------------------------------------


typedef std::list< StoreEntry* > StoreEntryList; //used more than once 


class TmpStore
{
  
  InsertPkg m_cmdpkg;
  InsertDynLinked m_cmddynlinked;
  InsertRequired m_cmdrequired;
  InsertRRunPath m_cmdrrunpath;
  CacheDB& m_dbref;

  // need to call set<paramtype> once, then use of setValue is faster..  
  
  StoreEntryList m_entrylist;

public:
  TmpStore( CacheDB& dbref ) :
    m_cmdpkg(), m_cmddynlinked(), m_cmdrequired() ,m_dbref(dbref)
  {
    m_dbref.CompileCommand(&m_cmdpkg);
    m_dbref.CompileCommand(&m_cmddynlinked);
    m_dbref.CompileCommand(&m_cmdrequired);
    m_dbref.CompileCommand(&m_cmdrrunpath);
  }//-----------------------------------------------------------------------------------------------

  ~TmpStore()
  {
    //m_cmdpkg.Release(); .. and others happens automatic in d'tor..
    for (StoreEntryList::iterator pos = m_entrylist.begin(); pos != m_entrylist.end(); ++pos)
      {
        delete (*pos);
      }
  }//-----------------------------------------------------------------------------------------------

/*  
  void
  addEntry( StoreEntry* const & entry ) // for the code analyser to ger rid of the semantic error ...
  {
#pragma omp critical (tmpStoreAddEntry)
      {
        m_entrylist.push_back(entry);
      }
  }//-----------------------------------------------------------------------------------------------
*/
  
  void
  addEntries( StoreEntryList& entries ) // for my collector pattern, lock it here...
  {
#pragma omp critical (tmpStoreAddEntry)
      {
        m_entrylist.insert(m_entrylist.end(), entries.begin(), entries.end());
      }
  }//-----------------------------------------------------------------------------------------------  
  
  void
  Persist()
  {
    //transaction must be done by the caller! 

    //m_dbref.Execute("BEGIN TRANSACTION");
    for (StoreEntryList::iterator pos = m_entrylist.begin(); pos != m_entrylist.end(); ++pos)
      {
        try
          {
            Persist((*pos)->m_pkname, (*pos)->m_dllist , (*pos)->m_timestamp);
          }
        catch ( const a4z::Err& e )
          {
            LogError() << e << "\n";
          }
        
      }
    //m_dbref.Execute("COMMIT;");

  }//-----------------------------------------------------------------------------------------------
  
  
  
  void
  Persist( PkgName& pkgname, Pkg::DynLinkedFiles& dllist, int64_t& timestamp)
  {
    using a4sqlt3::DbValue ;

    // what's going on here:
    // store the pkg,
    // to the pkg store all dynlinked infos
    // to each dynlinked info store Needed list and RunRPaths info

    InsertPkg& m_cmdpkg = this->m_cmdpkg;
    InsertDynLinked& m_cmddynlinked = this->m_cmddynlinked;
    InsertRequired& m_cmdrequired = this->m_cmdrequired;
    InsertRRunPath& m_cmdrrunpath = this->m_cmdrrunpath;
    CacheDB& m_dbref = this->m_dbref;


    auto store_pkg = [&m_dbref, &m_cmdpkg](PkgName& pkgname, int64_t& timestamp) -> int64_t {
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

};

// for first private collect / write back..
class StoreEntryCollector
{
  
  TmpStore& m_tmpstore;
  StoreEntryList m_list;
public:
  StoreEntryCollector( TmpStore& tmpstore ) :
    m_tmpstore(tmpstore)
  {
  }
  ~StoreEntryCollector()
  {
      m_tmpstore.addEntries(m_list);
  }
  
  void   
  collect( StoreEntry* const & se ) // for the code analyser to ger rid of the semantic error ...
  {
    m_list.push_back(se);
  }
  
};

}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------  


//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------


Cache::Cache( const std::string& dbname ) :
  m_db(dbname)
{
  
  m_isnew = false;
  
  try
    {
      m_db.Open();
    }
  catch ( const a4sqlt3::SQLite3Error& e )
    {
      
      try
        {
          m_db.Create();
          m_db.Close();
          m_db.Open(); 
          CreateSchema();
          
          m_isnew = true;
        }
      catch ( const a4sqlt3::SQLite3Error& e )
        {
          LogError() << e << "\n" ;
          A4Z_THROW_NESTED("");
        }
      catch ( const a4z::Err& e )
        {
          A4Z_THROW_NESTED("");
        }
      
    }
  
}
//--------------------------------------------------------------------------------------------------


Cache::~Cache()
{
  
}
//--------------------------------------------------------------------------------------------------


void
Cache::checkVersion( int major, int minor, int patchlevel )
{
    {
      std::string sql = "select count(*) from sqlite_master where name='version';";
      a4sqlt3::OneValResult<int> rc;
      m_db.Execute(sql, &rc);
      if(!rc.isValid())
        throw a4z::ErrorTodo();

      if( rc.Val() != 1 )
        {
          if( rc.Val() > 1 )
            {
              LogError() << "more than one entry in version table, confused and can not continue\n";
              throw a4z::ErrorTodo();
            }
          m_db.Execute(CacheSQL::CreateVersion(0, 1, 0)); // set default to 1, so update steps gi from one.
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
  auto getDbVersion = [calcDbVersion](CacheDB& db) -> int
    {
      Dataset ds( {DbValueType::Int, DbValueType::Int} );
      db.Execute("SELECT major, minor FROM version", &ds);
      return calcDbVersion(ds.getField(0).getInt(), ds.getField(1).getInt());
    };
  auto getDbAppVersion = [calcFullVersion](CacheDB& db) -> int
    {
      Dataset ds({DbValueType::Int, DbValueType::Int, DbValueType::Int});
      db.Execute("SELECT major, minor , patchlevel FROM version", &ds);
      return calcFullVersion(ds.getField(0).getInt(),
          ds.getField(1).getInt(),ds.getField(2).getInt());
    };


  if( calcFullVersion(major, minor, patchlevel) == getDbAppVersion(m_db) )
    return ;


  int app_dbversion = calcDbVersion(major, minor);
  int db_dbversion = getDbVersion(m_db);

  while( db_dbversion < app_dbversion )
    {
      if( db_dbversion ==  calcDbVersion(0, 1) )
        {
          LogInfo()<< "updateing db schema form 0.1 to 0.2" << std::endl;
          Transaction transaction(m_db);
          m_db.Execute("ALTER TABLE rrunpath ADD COLUMN lddir TEXT;");
          std::string sql="update rrunpath "
              "set lddir = mkRealPath("
              "replaceOrigin(ldpath,(SELECT dirname FROM dynlinked WHERE id=dynlinked_id))"
              ") ;" ;
          m_db.Execute(sql);
          m_db.Execute("UPDATE version set major=0, minor=2, patchlevel=0;");
          transaction.commit();
        }
//      // for the future
//      else if( db_dbversion ==  calcDbVersion(0, 2) )
//        {
//          // update form 0.2.x to 0.3.x
//        }
      else
        throw a4z::ErrorTodo("Cache version update failed in compare version number");


      db_dbversion = getDbVersion(m_db);
   }


  if( calcFullVersion(major, minor, patchlevel) > getDbAppVersion(m_db) )
    {
      std::stringstream ss;
      ss << "UPDATE version set patchlevel=" << patchlevel << ";" ;
      m_db.Execute(ss.str());
    }

#ifdef DEBUG
  if( calcFullVersion(major, minor, patchlevel) != getDbAppVersion(m_db) )
    throw a4z::ErrorNeverReach("version update incorrect");
#endif

}
//--------------------------------------------------------------------------------------------------

void
Cache::doSync()
{
  
  if (m_isnew)
    {
      Log::Info() << "create cache (" << m_db.Name() <<")" << std::endl;
      CreateData();
      // create indexes after first data
      CreateIndexes() ; 
    }
  else
    {
      Log::Info() << "sync cache (" << m_db.Name() <<")" << std::endl;
      SyncData();
    }
 
  /*
   * not to me, currently there are 3 transactions
   * on inserting new pkgs, whenn called PersistPgks()
   * remove pkgs what is currently withing SyncData() 
   * and the lddir creation is also a transaction
   * TODO clean this up a little bit , 
   * eg stripe out deleting into an own funciton ....
   */
  
  
}
//--------------------------------------------------------------------------------------------------

void
Cache::CreateSchema()
{
  
  try
    {
      m_db.Execute("BEGIN TRANSACTION;");
      
      m_db.Execute(CacheSQL::CreateSchemaSQL());
      
      m_db.Execute("COMMIT;");
    }
  catch ( const a4sqlt3::SQLite3Error& e )
    {
      LogError() << e << "\n" << CacheSQL::CreateSchemaSQL() << "\n";
      
      m_db.Execute("ROLLBACK;");
      throw;
    }
  catch ( const a4z::Err& e )
    {
      m_db.Execute("ROLLBACK;");
      A4Z_THROW_NESTED("");
    }
  
}
//--------------------------------------------------------------------------------------------------


void
Cache::CreateIndexes()
{
  
  try
    {
      m_db.Execute("BEGIN TRANSACTION;");
      
      m_db.Execute(CacheSQL::CreateIndexes());
      
      m_db.Execute("COMMIT;");
    }
  catch ( const a4sqlt3::SQLite3Error& e )
    {
      LogError() << e << "\n" << CacheSQL::CreateSchemaSQL() << "\n";
      
      m_db.Execute("ROLLBACK;");
      throw;
    }
  catch ( const a4z::Err& e )
    {
      m_db.Execute("ROLLBACK;");
      A4Z_THROW_NESTED("");
    }
  
}
//--------------------------------------------------------------------------------------------------


void
Cache::CreateData()
{
  
  StringVec pkgnamevec;

  PkgAdmDir pkg_adm_dir;
  auto newfiles_cb = [&](const std::string& d,const std::string&& f) -> bool {
    pkgnamevec.push_back(d +"/"+ f);
    return true ;
  };
  pkg_adm_dir.apply(newfiles_cb) ;



  // TODO , think about how to use such a scope guard in future...
  // TODO "PRAGMA journal_mode = OFF" and "PRAGMA synchronous = OFF"
  // for first creation I could turn all off, put this on research ....
  Transaction transaction(m_db);

  PersistPgks( pkgnamevec , false) ;
  UpdateLdDirs(false) ;

  transaction.commit();

  // TODO , if this throws, I will have an empty db, think about that

  m_isnew = false;
  
}
//--------------------------------------------------------------------------------------------------


void
Cache::SyncData()
{
  // get diff from filesystem and db, remove old stuff from db and insert new 
  // get what is not present in fs but in db for remove
  // get what is not present in db but in fs for insert
  // get files with newer date and existing name for handling reinstalled pkgs
  

  StringSet allpkgfiles; // all pks in file system
  StringSet allpkgindb; // all pks in the db
  StringSet newpkgs; // // all pks in file system with a newer date as the lastest known date


  // get latest known file time for a package to compare later
  a4sqlt3::OneValResult< int64_t > MaxTimeStampRH;
  m_db.Execute(CacheSQL::MaxPkgTimeStamp(), &MaxTimeStampRH);
  const time_t maxknownftime = MaxTimeStampRH.Val() ;

  // var/adm/packages file dir for iteratong through ..
  PkgAdmDir pkg_adm_dir;

#pragma omp parallel sections
    {

#pragma omp section
        {
          auto newfiles_cb =
              [&allpkgfiles, &newpkgs, &maxknownftime]
               (const std::string& d,const std::string&& f) -> bool
              {
                Path path(d + "/" + f); //
                if ( path.isRegularFile() )
                  {
                    allpkgfiles.insert(f); // insert filename in all existing file_pkgs,
                    // and if newer as the latest known file time
                    if ( path.getLastModificationTime() > maxknownftime )
                          newpkgs.insert(f); // insert into new pkgs
                  }
                return true ;
              };

          pkg_adm_dir.apply(newfiles_cb) ;

        }// opm section
#pragma omp section
        {
          // insert existing names into all_pkgs;
          auto rh = [&allpkgindb](a4sqlt3::Columns& cols) -> bool
              {
                allpkgindb.insert(cols[0].get<std::string>());
                return true ;
              } ;

          m_db.Execute("SELECT fullname FROM pkgs;", rh);
        } // opm section
    } //omp parallel sections
    

  StringList toremoveList;
  StringList toinsertList;
  StringList reinstalledList;

#pragma omp parallel sections
    {
#pragma omp section
        {
          // all that are in db but not in filesystem are to remove
          std::set_difference(allpkgindb.begin(), allpkgindb.end(),
              allpkgfiles.begin(), allpkgfiles.end(),
              std::inserter(toremoveList, toremoveList.begin()));
        }

#pragma omp  section
        {
          // all that are in filesystem but not in db are new and need to be added to insert list
          std::set_difference(allpkgfiles.begin(), allpkgfiles.end(),
              allpkgindb.begin(), allpkgindb.end(),
              std::inserter(toinsertList, toinsertList.begin()));

          // new pkgs that are not in the insert list are re-installed and need to be added again
          std::set_difference(newpkgs.begin(), newpkgs.end(), toinsertList.begin(),
              toinsertList.end(), std::inserter(reinstalledList, reinstalledList.begin()));
          
        }

    } //omp sections

  
  // for indexing full path is needed, so create a list of all new pkgs with the full path
  StringVec allinserts;
  auto fullpathname = [&pkg_adm_dir](std::string& s) ->std::string
      { return pkg_adm_dir.getDirName() + "/" +s ; } ;

  // add the new ones
  std::transform(toinsertList.begin(), toinsertList.end(),
      std::back_insert_iterator<StringVec>(allinserts), fullpathname );
  // add the reinstalled ones to all, they need to be re indexed
  std::transform(reinstalledList.begin(), reinstalledList.end(),
      std::back_insert_iterator<StringVec>(allinserts), fullpathname );


  m_db.Execute("BEGIN TRANSACTION");

  if(toremoveList.size()> 0)
    DeletePgks(toremoveList, false) ;

  if(reinstalledList.size()> 0)
    DeletePgks(reinstalledList, false) ;

  if(allinserts.size() > 0 )
    PersistPgks(allinserts, false) ;

  UpdateLdDirs(false);

  m_db.Execute("COMMIT TRANSACTION");

  //--------------------
  // from here only info generation about what happened within sync..
  //--------------------
  typedef std::map<std::string, std::string> MessageMap;
  MessageMap messageMap;
  for(const std::string& inserted : toinsertList)
    {
      PkgName inname(inserted) ; // need to compare just the  pkg name, without version tag ,..

      auto compair = [&inname](const std::string& fullname)->bool {
        return inname.Name() == PkgName(fullname).Name();
      };
      auto toremoveIter = std::find_if(toremoveList.begin(), toremoveList.end(), compair) ;
      
      if ( toremoveIter != toremoveList.end() )
        {
          messageMap.insert( MessageMap::value_type(inserted , " => update from " + *toremoveIter));
          toremoveList.erase(toremoveIter) ;
        }
      else
        {
          messageMap.insert( MessageMap::value_type(inserted , " => installed " ) ) ;
        }      

    }  
  

  for (StringList::iterator pos = toremoveList.begin(); pos != toremoveList.end(); ++pos)
    {
      messageMap.insert( MessageMap::value_type(*pos , " => removed " ) ) ;
    }
  for (StringList::iterator pos = reinstalledList.begin(); pos != reinstalledList.end(); ++pos)
    {
      messageMap.insert( MessageMap::value_type(*pos , " => reinstalled " ) ) ;
    }  
  
  for(MessageMap::iterator pos=messageMap.begin() ; pos!= messageMap.end(); ++pos)
    {
      Log::Info() << pos->first << pos->second << std::endl; 
    }
  
}
//--------------------------------------------------------------------------------------------------


void 
Cache::PersistPgks( const StringVec& pkgfiles , bool owntransaction )
{
  
  TmpStore tmpStore(m_db);
  StoreEntryCollector collector(tmpStore); //writes the entries to store on thread end,first private

#pragma omp parallel for shared(tmpStore) firstprivate(collector)  
        for( std::size_t i = 0; i < pkgfiles.size() ; ++i )
          {
            Path path(pkgfiles[i]) ;
            
            Pkg pkfile  = Pkg::create(path.getURL()) ; // TODO re-check this
            if (pkfile.getType() != PkgType::Installed || not pkfile.Load() )
              {
                LogError()<< "waring, unable to load pkgfile with path " << path << "\n";  
              }
            else
              {  
                StoreEntry* se = 
                    new StoreEntry( pkfile.getPathName().getBase(), 
                        pkfile.getDynLinked() ,
                        path.getLastModificationTime()) ;
                collector.collect(se) ;
              }
          }
  
 
  if(owntransaction)m_db.Execute("BEGIN TRANSACTION");
  tmpStore.Persist();
  if(owntransaction)m_db.Execute("COMMIT;");
  
  
}
//--------------------------------------------------------------------------------------------------


void 
Cache::DeletePgks( const StringList& pkgnames, bool owntransaction)
{

  DeletePkgByFullName cmd_delpkg;
  m_db.CompileCommand(&cmd_delpkg);
  
  if( owntransaction ) m_db.Execute("BEGIN TRANSACTION;");
  
  for (StringList::const_iterator pos = pkgnames.begin(); pos != pkgnames.end(); ++pos)
    {
      cmd_delpkg.setFullName(*pos) ;
      cmd_delpkg.Run();
    }
  
  if( owntransaction ) m_db.Execute("COMMIT;"); // TODO , was machen bei fehler?? 

  
}
//--------------------------------------------------------------------------------------------------

void 
Cache::UpdateLdDirs(bool owntransaction )
{
 
  InsertLdDir cmdlddir;
  InsertLdLnkDir cmdldlnkdir;  
  m_db.CompileCommand(&cmdlddir);
  m_db.CompileCommand(&cmdldlnkdir);  

  
  LDDirs lddirs; 
  lddirs.readLdDirs()  ;
  lddirs.readLdLinkDirs() ;
 
  StringSet ldlnknames ;
  
  std::set_difference(lddirs.getLdLnkDirs().begin(), lddirs.getLdLnkDirs().end(), 
      lddirs.getLdDirs().begin(), lddirs.getLdDirs().end(),
      std::inserter(ldlnknames, ldlnknames.begin()));  
  
  if (owntransaction)m_db.Execute("BEGIN TRANSACTION;");
  
  m_db.Execute("DELETE FROM lddirs;");
  m_db.Execute("DELETE FROM ldlnkdirs;");
  
  StringSet::const_iterator iterpos= lddirs.getLdDirs().begin() ;
  for(;iterpos!=lddirs.getLdDirs().end(); ++iterpos)
    {
      cmdlddir.Parameters().Nr(1).set(*iterpos);
      m_db.Execute(&cmdlddir);    
    }
  
  iterpos= ldlnknames.begin() ;
  for(;iterpos!=ldlnknames.end(); ++iterpos)
    {
      cmdldlnkdir.Parameters().Nr(1).set(*iterpos);
      m_db.Execute(&cmdldlnkdir);
    }  
  
  if (owntransaction)m_db.Execute("COMMIT TRANSACTION;");
  
}
//--------------------------------------------------------------------------------------------------

} // ns
