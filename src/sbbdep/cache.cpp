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

#include <sbbdep/pkgadmdir.hpp>
#include <sbbdep/log.hpp>


#include <a4sqlt3/columns.hpp>
#include <a4sqlt3/error.hpp>

#include <vector>
#include <set>
#include <algorithm>
#include <thread>



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



Cache::SyncData
Cache::doSync()
{

  StringVec toremove;
  StringVec toinsert;

  SyncData syncdata;

  if (m_db.isNew())
    {
      Log::Info() << "create cache (" << m_db.Name() <<")" << std::endl;

      auto newfiles_cb = [&toinsert](const std::string& d,const std::string&& f) -> bool {
        toinsert.emplace_back( f);
        return true ;
      };
      PkgAdmDir().apply(newfiles_cb) ;

      // for the report
      syncdata = {toremove, toinsert, StringVec() } ;

    }
  else
    {
      Log::Info() << "sync cache (" << m_db.Name() <<")" << std::endl;

      syncdata = getSyncData() ;

      toremove = syncdata.removed;
      toinsert = syncdata.installed;

      //merge reinstalled into installed and removed, need to be handled both
      // but them on the begin, think looks nicer in the output
      toremove.insert(toremove.begin(), syncdata.reinstalled.begin(), syncdata.reinstalled.end());
      toinsert.insert(toinsert.begin(), syncdata.reinstalled.begin(), syncdata.reinstalled.end());

    }
 

  m_db.updateData( toremove , toinsert) ;


  return syncdata  ;

}
//--------------------------------------------------------------------------------------------------



Cache::SyncData
Cache::getSyncData()
{

  // get diff from filesystem and db, remove old stuff from db and insert new
  // get what is not present in fs but in db for remove
  // get what is not present in db but in fs for insert
  // get files with newer date and existing name for handling reinstalled pkgs

  LogInfo() << "search for changes" << std::endl;

  using StringSet  = std::set<std::string> ;


  Cache::SyncData retval;


  StringSet allpkgfiles; // all pks in file system
  StringSet newpkgs; // // all pks in file system with a newer date as the latest known date
  const time_t maxknownftime = m_db.getLatestPkgTimeStamp() ;

  // helper to wait for a thread
  auto waitfor = [](std::thread& th){ if(th.joinable()) th.join(); };

  // filter for pkgadm dir
  auto checkFileSystem = [&allpkgfiles, &newpkgs, &maxknownftime]
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



  PkgAdmDir admdir;
  std::thread checkFS(&PkgAdmDir::apply, &admdir, checkFileSystem);
// while this runs, get all from the db

  StringSet allpkgindb; // all pks in the db
  // get all in the database
  auto rh = [&allpkgindb](a4sqlt3::Columns& cols) -> bool
        {
          allpkgindb.insert(cols[0].get<std::string>());
          return true ;
        } ;

  m_db.Execute("SELECT fullname FROM pkgs;", rh);

  waitfor(checkFS);

  //get the removed ones, these are not in the filesystem but in the db
  std::thread searchdeleted ( [&allpkgindb, &allpkgfiles , &retval]()->void{
    std::set_difference(allpkgindb.begin(), allpkgindb.end(),
        allpkgfiles.begin(), allpkgfiles.end(),
        std::inserter(retval.removed, retval.removed.begin()));
  });


  // all that are in filesystem but not in db are new and need to be added to installed
  std::set_difference(allpkgfiles.begin(), allpkgfiles.end(),
      allpkgindb.begin(), allpkgindb.end(),
      std::inserter(retval.installed, retval.installed.begin() ));

  // check sorted, should not be required but ensure that next difference will work
  if(not std::is_sorted(std::begin(retval.installed), std::end(retval.installed)))
    std::sort(std::begin(retval.installed), std::end(retval.installed));

  // new pkgs that are not in the installed list are re-installed
  std::set_difference(newpkgs.begin(), newpkgs.end(),
      retval.installed.begin(), retval.installed.end(),
      std::inserter(retval.reinstalled, retval.reinstalled.begin()));

  waitfor(searchdeleted);

  return retval;
}


//--------------------------------------------------------------------------------------------------

} // ns
