/*
--------------Copyright (c) 2010-2018 H a r a l d  A c h i t z---------------
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



#include "cachesql.hpp"
#include "conststr.hpp"

#include <sbbdep/config.hpp>
#include <sbbdep/elffile.hpp>
#include <sbbdep/path.hpp>
#include <sbbdep/log.hpp>
#include <sqlite3.h>



#define STRINGIZE(A) #A
#define AS_STR(A) STRINGIZE(A)



namespace sbbdep {

namespace sql {

constexpr const char*
createSchema()
{

return R"~( 
CREATE TABLE pkgs (
    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, 
    fullname TEXT NOT NULL, 
    name TEXT NOT NULL, 
    version TEXT NOT NULL, 
    arch TEXT NOT NULL, 
    build INTEGER NOT NULL, 
    tag TEXT, 
    timestamp INTEGER NOT NULL 
);
CREATE TABLE dynlinked ( 
    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, 
    pkg_id INTEGER NOT NULL,       
    filename TEXT NOT NULL, 
    dirname TEXT NOT NULL, 
    basename TEXT NOT NULL, 
    soname TEXT , 
    arch INTEGER NOT NULL 
);
CREATE TABLE required ( 
    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, 
    dynlinked_id INTEGER NOT NULL, 
    needed TEXT NOT NULL
);      
CREATE TABLE rrunpath(
    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    dynlinked_id INTEGER NOT NULL,
    ldpath TEXT NOT NULL,
    lddir TEXT
);
CREATE TABLE keyvalstore (
    key Text UNIQUE NOT NULL,
    value  NOT NULL
);

CREATE TABLE ldlinks(
    dynlinked_id INTEGER PRIMARY KEY NOT NULL,
    dirname TEXT NOT NULL
);

CREATE TRIGGER on_before_delete_pkgs BEFORE DELETE ON pkgs
  FOR EACH ROW  BEGIN
  DELETE from dynlinked WHERE pkg_id = OLD.id;
  END;
CREATE TRIGGER on_before_delete_dynlinked BEFORE DELETE ON dynlinked
  FOR EACH ROW  BEGIN
  DELETE from required WHERE dynlinked_id = OLD.id;
  DELETE from rrunpath WHERE dynlinked_id = OLD.id;
  DELETE from ldlinks WHERE dynlinked_id = OLD.id;
  END;
CREATE TABLE lddirs (dirname TEXT PRIMARY KEY NOT NULL);

CREATE TABLE ldlnkdirs (dirname TEXT PRIMARY KEY NOT NULL);
CREATE TABLE ldusrdirs (dirname TEXT PRIMARY KEY NOT NULL);



CREATE TABLE version ( major INTEGER NOT NULL,
 minor INTEGER NOT NULL, patchlevel INTEGER NOT NULL);



create index  idx_pkgs_fullname on pkgs(fullname); 

create index  idx_dynlinked_soname on dynlinked(soname);
create index  idx_dynlinked_pkg_id on dynlinked(pkg_id);
create index  idx_dynlinked_dirname on dynlinked(dirname);
create index  idx_dynlinked_basename on dynlinked(basename);

create index  idx_required_dynlinked_id on required(dynlinked_id);
create index  idx_required_needed on required(needed);

create index  idx_rrunpath_dynlinked_id on rrunpath(dynlinked_id);
create index  idx_rrunpath_lddir on rrunpath(lddir);

INSERT INTO keyvalstore (key , value) VALUES ('ldsoconf', 0);        


)~"
"INSERT INTO VERSION (major, minor, patchlevel) "
"VALUES('" AS_STR(SBBDEP_MAJOR_VERSION) "', '"
 AS_STR(SBBDEP_MINOR_VERSION) "', '"
 AS_STR(SBBDEP_PATCH_VERSION) "');"

;

}//-----------------------------------------------------------------------------


constexpr const char*
selectVersion()
{
  return R"~(
SELECT * FROM version WHERE
major = ? AND minor = ? AND patchlevel = ? ;
)~" ;

}//-----------------------------------------------------------------------------


constexpr const char*
createVersion()
{
  return R"~(
CREATE TABLE version ( major INTEGER NOT NULL,
 minor INTEGER NOT NULL, patchlevel INTEGER NOT NULL); )~"
"INSERT INTO VERSION (major, minor, patchlevel) "
"VALUES('" AS_STR(SBBDEP_MAJOR_VERSION) "','" AS_STR(SBBDEP_MINOR_VERSION) "','"
 AS_STR(SBBDEP_PATCH_VERSION) "');"
 ;

}//-----------------------------------------------------------------------------


constexpr const char* insertPkg()
{
  return "INSERT INTO pkgs "
      " (fullname, name, version, arch, build, tag, timestamp) "
            " VALUES( ?, ?,?,?,?,?,? ) ; "
  ;
}//-----------------------------------------------------------------------------


constexpr const char* insertDynLinked()
{
  return "INSERT INTO dynlinked "
      "( pkg_id, filename, dirname, basename, soname, arch) "
         " VALUES( ?,?,?,?,?,? ) ; "
  ;
}//-----------------------------------------------------------------------------


constexpr const char* insertRequired()
{
  return "INSERT INTO required ( dynlinked_id, needed )"
           " VALUES( ?,? ) ; "
    ;
}//-----------------------------------------------------------------------------


constexpr const char* insertRRunPath()
{
  return "INSERT INTO rrunpath ( dynlinked_id, ldpath, lddir )"
           " VALUES( ?1,?2, mkRealPath( replaceOrigin(?2, ?3) ) ) ; "
    ;
}//-----------------------------------------------------------------------------


constexpr const char* insertLdDir()
{
  return "INSERT OR IGNORE INTO lddirs ( dirname ) VALUES(?)" ;
}//-----------------------------------------------------------------------------


constexpr const char* insertLdLnkDir()
{
  return "INSERT OR IGNORE INTO ldlnkdirs ( dirname ) VALUES(?)" ;
}//-----------------------------------------------------------------------------

constexpr const char* insertKeyVal()
{
  return "INSERT INTO ldlnkdirs ( dirname ) VALUES(?)" ;
}//-----------------------------------------------------------------------------


  constexpr const char* insertLdLinks()
  { // file name will be dirname + soname from dynlinks
    return "INSERT INTO ldlinks dynlinked_id, dirname VALUES(?, ?)";
  }


  constexpr const char* deletePkgByFullname()
{
  return "DELETE from pkgs WHERE fullname=?" ;
}//-----------------------------------------------------------------------------




constexpr const char* SearchPgkOfFile()
{
  return R"~(
SELECT fullname FROM pkgs INNER JOIN dynlinked ON pkgs.id = dynlinked.pkg_id
 WHERE dynlinked.dirname=? AND dynlinked.basename=? AND dynlinked.arch=? ; 
)~";
}//-----------------------------------------------------------------------------

// note used
constexpr const char* insertKeyValStore()
{
  return "INSERT INTO keyvalstore (key, value) VALUES ( ?1, ?2);" ;
}//-----------------------------------------------------------------------------
// note used
constexpr const char* updateKeyValStore()
{
  return "UPDATE keyvalstore SET value = ?2 WHERE key = ?1;" ;
}//-----------------------------------------------------------------------------


constexpr const char* setKeyVal()
{
  return "INSERT OR REPLACE INTO keyvalstore (key, value) VALUES ( ?1, ?2)";

}//-----------------------------------------------------------------------------




void createSchema(sl3::Database& db)
{ //LogDebug() << createSchema() ;
  db.execute(createSchema()) ;

}//-----------------------------------------------------------------------------

void addVersionTable(sl3::Database& db)
{
  db.execute(createVersion()) ;
}//-----------------------------------------------------------------------------


auto makeCommand(sl3::Database& db, Cache::sqlid id)
  ->sl3::Command
{

  using sl3::Type ;

  switch (id)
  {
    case Cache::sqlid::insert_pkg:
      return db.prepare(sql::insertPkg(),{ Type::Text,
                                           Type::Text,
                                           Type::Text,
                                           Type::Text,
                                           Type::Int,
                                           Type::Text,
                                           Type::Int } );
      break;

    case Cache::sqlid::insert_dynlinked:
      return db.prepare(sql::insertDynLinked(), {Type::Int,
                                                Type::Text,
                                                Type::Text,
                                                Type::Text,
                                                Type::Text,
                                                Type::Int });


      break;

    case Cache::sqlid::insert_required:
      return db.prepare(sql::insertRequired(), { Type::Int,
                                                  Type::Text } );
      break;

    case Cache::sqlid::insert_rrunpath:
      return db.prepare(sql::insertRRunPath(),{ Type::Int,
                                                  Type::Text,
                                                  Type::Text });
      break;

    case Cache::sqlid::insert_ldDir:
      return db.prepare(sql::insertLdDir(), {Type::Text});
      break;

    case Cache::sqlid::insert_ldLnkDir:
      return db.prepare(sql::insertLdLnkDir(), {Type::Text});
      break;

    case Cache::sqlid::insert_ldLinks:
      return db.prepare(sql::insertLdLinks (), {Type::Int,
                                                Type::Text});
      break;

    case Cache::sqlid::set_keyval:
      return db.prepare(sql::setKeyVal(), {Type::Text,
                                           Type::Variant});
      break;

    case Cache::sqlid::del_byfullname :
      return  db.prepare(sql::deletePkgByFullname(),{ Type::Text });
      break;

    default:
      throw ErrUnexpected("should never happen :-)");
  }

}//-----------------------------------------------------------------------------



namespace {

  void
  replace_origin_func(sqlite3_context* context,
                      int              argc,
                      sqlite3_value**  argv)
  {
    if (argc != 2)
      {
        conststr errmsg { "incorrect count of arguments, should be 2" };
        sqlite3_result_error(context,errmsg.c_str(), errmsg.size() ) ;
      }

    std::string filepath = (const char*)sqlite3_value_text(argv[0]);
    std::string homepath = (const char*)sqlite3_value_text(argv[1]);

    std::string result= replaceORIGIN(filepath, homepath);

    sqlite3_result_text(context, result.c_str(), -1, SQLITE_TRANSIENT);

  }//---------------------------------------------------------------------------


  void
  make_realpath_func(sqlite3_context* context,
                     int              argc,
                     sqlite3_value**  argv)
  {
    if (argc != 1)
      {
        conststr errmsg {"incorrect count of arguments, should be 1"} ;
        sqlite3_result_error(context,errmsg.c_str(), errmsg.size() ) ;
      }

    Path p((const char*)sqlite3_value_text(argv[0]));
    p.makeRealPath();

    if ( p.isValid() )
      sqlite3_result_text(context, p.str().c_str(), -1, SQLITE_TRANSIENT);
    else
      sqlite3_result_null(context);
  }//---------------------------------------------------------------------------

} // anno ns

void register_own_functions(sqlite3* db)
{
  // parameter 4 , this is only in newer version
  // possible I should force using the internal db ...

  sqlite3_create_function(db,
                          "replaceOrigin",
                          2,
                          SQLITE_UTF8 /*| SQLITE_DETERMINISTIC*/,
                          0,
                          &replace_origin_func , 0 , 0 );

  sqlite3_create_function(db,
                          "mkRealPath",
                          1,
                          SQLITE_UTF8 /*| SQLITE_DETERMINISTIC*/,
                          0,
                          &make_realpath_func , 0 , 0 );
  // mkRealPath( replaceOrigin("$ORIGIN/../where/ever", dirOfFile) )

}//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
}
}




