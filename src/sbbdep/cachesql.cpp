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
  " AND dirname IN (SELECT dirname FROM lddirs);"
  ;
}
//--------------------------------------------------------------------------------------------------



}
