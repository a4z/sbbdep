/*
--------------Copyright (c) 2010-2011 H a r a l d  A c h i t z---------------
-----------------< a g e dot a 4 z at g m a i l dot c o m >------------------
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


#include "sbbdep/cache.hpp"

#include "sbbdep/cachesql.hpp"

#include "sbbdep/dircontent.hpp"
#include "sbbdep/path.hpp"
#include "sbbdep/pathname.hpp"
#include "sbbdep/pkgname.hpp"
#include "sbbdep/pkgfile.hpp" 
#include "sbbdep/error.hpp"
#include "sbbdep/stringlist.hpp"
#include "sbbdep/dynlinkedinfolist.hpp"

#include "sbbdep/log.hpp"

#include "a4sqlt3/sqlparamcommand.hpp"
#include "a4sqlt3/parameters.hpp"

#include "a4sqlt3/error.hpp"
#include "a4sqlt3/rowhandler.hpp"
#include "a4sqlt3/columns.hpp"
#include "a4sqlt3/onevalresult.hpp"

#include "a4z/errtrace.hpp"

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

struct InsertPkg : public a4sqlt3::SqlParamCommand
{
  InsertPkg() :
    a4sqlt3::SqlParamCommand(CacheSQL::InsertPkgSQL())
  {
  }//-----------------------------------------------------------------------------------------------
};
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

struct InsertDynLinked : public a4sqlt3::SqlParamCommand
{
  InsertDynLinked() :
    a4sqlt3::SqlParamCommand(CacheSQL::InsertDynLinkedSQL())
  {
  }//-----------------------------------------------------------------------------------------------
};
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

struct InsertRequired : public a4sqlt3::SqlParamCommand
{
  InsertRequired() :
    a4sqlt3::SqlParamCommand(CacheSQL::InsertRequiredSQL())
  {
  }//-----------------------------------------------------------------------------------------------
};
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
struct InsertRRunPath : public a4sqlt3::SqlParamCommand
{
  InsertRRunPath() :
    a4sqlt3::SqlParamCommand(CacheSQL::InsertRRunPathSQL())
  {
  }//-----------------------------------------------------------------------------------------------
};
//--------------------------------------------------------------------------------------------------

struct DeletePkgByFullName : public a4sqlt3::SqlParamCommand
{
  DeletePkgByFullName() :
    a4sqlt3::SqlParamCommand(CacheSQL::DeletePkgByFullnameSQL())
  {
  }//-----------------------------------------------------------------------------------------------

};
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

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
  bool m_cmdpkg_param_init; 
  bool m_cmddynlinked_init;
  bool m_cmdrequired_init;
  bool m_cmdrrunpath_init;
  
  StoreEntryList m_entrylist;

public:
  TmpStore( CacheDB& dbref ) :
    m_cmdpkg(), m_cmddynlinked(), m_cmdrequired() ,m_dbref(dbref),
    m_cmdpkg_param_init(false), m_cmddynlinked_init(false), m_cmdrequired_init(false) 
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

  void
  addEntry( StoreEntry* entry ) // for my collector pattern, lock it here...
  {
#pragma omp critical (tmpStoreAddEntry)
      {
        m_entrylist.push_back(entry);
      }
  }//-----------------------------------------------------------------------------------------------

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

  void CmdPkgParamInit(PkgName& pkgname, int64_t& timestamp)
  {
    m_cmdpkg.Parameters()->Nr(1)->set<a4sqlt3::ParamStringRef>( pkgname.FullName() ) ;
    m_cmdpkg.Parameters()->Nr(2)->set<a4sqlt3::ParamStringRef>( pkgname.Name() ) ;
    m_cmdpkg.Parameters()->Nr(3)->set<a4sqlt3::ParamStringRef>( pkgname.Version() ) ;
    m_cmdpkg.Parameters()->Nr(4)->set<a4sqlt3::ParamStringRef>( pkgname.Arch() ) ;
    m_cmdpkg.Parameters()->Nr(5)->set<a4sqlt3::ParameterInt>( pkgname.Build().Num() ) ;
    m_cmdpkg.Parameters()->Nr(6)->set<a4sqlt3::ParamStringRef>( pkgname.Build().Tag() ) ;
    m_cmdpkg.Parameters()->Nr(7)->set<a4sqlt3::ParameterInt64>( timestamp ) ;    
    m_cmdpkg_param_init = true;
  }
  void CmdDynLinkedInit(int64_t& pkgid, const DynLinkedInfo& dli)
  {
    m_cmddynlinked.Parameters()->Nr(1)->set<a4sqlt3::ParameterInt64>( pkgid );
    m_cmddynlinked.Parameters()->Nr(2)->set<a4sqlt3::ParamStringRef>( dli.filename );
    m_cmddynlinked.Parameters()->Nr(3)->set<a4sqlt3::ParameterString>( dli.filename.getDir() );
    m_cmddynlinked.Parameters()->Nr(4)->set<a4sqlt3::ParameterString>( dli.filename.getBase() );
    
    if( dli.soName.size()>0 )
      m_cmddynlinked.Parameters()->Nr(5)->set<a4sqlt3::ParamStringRef>( dli.soName );
    else
      m_cmddynlinked.Parameters()->Nr(5)->setNull();
    
    m_cmddynlinked.Parameters()->Nr(6)->set<a4sqlt3::ParameterInt>( dli.arch );    
    m_cmddynlinked_init = true;
  }  
  void CmdRequiredInit(int64_t& fileid, const std::string& needed)
  {
    m_cmdrequired.Parameters()->Nr(1)->set<a4sqlt3::ParameterInt64>(fileid) ;
    m_cmdrequired.Parameters()->Nr(2)->set<a4sqlt3::ParamStringRef>( needed );
    m_cmdrequired_init = true;
  }  
  void CmdRRRunPathInit(int64_t& fileid, const std::string& rp)
  {
    m_cmdrrunpath.Parameters()->Nr(1)->set<a4sqlt3::ParameterInt64>(fileid) ;
    m_cmdrrunpath.Parameters()->Nr(2)->set<a4sqlt3::ParamStringRef>( rp );
    m_cmdrrunpath_init = true;
  }  
  
  
  void
  Persist( PkgName& pkgname, DynLinkedInfoList& dllist, int64_t& timestamp)
  {
    //InsertPkg m_cmdpkg;
    //"INSERT INTO pkgs (fullname, name, version, arch, build, tag, timestamp)"
    //            " VALUES( ?, ?,?,?,?,?,? ) ; "    
    //InsertDynLinked m_cmddynlinked;
    //"INSERT INTO dynlinked ( pkg_id, filename, dirname, basename,soname, arch)"
    //         " VALUES( ?,?,?,?,?,? ) ; "    
    //InsertRequired m_cmdrequired;    
    //"INSERT INTO required ( dynlinked_id, needed )"
    //           " VALUES( ?,? ) ; "    
    
    // mach das noch flotter, fuer jedes ein has param init, wenn false, set methode, sonst set value
    
    if ( !m_cmdpkg_param_init) CmdPkgParamInit( pkgname , timestamp) ;
    else
      {
        m_cmdpkg.Parameters()->Nr(1)->setRefVal( pkgname.FullName() ) ;
        m_cmdpkg.Parameters()->Nr(2)->setRefVal( pkgname.Name() ) ;
        m_cmdpkg.Parameters()->Nr(3)->setRefVal( pkgname.Version() ) ;
        m_cmdpkg.Parameters()->Nr(4)->setRefVal( pkgname.Arch() ) ;
        m_cmdpkg.Parameters()->Nr(5)->setValue( pkgname.Build().Num() ) ;
        m_cmdpkg.Parameters()->Nr(6)->setRefVal( pkgname.Build().Tag() ) ;
        m_cmdpkg.Parameters()->Nr(7)->setValue( timestamp ) ;
      }
    m_dbref.Execute(&m_cmdpkg);
    
    
    int64_t pkgid = m_dbref.getLastInsertRowid() ;
    for( DynLinkedInfoList::const_iterator pos=dllist.begin(); pos!=dllist.end();++pos)
      {
        
        if (!m_cmddynlinked_init) CmdDynLinkedInit( pkgid, *pos ) ;
        else
          {
            m_cmddynlinked.Parameters()->Nr(1)->setValue( pkgid );
            m_cmddynlinked.Parameters()->Nr(2)->setRefVal( pos->filename.Str() );
            m_cmddynlinked.Parameters()->Nr(3)->setValue( pos->filename.getDir() );
            m_cmddynlinked.Parameters()->Nr(4)->setValue( pos->filename.getBase() );
            //m_cmddynlinked.Parameters()->Nr(5)->setRefVal( pos->soName );
            if( pos->soName.size()>0 )
              m_cmddynlinked.Parameters()->Nr(5)->set<a4sqlt3::ParamStringRef>( pos->soName );
            else
              m_cmddynlinked.Parameters()->Nr(5)->setNull() ;
            
            m_cmddynlinked.Parameters()->Nr(6)->setValue( pos->arch );            
          }
        
        m_dbref.Execute(&m_cmddynlinked);
        int64_t fileid = m_dbref.getLastInsertRowid() ;
        
        StringList::const_iterator needediter= pos->Needed.begin() ;
        while( needediter != pos->Needed.end())
          {
            if(!m_cmdrequired_init) CmdRequiredInit(fileid, *needediter) ;
            else
              {
                m_cmdrequired.Parameters()->Nr(1)->setValue(fileid) ;
                m_cmdrequired.Parameters()->Nr(2)->setRefVal( *needediter );
              }
            m_dbref.Execute(&m_cmdrequired);
            ++needediter;
          }
        
        StringList::const_iterator rrunpathiter= pos->RunRPaths.begin();
        while( rrunpathiter != pos->RunRPaths.end())
          {
            if(!m_cmdrrunpath_init) CmdRRRunPathInit(fileid, *rrunpathiter) ;
            else
              {
                m_cmdrrunpath.Parameters()->Nr(1)->setValue(fileid) ;
                m_cmdrrunpath.Parameters()->Nr(2)->setRefVal( *rrunpathiter );
              }
            m_dbref.Execute(&m_cmdrrunpath);
            ++rrunpathiter;
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
  collect( StoreEntry* se )
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
          m_db.Open(); // TODO, think about single thread for create ...
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
  
  // TODO, alt least for create a single thread mode open could be.
  if (m_isnew)
    {
      std::cout << "create data " << std::endl;
      CreateData();
      // create indexes after first data
      CreateIndexes() ; 
    }
  else
    {
      std::cout << "sync data " << std::endl;
      SyncData();
    }
  
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
Cache::PersistPgks( const StringVec& pkgfiles )
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
              { //StoreEntry( const std::string& pkgname, const DynLinkedInfoList& dllist ) 
                StoreEntry* se = 
                    new StoreEntry( pkfile.getPathName().getBase(), 
                        pkfile.getDynLinkedInfos() ,
                        path.getLastModificationTime()) ;
                collector.collect(se) ;
              }
          }
  

  // make transaktion here 
  m_db.Execute("BEGIN TRANSACTION");
  tmpStore.Persist();
  m_db.Execute("COMMIT;");
  
  
}
//--------------------------------------------------------------------------------------------------

void
Cache::CreateData()
{
  

  DirContent dircontent("/var/adm/packages/");
  
  dircontent.Open();
  
  StringVec pkgnamevec;
  
  std::string pkgfilename;
  while (dircontent.getNext(pkgfilename))  
    {
      if (pkgfilename.length() > 2) 
        {
          if ( pkgfilename[0]=='.' ) continue;
          if ( *(pkgfilename.rbegin())=='~'  ) continue ;
          // TODO, filter out .filenames and other filters, like filename~ or #filename#
 
          pkgnamevec.push_back(dircontent.getDirName() +"/"+ pkgfilename);          
        }

       
    }
  
  PersistPgks( pkgnamevec ) ;
  
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
  

  StringSet fpkgs;
  StringSet dbpkgs;
  StringSet newpkgs;
  
  DirContent dircontent("/var/adm/packages/");
  
  dircontent.Open();
  
  struct RowHandler : public a4sqlt3::RowHandler
  {
    StringSet& m_dbpkgs;
    RowHandler( StringSet& dbpkgs_ ) :
      m_dbpkgs(dbpkgs_)
    {
    }
    
    bool
    OnHandleRow( a4sqlt3::Columns& rowcols, sqlite3_stmt* stmt )
    {
      m_dbpkgs.insert(rowcols[0].get<std::string>());
      return true;
    }
  };
  
  RowHandler rh(dbpkgs);
  
  a4sqlt3::OneValResult< int64_t > MaxTimeStampRH;
  
  m_db.Execute(CacheSQL::MaxPkgTimeStamp(), &MaxTimeStampRH);
  
#pragma omp parallel sections
    {
      
#pragma omp section
        {
          
          std::string pkgfilename;
          
          while (dircontent.getNext(pkgfilename))
            {
              if (pkgfilename.length() > 2) 
                {
                  // TODO, filter out .filenames and other filters, like filename~ or #filename#
                  if ( pkgfilename[0]=='.' ) continue;
                  if ( *(pkgfilename.rbegin())=='~'  ) continue ;
                  fpkgs.insert(pkgfilename);                  
                }
              
              
              Path path(dircontent.getDirName() + "/" + pkgfilename);
              if (path.isRegularFile()) // why did i do this, is it required?
                { //also get reinstalled pkgs.. these have the same name but a newer time..
                  if (path.getLastModificationTime() > MaxTimeStampRH.Val())
                    {
                      newpkgs.insert(pkgfilename);
                    }
                }
            }
        }// opm section
#pragma omp section
        {
          m_db.Execute("SELECT fullname FROM pkgs;", &rh);
        } // opm section
    } //omp parallel sections
    

  // vergleich machen, unterschiede , .... 
  StringList toremoveList;
  StringList toinsertList;
  StringList reinstalledList;
  
  //TH1 toremoveList ist was in db aber nicht fs ist, 
  //TH2 toinsertVec ist was in fs aber nicht db ist, get reinstalled
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

  //reinstalled to remove and insert list..
  toremoveList.insert(toremoveList.end(), reinstalledList.begin(), reinstalledList.end());
  toinsertList.insert(toinsertList.end(), reinstalledList.begin(), reinstalledList.end());
  
  StringVec allinserts( toinsertList.size() + reinstalledList.size() ) ;
  StringVec::iterator allinIter = allinserts.begin(); 
  for ( StringList::iterator pos=toinsertList.begin();pos!=toinsertList.end() ;++pos)
    {
      *allinIter = dircontent.getDirName() +  *pos;
      ++allinIter;
    }
  for ( StringList::iterator pos = reinstalledList.begin();pos != reinstalledList.end();++pos)
    {
      *allinIter = dircontent.getDirName() +*pos;
      ++allinIter;
    }  
  
  if(allinIter != allinserts.end()) throw a4z::ErrorNeverReach("valptr transfair to vector failed");
  
  if(allinserts.size() > 0 ) PersistPgks(allinserts) ; 
  // if this worked, than continue delete old pkgs.. 

  
  DeletePkgByFullName delcmd;
  m_db.CompileCommand(&delcmd);
  std::string dummystr; 
  delcmd.Parameters()->Nr(1)->set<a4sqlt3::ParamStringRef>(dummystr) ;
  
  if( toremoveList.size() > 0 ) m_db.Execute("BEGIN TRANSACTION;");
  
  for (StringList::iterator pos = toremoveList.begin(); pos != toremoveList.end(); ++pos)
    {
       delcmd.Parameters()->Nr(1)->setRefVal(*pos) ;
       m_db.Execute(&delcmd);
    }
  
  if( toremoveList.size() > 0 ) m_db.Execute("COMMIT;"); // TODO , was machen bei fehler?? 
  
/*
  // TODO, das da anpassen, besser machen ( removed, upgraded, installed, reinstalled ) 
  std::cout << "sync removed:\n";
  std::copy(toremoveList.begin(), toremoveList.end(), std::ostream_iterator< std::string >(
      std::cout, "\n"));
  
  std::cout << "sync installed:\n";
  std::copy(toinsertList.begin(), toinsertList.end(), std::ostream_iterator< std::string >(
      std::cout, "\n"));
  
  // contain all that are newer by date
  std::cout << "sync new by date:\n";
  std::copy(newpkgs.begin(), newpkgs.end(), std::ostream_iterator< std::string >(std::cout, "\n"));
  
  std::cout << "sync reinstall:\n";
  std::copy(reinstalledList.begin(), reinstalledList.end(), std::ostream_iterator< std::string >(
      std::cout, "\n"));
  std::cout << "\n\n";
*/

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
      std::cout << pos->first << pos->second << std::endl; 
    }
  
}
//--------------------------------------------------------------------------------------------------


} // ns
