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
#include "sbbdep/lddirs.hpp"

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

class InsertPkg : public a4sqlt3::SqlParamCommand
{
  std::string dummystr;
public:  
  InsertPkg() :
    a4sqlt3::SqlParamCommand(CacheSQL::InsertPkgSQL())
  , dummystr("")
  {
  }//-----------------------------------------------------------------------------------------------
  void Compile()
  {
    a4sqlt3::SqlParamCommand::Compile();
    Parameters()->Nr(1)->set<a4sqlt3::ParamStringRef>( dummystr ) ; // pkgname.FullName()
    Parameters()->Nr(2)->set<a4sqlt3::ParamStringRef>( dummystr ) ; // pkgname.Name()
    Parameters()->Nr(3)->set<a4sqlt3::ParamStringRef>( dummystr ) ; // pkgname.Version()
    Parameters()->Nr(4)->set<a4sqlt3::ParamStringRef>( dummystr ) ; // pkgname.Arch()
    Parameters()->Nr(5)->set<a4sqlt3::ParameterInt>( 0 ) ; //pkgname.Build().Num()
    Parameters()->Nr(6)->set<a4sqlt3::ParamStringRef>( dummystr ) ; // pkgname.Build().Tag()
    Parameters()->Nr(7)->set<a4sqlt3::ParameterInt64>( 0 ) ; // timestamp
  }  
};
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

class InsertDynLinked : public a4sqlt3::SqlParamCommand
{
  std::string dummystr;
public:  
  InsertDynLinked() :
    a4sqlt3::SqlParamCommand(CacheSQL::InsertDynLinkedSQL())
  , dummystr("")
  {
  }//-----------------------------------------------------------------------------------------------
  void Compile()
  {
    a4sqlt3::SqlParamCommand::Compile();
    Parameters()->Nr(1)->set<a4sqlt3::ParameterInt64>( 0 ); // pkgid
    Parameters()->Nr(2)->set<a4sqlt3::ParamStringRef>( dummystr ); //dli.filename
    Parameters()->Nr(3)->set<a4sqlt3::ParameterString>( dummystr ); // dli.filename.getDir()
    Parameters()->Nr(4)->set<a4sqlt3::ParameterString>( dummystr ); // dli.filename.getBase()
    // 5 setNull (default), soname
    Parameters()->Nr(6)->set<a4sqlt3::ParameterInt>( 0 ); // dli.arch    
    
  }
};
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

class InsertRequired : public a4sqlt3::SqlParamCommand
{ 
  std::string dummystr;
public:
  InsertRequired() :
    a4sqlt3::SqlParamCommand(CacheSQL::InsertRequiredSQL())
    , dummystr("")
  {
  }//-----------------------------------------------------------------------------------------------

  void Compile()
  {
    a4sqlt3::SqlParamCommand::Compile();
    Parameters()->Nr(1)->set<a4sqlt3::ParameterInt64>(0); // file id
    Parameters()->Nr(2)->set<a4sqlt3::ParamStringRef>( dummystr ); // needed
  }  

};
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
class InsertRRunPath : public a4sqlt3::SqlParamCommand
{
  std::string dummystr;
public:  
  InsertRRunPath() :
    a4sqlt3::SqlParamCommand(CacheSQL::InsertRRunPathSQL())
  {
  }//-----------------------------------------------------------------------------------------------
  void Compile()
  {
    a4sqlt3::SqlParamCommand::Compile();
    Parameters()->Nr(1)->set<a4sqlt3::ParameterInt64>(0); // file id
    Parameters()->Nr(2)->set<a4sqlt3::ParamStringRef>( dummystr ); // runpath
  }
};
//--------------------------------------------------------------------------------------------------

class InsertLdDir : public a4sqlt3::SqlParamCommand
{
  std::string dummy;
public:
  InsertLdDir() :
    a4sqlt3::SqlParamCommand(CacheSQL::InsertLdDirSQL()), dummy("")
  {
  }//-----------------------------------------------------------------------------------------------
  
  void Compile()
  {
    a4sqlt3::SqlParamCommand::Compile();
    Parameters()->Nr(1)->set<a4sqlt3::ParamStringRef>(dummy);
  }
  
};
//--------------------------------------------------------------------------------------------------
class InsertLdLnkDir : public a4sqlt3::SqlParamCommand
{
  std::string dummy;
public:  
  InsertLdLnkDir() :
    a4sqlt3::SqlParamCommand(CacheSQL::InsertLdLnkDirSQL()), dummy("")
  {
  }//-----------------------------------------------------------------------------------------------
  void Compile()
  {
    a4sqlt3::SqlParamCommand::Compile();
    Parameters()->Nr(1)->set<a4sqlt3::ParamStringRef>(dummy);
  }  
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

  void
  addEntry( StoreEntry* const & entry ) // for the code analyser to ger rid of the semantic error ...
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
  
  
  
  void
  Persist( PkgName& pkgname, DynLinkedInfoList& dllist, int64_t& timestamp)
  {

    m_cmdpkg.Parameters()->Nr(1)->setRefVal( pkgname.FullName() ) ;
    m_cmdpkg.Parameters()->Nr(2)->setRefVal( pkgname.Name() ) ;
    m_cmdpkg.Parameters()->Nr(3)->setRefVal( pkgname.Version() ) ;
    m_cmdpkg.Parameters()->Nr(4)->setRefVal( pkgname.Arch() ) ;
    m_cmdpkg.Parameters()->Nr(5)->setValue( pkgname.Build().Num() ) ;
    m_cmdpkg.Parameters()->Nr(6)->setRefVal( pkgname.Build().Tag() ) ;
    m_cmdpkg.Parameters()->Nr(7)->setValue( timestamp ) ;

    m_dbref.Execute(&m_cmdpkg);
    
    
    int64_t pkgid = m_dbref.getLastInsertRowid() ;
    for( DynLinkedInfoList::const_iterator pos=dllist.begin(); pos!=dllist.end();++pos)
      {

        m_cmddynlinked.Parameters()->Nr(1)->setValue( pkgid );
        m_cmddynlinked.Parameters()->Nr(2)->setRefVal( pos->filename.Str() );
        m_cmddynlinked.Parameters()->Nr(3)->setValue( pos->filename.getDir() );
        m_cmddynlinked.Parameters()->Nr(4)->setValue( pos->filename.getBase() );
        
        if( pos->soName.size()>0 )
          m_cmddynlinked.Parameters()->Nr(5)->set<a4sqlt3::ParamStringRef>( pos->soName );
        else
          m_cmddynlinked.Parameters()->Nr(5)->setNull() ;
        
        m_cmddynlinked.Parameters()->Nr(6)->setValue( pos->arch );            
        
        m_dbref.Execute(&m_cmddynlinked);
        int64_t fileid = m_dbref.getLastInsertRowid() ;
        
        StringList::const_iterator needediter= pos->Needed.begin() ;
        for( ; needediter != pos->Needed.end(); ++needediter)
          {
            m_cmdrequired.Parameters()->Nr(1)->setValue(fileid) ;
            m_cmdrequired.Parameters()->Nr(2)->setRefVal( *needediter );
            m_dbref.Execute(&m_cmdrequired);
          }

        
        StringList::const_iterator rrunpathiter= pos->RunRPaths.begin();
        for( ;rrunpathiter != pos->RunRPaths.end(); ++rrunpathiter)
          {
            m_cmdrrunpath.Parameters()->Nr(1)->setValue(fileid) ;
            m_cmdrrunpath.Parameters()->Nr(2)->setRefVal( *rrunpathiter );
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
          // TODO, filter out others also,  temporary emacs files #filename#
 
          pkgnamevec.push_back(dircontent.getDirName() +"/"+ pkgfilename);          
        }

       
    }
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
                  // other filters required? like if file was edit with emacs.. #filename#
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
      *allinIter = dircontent.getDirName() +  *pos;
      ++allinIter;
    }
  for ( StringList::iterator pos = reinstalledList.begin();pos != reinstalledList.end();++pos)
    {
      *allinIter = dircontent.getDirName() +*pos;
      ++allinIter;
    }  
  
  if(allinIter != allinserts.end()) throw a4z::ErrorNeverReach("valptr transfair to vector failed");
  
  // TODO handle transaction flags for these calls, do transaction maybe arround   
  if(toremoveList.size()> 0) DeletePgks(toremoveList) ;
  if(allinserts.size() > 0 ) PersistPgks(allinserts) ; 
  UpdateLdDirs();
  

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

  DeletePkgByFullName delcmd;
  m_db.CompileCommand(&delcmd);
  std::string dummystr; 
  delcmd.Parameters()->Nr(1)->set<a4sqlt3::ParamStringRef>(dummystr) ;
  
  if( owntransaction ) m_db.Execute("BEGIN TRANSACTION;");
  
  for (StringList::const_iterator pos = pkgnames.begin(); pos != pkgnames.end(); ++pos)
    {
       delcmd.Parameters()->Nr(1)->setRefVal(*pos) ;
       m_db.Execute(&delcmd);
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
      cmdlddir.Parameters()->Nr(1)->setRefVal(*iterpos);
      m_db.Execute(&cmdlddir);    
    }
  
  iterpos= ldlnknames.begin() ;
  for(;iterpos!=ldlnknames.end(); ++iterpos)
    {
      cmdldlnkdir.Parameters()->Nr(1)->setRefVal(*iterpos);
      m_db.Execute(&cmdldlnkdir);
    }  
  
  if (owntransaction)m_db.Execute("COMMIT TRANSACTION;");
  
}
//--------------------------------------------------------------------------------------------------

} // ns
