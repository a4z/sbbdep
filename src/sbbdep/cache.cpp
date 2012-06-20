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
#include <sbbdep/pkgfile.hpp>
#include <sbbdep/error.hpp>
#include <sbbdep/stringlist.hpp>
#include <sbbdep/dynlinkedinfolist.hpp>
#include <sbbdep/lddirs.hpp>

#include <sbbdep/cachecmds.hpp>
#include <sbbdep/pkgadmdir.hpp>


#include <sbbdep/log.hpp>


#include <a4sqlt3/sqlparamcommand.hpp>
#include <a4sqlt3/parameters.hpp>



#include <a4sqlt3/error.hpp>
#include <a4sqlt3/rowhandler.hpp>
#include <a4sqlt3/columns.hpp>
#include <a4sqlt3/onevalresult.hpp>

#include <a4z/errtrace.hpp>

#include <list>
#include <vector>
#include <set>
#include <algorithm>
#include <iterator>

#include <iostream>



namespace sbbdep {

namespace {
// some helpers
//--------------------------------------------------------------------------------------------------

using namespace sbbdep;


struct StoreEntry
{

  PkgName m_pkname;
  DynLinkedInfoList m_dllist;
  int64_t m_timestamp; 

  StoreEntry( const PkgName& pkname, const DynLinkedInfoList& dllist, const int64_t&timestamp )
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
  Persist( PkgName& pkgname, DynLinkedInfoList& dllist, int64_t& timestamp)
  {

    m_cmdpkg.Parameters()->Nr(1)->setValue( pkgname.FullName() ) ;
    m_cmdpkg.Parameters()->Nr(2)->setValue( pkgname.Name() ) ;
    m_cmdpkg.Parameters()->Nr(3)->setValue( pkgname.Version() ) ;
    m_cmdpkg.Parameters()->Nr(4)->setValue( pkgname.Arch() ) ;
    m_cmdpkg.Parameters()->Nr(5)->setValue( pkgname.Build().Num() ) ;
    m_cmdpkg.Parameters()->Nr(6)->setValue( pkgname.Build().Tag() ) ;
    m_cmdpkg.Parameters()->Nr(7)->setValue( timestamp ) ;

    m_dbref.Execute(&m_cmdpkg);
    
    
    int64_t pkgid = m_dbref.getLastInsertRowid() ;
    for( DynLinkedInfoList::const_iterator pos=dllist.begin(); pos!=dllist.end();++pos)
      {

        m_cmddynlinked.Parameters()->Nr(1)->setValue( pkgid );
        m_cmddynlinked.Parameters()->Nr(2)->setValue( pos->filename.Str() );
        m_cmddynlinked.Parameters()->Nr(3)->setValue( pos->filename.getDir() );
        m_cmddynlinked.Parameters()->Nr(4)->setValue( pos->filename.getBase() );
        
        if( pos->soName.size()>0 )
          m_cmddynlinked.Parameters()->Nr(5)->setType<a4sqlt3::ParameterStringRef>( pos->soName );
        else
          m_cmddynlinked.Parameters()->Nr(5)->setNull() ;
        
        m_cmddynlinked.Parameters()->Nr(6)->setValue( pos->arch );            
        
        m_dbref.Execute(&m_cmddynlinked);
        int64_t fileid = m_dbref.getLastInsertRowid() ;
        
        StringList::const_iterator needediter= pos->Needed.begin() ;
        for( ; needediter != pos->Needed.end(); ++needediter)
          {
            m_cmdrequired.Parameters()->Nr(1)->setValue(fileid) ;
            m_cmdrequired.Parameters()->Nr(2)->setValue( *needediter );
            m_dbref.Execute(&m_cmdrequired);
          }

        
        StringList::const_iterator rrunpathiter= pos->RunRPaths.begin();
        for( ;rrunpathiter != pos->RunRPaths.end(); ++rrunpathiter)
          {
            m_cmdrrunpath.Parameters()->Nr(1)->setValue(fileid) ;
            m_cmdrrunpath.Parameters()->Nr(2)->setValue( *rrunpathiter );
            m_dbref.Execute(&m_cmdrrunpath);
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


Cache::Cache( const char* dbname ) :
  m_db(std::string(dbname))
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
          throw A4Z_ERRTRACE( e );
        }
      catch ( const a4z::Err& e )
        {
          throw A4Z_ERRTRACE( e );
        }
      
    }
  
}
//--------------------------------------------------------------------------------------------------


Cache::~Cache()
{
  
}
//--------------------------------------------------------------------------------------------------


void
Cache::doSync()
{
  
  if (m_isnew)
    {
      Log::Info() << "create cache  \n" << std::endl;
      CreateData();
      // create indexes after first data
      CreateIndexes() ; 
    }
  else
    {
      Log::Info() << "sync cache \n" << std::endl;
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
      throw A4Z_ERRTRACE( e );
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
      throw A4Z_ERRTRACE( e );
    }
  
}
//--------------------------------------------------------------------------------------------------


void
Cache::CreateData()
{
  
  StringVec pkgnamevec;

  PkgAdmDir pkg_adm_dir;
  auto newfiles_cb = [&](const std::string& d,const std::string& f) -> bool {
    pkgnamevec.push_back(d +"/"+ f);
    return true ;
  };
  pkg_adm_dir.apply(newfiles_cb) ;

  // TODO handle transaction flags for these calls, do transaction maybe arround
  PersistPgks( pkgnamevec ) ;
  UpdateLdDirs() ;

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
  

  StringSet fpkgs; // all pks in file system
  StringSet dbpkgs; // all pks in the db
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
          auto newfiles_cb = [&fpkgs, &newpkgs, &maxknownftime](const std::string& d,const std::string& f) -> bool {
            fpkgs.insert(f); // insert filename in all existing file_pkgs,
            Path path(d + "/" + f); // and if filedate is newer, insert into newpkgs
            if (path.isRegularFile() && path.getLastModificationTime() > maxknownftime)
                  newpkgs.insert(f);
            return true ;
          };
          pkg_adm_dir.apply(newfiles_cb) ;

        }// opm section
#pragma omp section
        {
          // insert existing names into all_pkgs;
          auto rh = [&dbpkgs](a4sqlt3::Columns& cols) -> bool {
            dbpkgs.insert(cols[0].get<std::string>());
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
          std::set_difference(dbpkgs.begin(), dbpkgs.end(), fpkgs.begin(), fpkgs.end(),
              std::inserter(toremoveList, toremoveList.begin()));
        }// opm section
#pragma omp  section
        {
          
          std::set_difference(fpkgs.begin(), fpkgs.end(), dbpkgs.begin(), dbpkgs.end(),
              std::inserter(toinsertList, toinsertList.begin()));

          std::set_difference(newpkgs.begin(), newpkgs.end(), toinsertList.begin(),
              toinsertList.end(), std::inserter(reinstalledList, reinstalledList.begin()));
          
        }// opm section
    } //omp sections

  
  toremoveList.insert(toremoveList.end(), reinstalledList.begin(), reinstalledList.end());
  toinsertList.insert(toinsertList.end(), reinstalledList.begin(), reinstalledList.end());
  //need full path to precess these packages
  StringVec allinserts( toinsertList.size() + reinstalledList.size() ) ;
  StringVec::iterator allinIter = allinserts.begin(); 
  for ( StringList::iterator pos=toinsertList.begin();pos!=toinsertList.end() ;++pos)
    {
      *allinIter = pkg_adm_dir.getDirName() + "/" +  *pos;
      ++allinIter;
    }
  for ( StringList::iterator pos = reinstalledList.begin();pos != reinstalledList.end();++pos)
    {
      *allinIter = pkg_adm_dir.getDirName() + "/" +*pos;
      ++allinIter;
    }  
  
  if(allinIter != allinserts.end())
    throw a4z::ErrorNeverReach("valptr transfair to vector failed");
  

  m_db.Execute("BEGIN TRANSACTION");

  if(toremoveList.size()> 0)
    DeletePgks(toremoveList, false) ;

  if(allinserts.size() > 0 )
    PersistPgks(allinserts, false) ;

  UpdateLdDirs(false);

  m_db.Execute("COMMIT TRANSACTION");

  //--------------------
  // from here only info generation about what happend within sync..
  //--------------------
  typedef std::map<std::string, std::string> MessageMap;
  MessageMap messageMap;
  for (StringList::iterator pos = toinsertList.begin(); pos != toinsertList.end(); ++pos)
    {
      
      PkgName inname(*pos) ;
      StringList::iterator del = toremoveList.begin();
      while( del!= toremoveList.end())
        {
          PkgName delname(*del) ;
          if ( delname.Name() == inname.Name()) break;
          ++del; 
        }

      if ( del != toremoveList.end()) 
        {
          messageMap.insert( MessageMap::value_type(*pos , " => update from " + *del ) ) ;
          toremoveList.erase(del) ;
        }
      else
        {
          messageMap.insert( MessageMap::value_type(*pos , " => installed " ) ) ;
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
  StoreEntryCollector collector(tmpStore); //writes the entries to store on thread end,firstprvate

#pragma omp parallel for shared(tmpStore) firstprivate(collector)  
        for( std::size_t i = 0; i < pkgfiles.size() ; ++i )
          {
            Path path(pkgfiles[i]) ;
            
            PkgFile pkfile(path.getURL()) ;
            if (!pkfile.Load() )
              {
                LogError()<< "waring, unable to load pkgfile with path " << path << "\n";  
              }
            else
            // query > 0 == stupid cause non bin pkgs would always be handled within sync
            //if( pkfile.getDynLinkedInfos().size() > 0 )   
              {  
                StoreEntry* se = 
                    new StoreEntry( pkfile.getPathName().getBase(), 
                        pkfile.getDynLinkedInfos() ,
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
     // m_db.Execute(&cmd_delpkg);
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
      cmdlddir.Parameters()->Nr(1)->setValue(*iterpos);
      m_db.Execute(&cmdlddir);    
    }
  
  iterpos= ldlnknames.begin() ;
  for(;iterpos!=ldlnknames.end(); ++iterpos)
    {
      cmdldlnkdir.Parameters()->Nr(1)->setValue(*iterpos);
      m_db.Execute(&cmdldlnkdir);
    }  
  
  if (owntransaction)m_db.Execute("COMMIT TRANSACTION;");
  
}
//--------------------------------------------------------------------------------------------------

} // ns
