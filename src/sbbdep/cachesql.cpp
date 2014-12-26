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

CREATE TRIGGER on_before_delete_pkgs BEFORE DELETE ON pkgs 
  FOR EACH ROW  BEGIN
  DELETE from dynlinked WHERE pkg_id = OLD.id;
  END;
CREATE TRIGGER on_before_delete_dynlinked BEFORE DELETE ON dynlinked 
  FOR EACH ROW  BEGIN
  DELETE from required WHERE dynlinked_id = OLD.id;
  DELETE from rrunpath WHERE dynlinked_id = OLD.id;
  END;
CREATE TABLE lddirs (dirname TEXT PRIMARY KEY NOT NULL);

CREATE TABLE ldlnkdirs (dirname TEXT PRIMARY KEY NOT NULL);      
CREATE TABLE ldusrdirs (dirname TEXT PRIMARY KEY NOT NULL); 



CREATE TABLE version ( major INTEGER NOT NULL,
 minor INTEGER NOT NULL, patchlevel INTEGER NOT NULL);

create view sbbdep_known_deplinks as
select pkgs.fullname as pkg, dynlinked.filename as file , 
required.needed as needed , d2.filename as fromfile , p2.fullname as frompkg
from
pkgs 
inner join dynlinked on pkgs.id = dynlinked.pkg_id
inner join required on dynlinked.id = required.dynlinked_id
left join rrunpath on dynlinked.id = rrunpath.dynlinked_id 
left join dynlinked d2 on required.needed = d2.soname 
inner join pkgs p2 on p2.id = d2.pkg_id
where  d2.arch = dynlinked.arch 
AND 
( 
( rrunpath.lddir IS NOT NULL AND d2.dirname not in 
    (  select distinct * from lddirs union select distinct * from ldlnkdirs )
  and rrunpath.lddir = d2.dirname )
OR 
(
d2.dirname in 
  ( select distinct * from lddirs union select distinct * from ldlnkdirs ) 
AND ( rrunpath.lddir IS  NULL OR rrunpath.lddir in 
  ( select distinct * from lddirs union select distinct * from ldlnkdirs ) )
)
)
;


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
"VALUES('" AS_STR(SBBDEP_MAJOR_VERSION) "', '" AS_STR(SBBDEP_MINOR_VERSION) "', '"
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




void createSchema(a4sqlt3::Database& db)
{ //LogDebug() << createSchema() ;
  db.execute(createSchema()) ;

}//-----------------------------------------------------------------------------

void addVersionTable(a4sqlt3::Database& db)
{
  db.execute(createVersion()) ;
}//-----------------------------------------------------------------------------


auto makeCommand(a4sqlt3::Database& db, Cache::sqlid id)
  ->a4sqlt3::SqlCommand
{

  using a4sqlt3::DbValueType ;

  switch (id)
  {
    case Cache::sqlid::insert_pkg:
      return db.command(sql::insertPkg(),{ DbValueType::Text,
                                           DbValueType::Text,
                                           DbValueType::Text,
                                           DbValueType::Text,
                                           DbValueType::Int64,
                                           DbValueType::Text,
                                           DbValueType::Int64 } );
      break;

    case Cache::sqlid::insert_dynlinked:
      return db.command(sql::insertDynLinked(), {DbValueType::Int64,
                                                DbValueType::Text,
                                                DbValueType::Text,
                                                DbValueType::Text,
                                                DbValueType::Text,
                                                DbValueType::Int64 });


      break;

    case Cache::sqlid::insert_required:
      return db.command(sql::insertRequired(), { DbValueType::Int64,
                                                  DbValueType::Text } );
      break;

    case Cache::sqlid::insert_rrunpath:
      return db.command(sql::insertRRunPath(),{ DbValueType::Int64,
                                                  DbValueType::Text,
                                                  DbValueType::Text });
      break;

    case Cache::sqlid::insert_ldDir:
      return db.command(sql::insertLdDir(), {DbValueType::Text});
      break;

    case Cache::sqlid::insert_ldLnkDir:
      return db.command(sql::insertLdLnkDir(), {DbValueType::Text});
      break;

    case Cache::sqlid::set_keyval:
      return db.command(sql::setKeyVal(), {DbValueType::Text,
                                           DbValueType::Variant});
      break;

    case Cache::sqlid::del_byfullname :
      return  db.command(sql::deletePkgByFullname(),{ DbValueType::Text });
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
      { // TODO this needs a test
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
      { // TODO this needs a test
        conststr errmsg {"incorrect count of arguments, should be 1"} ;
        sqlite3_result_error(context,errmsg.c_str(), errmsg.size() ) ;
      }

    Path p((const char*)sqlite3_value_text(argv[0]));
    p.makeAbsolute();
    p.makeRealPath();

    if ( p.isValid() )
      sqlite3_result_text(context, p.getURL().c_str(), -1, SQLITE_TRANSIENT);
    else
      sqlite3_result_null(context);
  }//---------------------------------------------------------------------------

} // anno ns

void register_own_functions(sqlite3* db)
{
  //todo  parameter 4 ?? re check docu
  //auto flag = SQLITE_UTF8 | SQLITE_DETERMINISTIC ;
  sqlite3_create_function(db, "replaceOrigin", 2, 0,0,
                          &replace_origin_func , 0 , 0 );

  sqlite3_create_function(db, "mkRealPath", 1, 0,0,
                          &make_realpath_func , 0 , 0 );
  // mkRealPath( replaceOrigin("$ORIGIN/../where/ever", dirOfFile) )

}//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
}
}




