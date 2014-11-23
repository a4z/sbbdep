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



#ifndef SBBDEP_CACHE_HPP_
#define SBBDEP_CACHE_HPP_

#include <a4sqlt3/database.hpp>

#include <sbbdep/pkgname.hpp>
#include <sbbdep/error.hpp>

namespace sbbdep {

class Pkg;

class Cache : public a4sqlt3::Database
{

public:

  using StringVec = std::vector<std::string> ;

  struct SyncData{
    using UpdateInfo = std::pair<PkgName, PkgName> ;
    StringVec removed;
    StringVec installed;
    StringVec reinstalled;
    std::vector<UpdateInfo> updated;
    bool wasNewCache;
  };

  // till I have a better place, this is here, TODO to config ???
  enum class sqlid {
    insert_pkg,
    insert_dynlinked,
    insert_required,
    insert_rrunpath,
    insert_ldDir,
    insert_ldLnkDir,

    set_keyval ,  // insert or replace
    del_byfullname
  };

  struct dbmsg {
    sqlid id ;
    a4sqlt3::DbValues args;
  };
  // since I rely an lastrowid, .... set it to the arg ( > -1 ) and it will
  // be applied

  using DbMessages = std::vector<dbmsg> ;

  Cache(const std::string& dbname);
  ~Cache();



  SyncData doSync();


  // sets last row id, if given, therefore not const
  void processMessage(dbmsg&) ;

private:




  // 01 check if there is a schema,
  // if not create it and return that the db is new
  bool isNewDb();

  void createDbSchema() ;
  // 01 b if not a new db, need to check if the schema is actual
  void checkDbSchemaVersion() ;

  // 02 get the sync data
  SyncData createNewSyncData(); //must set wasNewCache true

  SyncData createUpdateSyncData();

  // for new cache, create db data
  void createIndex(const SyncData& data);

  // for existing cache, update db data
  void updateIndex(const SyncData& data);

// internal helpers for above


  // stored sql command names

  // get stored command, if it does not exist, its created
  a4sqlt3::SqlCommand& getCommand(sqlid id) ;


  void
  indexPkg(const Pkg& pkg);


  // stored sql commands
  using commandMap = std::map<sqlid,a4sqlt3::SqlCommand> ;
  commandMap _commands;
  const std::string _name;


  void updateLdDirInfo();  // do not forget to implements this here persistLdSoTime

  int64_t latesPkgTimeStampInDb();

  
};



} // ns

#endif /* SBBDEP_CACHE_HPP_ */
