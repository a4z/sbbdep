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


#include "sbbdep/cachesql.hpp"

#include "sbbdep/pathname.hpp"
#include <boost/algorithm/string/replace.hpp>

#include <sqlite3.h>

namespace sbbdep {



// cache stuff 
std::string 
CacheSQL::CreateSchemaSQL()
{
  
  
  return "CREATE TABLE pkgs ("
      "    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
      "    fullname TEXT NOT NULL, "
      "    name TEXT NOT NULL, "
      "    version TEXT NOT NULL, "
      "    arch TEXT NOT NULL, "
      "    build INTEGER NOT NULL, "
      "    tag TEXT, "
      "    timestamp INTEGER NOT NULL "
      ");"
      "CREATE TABLE dynlinked ( "
      "    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
      "    pkg_id INTEGER NOT NULL, "      
      "    filename TEXT NOT NULL, "
      "    dirname TEXT NOT NULL, "
      "    basename TEXT NOT NULL, "
      "    soname TEXT , "
      "    arch INTEGER NOT NULL "
      ");"
      "CREATE TABLE required ( "
      "    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
      "    dynlinked_id INTEGER NOT NULL, "
      "    needed TEXT NOT NULL"
      ");"      
      "CREATE TABLE rrunpath( "
      "    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
      "    dynlinked_id INTEGER NOT NULL,     "
      "    ldpath TEXT NOT NULL "
      ");"
      "CREATE TRIGGER on_before_delete_pkgs BEFORE DELETE ON pkgs "
      "  FOR EACH ROW  BEGIN"
      "  DELETE from dynlinked WHERE pkg_id = OLD.id;"
      "  END;"
      "CREATE TRIGGER on_before_delete_dynlinked BEFORE DELETE ON dynlinked "
      "  FOR EACH ROW  BEGIN"
      "  DELETE from required WHERE dynlinked_id = OLD.id;"
      "  DELETE from rrunpath WHERE dynlinked_id = OLD.id;"
      "  END;"
      "CREATE TABLE lddirs (dirname TEXT PRIMARY KEY NOT NULL);"
      "INSERT INTO lddirs ( dirname ) VALUES ('/lib') ;"
      "INSERT INTO lddirs ( dirname ) VALUES ('/lib64') ;"
      "INSERT INTO lddirs ( dirname ) VALUES ('/usr/lib') ;"
      "INSERT INTO lddirs ( dirname ) VALUES ('/usr/lib64') ;"
      ;  

    
}

//--------------------------------------------------------------------------------------------------
std::string 
CacheSQL::CreateIndexes()
{
  return
  "create index  idx_required_needed on required(needed);"
  "create index  idx_dynlinked_soname on dynlinked(soname);"
  "create index  idx_dynlinked_dirname on dynlinked(dirname);"
  ;
}
//--------------------------------------------------------------------------------------------------

std::string 
CacheSQL::InsertPkgSQL()
{
  return "INSERT INTO pkgs (fullname, name, version, arch, build, tag, timestamp)"
            " VALUES( ?, ?,?,?,?,?,? ) ; "
  ;
}
//--------------------------------------------------------------------------------------------------

std::string 
CacheSQL::InsertDynLinkedSQL()
{
  return "INSERT INTO dynlinked ( pkg_id, filename, dirname, basename, soname, arch)"
         " VALUES( ?,?,?,?,?,? ) ; "
  ;
}
//--------------------------------------------------------------------------------------------------
std::string 
CacheSQL::InsertRequiredSQL()
{
  return "INSERT INTO required ( dynlinked_id, needed )"
           " VALUES( ?,? ) ; "
    ;  
}
//--------------------------------------------------------------------------------------------------

std::string 
CacheSQL::InsertRRunPathSQL()
{
  return "INSERT INTO rrunpath ( dynlinked_id, ldpath )"
           " VALUES( ?,? ) ; "
    ;  
}
//--------------------------------------------------------------------------------------------------

std::string 
CacheSQL::DeletePkgByFullnameSQL()
{
  return "DELETE from pkgs WHERE fullname=?" ;
}
//--------------------------------------------------------------------------------------------------

std::string 
CacheSQL::MaxPkgTimeStamp()
{
  
  return "SELECT MAX(timestamp) FROM pkgs;"
  ;
}
//--------------------------------------------------------------------------------------------------


//depfinder
std::string 
CacheSQL::SearchPgkOfSoNameSQL()
{
 
  
  return 
  "SELECT fullname FROM pkgs INNER JOIN dynlinked ON pkgs.id = dynlinked.pkg_id"
  " WHERE dynlinked.soname=? AND dynlinked.arch=? "
  " AND dirname IN (SELECT dirname FROM lddirs "
  " UNION SELECT replaceOrigin( ldpath, dynlinked.dirname) from rrunpath "
  " WHERE  dynlinked_id =  dynlinked.id)"
  ";"
  ;
  
  
}
//--------------------------------------------------------------------------------------------------

std::string 
CacheSQL::SearchRequiredByLib()
{

  return
  "SELECT "
  " pkgs.fullname AS pkgname," 
  " pkgs.name AS pkg, "
  " dynlinked.filename AS dynlinked," 
  " required.needed ,"
  " dl2.filename AS filename"  
  " FROM pkgs "
  " INNER JOIN dynlinked ON pkgs.id = dynlinked.pkg_id "
  " INNER JOIN required ON dynlinked.id =  required.dynlinked_id"
  " INNER JOIN dynlinked dl2 ON dl2.soname =  required.needed AND dl2.arch = dynlinked.arch" 
  " WHERE dl2.dirname IN( "
  " SELECT dirname FROM lddirs  UNION " 
  " SELECT replaceOrigin( ldpath, dynlinked.dirname) from rrunpath "
  " INNER JOIN dynlinked ON dynlinked.id = rrunpath.dynlinked_id"
  " WHERE dynlinked.soname=?1"
  " ) "
  " AND required.needed =?1" 
  " AND dynlinked.arch=?2"    
  /*" AND pkgs.name NOT IN ("
  " SELECT name from pkgs "
  " INNER JOIN dynlinked ON dynlinked.pkg_id = pkgs.id"
  " WHERE dynlinked.soname = ?1"
  ")"*/  
  " ORDER BY  pkg , dynlinked.filename , needed"
  ";"
   ;
  // do the fileter of pkg within result, cause query a lib may return the pkg it is memeber of...
}





namespace
{

  // replaceOrigin( ld_elf_filepath, ld_homedir  )
  static void replace_origin_func(sqlite3_context *context, int argc, sqlite3_value **argv)
  {
    if (argc != 2)
      {
        static const std::string errmsg = "incorrect count of arguments, should be 2";
        sqlite3_result_error(context,errmsg.c_str(), errmsg.size() ) ;
      }
    
    std::string filepath = (const char*)sqlite3_value_text(argv[0]);
    std::string homepath = (const char*)sqlite3_value_text(argv[1]);
    
    
    
    
    sbbdep::PathName home = homepath; 
    
    using boost::algorithm::replace_first_copy; 
    std::string result=replace_first_copy(filepath,"$ORIGIN/..",home.getDir()) ;
    result=replace_first_copy(result ,"$ORIGIN", home.getURL()) ;    
    
    //void sqlite3_result_text(sqlite3_context*, const char*, int, void(*)(void*));
    sqlite3_result_text(context, result.c_str(), -1, SQLITE_TRANSIENT);
    
  }
}

void 
CacheSQL::register_replaceOrigin_function(sqlite3* db)
{
  sqlite3_create_function(db, "replaceOrigin", 2, 0,0, &replace_origin_func , 0 , 0 );
}












}
