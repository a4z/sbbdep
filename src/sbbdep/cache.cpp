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

#include <sbbdep/dircontent.hpp>
#include <sbbdep/path.hpp>
#include <sbbdep/pkgname.hpp>

#include <sbbdep/error.hpp>


#include <sbbdep/lddirs.hpp>
#include <sbbdep/pkgadmdir.hpp>
#include <sbbdep/log.hpp>


#include <a4sqlt3/columns.hpp>
#include <a4sqlt3/error.hpp>

#include <vector>
#include <set>
#include <algorithm>




namespace sbbdep {



Cache::Cache( const std::string& dbname ) :
  m_db(dbname)
{
  
  

  try
    {
      m_db.Open();
    }
  catch ( const a4sqlt3::SQLite3Error& e )
    {
      
      try
        {
          m_db.Create();
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
Cache::doSync()
{
  
  if (m_db.isNew())
    {
      Log::Info() << "create cache (" << m_db.Name() <<")" << std::endl;
      CreateData();
    }
  else
    {
      Log::Info() << "sync cache (" << m_db.Name() <<")" << std::endl;
      SyncData();
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

  LDDirs lddirs;
  LogInfo() << "searching for links"<< std::endl;
  lddirs.getLdLnkDirs();


  m_db.updateData(StringVec(), pkgnamevec  ,
      StringVec(lddirs.getLdDirs().begin(), lddirs.getLdDirs().end()) ,
      StringVec(lddirs.getLdLnkDirs().begin(), lddirs.getLdLnkDirs().end())
  );



  
}
//--------------------------------------------------------------------------------------------------


void
Cache::SyncData()
{
  // get diff from filesystem and db, remove old stuff from db and insert new 
  // get what is not present in fs but in db for remove
  // get what is not present in db but in fs for insert
  // get files with newer date and existing name for handling reinstalled pkgs
  
  LogInfo() << "search for changes" << std::endl;
  StringSet allpkgfiles; // all pks in file system
  StringSet allpkgindb; // all pks in the db
  StringSet newpkgs; // // all pks in file system with a newer date as the lastest known date


  // get latest known file time for a package to compare later

  const time_t maxknownftime = m_db.getLatestPkgTimeStamp() ;

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


  StringVec allremoves;  ;
  allremoves.insert(allremoves.end(), toremoveList.begin(), toremoveList.end()) ;
  allremoves.insert(allremoves.end(), reinstalledList.begin(), reinstalledList.end()) ;

  // if just delete, no update of Ld Dirs is required...
  if(allinserts.size() == 0)
    {
      // call just if something is to remove
      if(allremoves.size()> 0 )
        m_db.updateData( allremoves , allinserts ,StringVec() , StringVec() ) ;

      return;
    }


  LDDirs lddirs;
  // force pre-loading
  LogInfo() << "searching for links"<< std::endl;
  lddirs.getLdLnkDirs();
  // TODO , check for putting this in the background at the begin of this function
  // or some other speed up, this takes somehow to much time...
  
  // TODO if nothing changed, skip the ld part (and the update call)

  LogInfo() << "apply changes "<< std::endl;

  m_db.updateData( allremoves , allinserts ,
      StringVec(lddirs.getLdDirs().begin(), lddirs.getLdDirs().end()) ,
      StringVec(lddirs.getLdLnkDirs().begin(), lddirs.getLdLnkDirs().end())
      ) ;



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


//--------------------------------------------------------------------------------------------------

} // ns
