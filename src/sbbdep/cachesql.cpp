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


#include <sbbdep/cachesql.hpp>

#include <sbbdep/pathname.hpp>
#include <sbbdep/path.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <sbbdep/config.hpp> // generated
#include <sqlite3.h>

#include <sstream>

namespace sbbdep {



// cache stuff 
std::string 
CacheSQL::CreateSchemaSQL()
{
  
  
  std::string sql= R"~( 
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
        
)~";

  sql+=CreateVersion(MAJOR_VERSION , MINOR_VERSION , PATCH_VERSION);
  return sql;


  /*    INSERT INTO lddirs ( dirname ) VALUES ('/lib') ;
        INSERT INTO lddirs ( dirname ) VALUES ('/lib64') ;
        INSERT INTO lddirs ( dirname ) VALUES ('/usr/lib') ;
        INSERT INTO lddirs ( dirname ) VALUES ('/usr/lib64') ;
  // moved to lddirs class
  */

}

//--------------------------------------------------------------------------------------------------

std::string
CacheSQL::CreateVersion(int major, int minor, int patchlevel)
{

  std::stringstream sql ;
  sql << "CREATE TABLE version (major INTEGER NOT NULL, minor INTEGER NOT NULL, patchlevel INTEGER NOT NULL);"
      << "INSERT INTO VERSION (major, minor, patchlevel) "
      <<    "VALUES(" << major << "," << minor << "," << patchlevel << ");";

  return sql.str();
}
//--------------------------------------------------------------------------------------------------

std::string
CacheSQL::CheckVersion(int major, int minor, int patchlevel)
{
  std::stringstream sql (
      "SELECT * FROM version"
  );
  sql << " WHERE "
      << " major = " << major
      << " AND minor = " << minor
      << " AND patchlevel = " << patchlevel
      << ";";

  return sql.str();
}
//--------------------------------------------------------------------------------------------------

constexpr const char*
CacheSQL::CreateViews()
{
  // for future, will need this ...
  return R"~(

)~";
}
//--------------------------------------------------------------------------------------------------

std::string 
CacheSQL::CreateIndexes()
{
  return R"~(
  create index  idx_pkgs_fullname on pkgs(fullname); 
  
  create index  idx_dynlinked_soname on dynlinked(soname);
  create index  idx_dynlinked_pkg_id on dynlinked(pkg_id);
  create index  idx_dynlinked_dirname on dynlinked(dirname);

  create index  idx_required_dynlinked_id on required(dynlinked_id);
  create index  idx_required_needed on required(needed);

  create index  idx_rrunpath_dynlinked_id on rrunpath(dynlinked_id);
  

)~";
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
  return "INSERT INTO rrunpath ( dynlinked_id, ldpath, lddir )"
           " VALUES( ?1,?2, mkRealPath( replaceOrigin(?2, ?3) ) ) ; "
    ;  
}
//--------------------------------------------------------------------------------------------------

std::string 
CacheSQL::InsertLdDirSQL()
{
  return "INSERT INTO lddirs ( dirname ) VALUES(?)" ;
}
//--------------------------------------------------------------------------------------------------

std::string 
CacheSQL::InsertLdLnkDirSQL()
{
  return "INSERT INTO ldlnkdirs ( dirname ) VALUES(?)" ;
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
  return R"~(
SELECT fullname FROM pkgs INNER JOIN dynlinked ON pkgs.id = dynlinked.pkg_id
 WHERE dynlinked.soname=? AND dynlinked.arch=? 
 AND dirname IN (SELECT dirname FROM lddirs 
 UNION SELECT dirname FROM ldlnkdirs UNION SELECT dirname FROM ldusrdirs
 UNION SELECT lddir from rrunpath 
 WHERE  dynlinked_id =  dynlinked.id 
  AND rrunpath.lddir IS NOT NULL 
  AND rrunpath.lddir NOT IN 
    ( SELECT dirname FROM lddirs UNION SELECT dirname FROM ldlnkdirs UNION SELECT dirname FROM ldusrdirs )
)  ;

)~";
  
  
}
//--------------------------------------------------------------------------------------------------

std::string 
CacheSQL::SearchRequiredByLib()
{

  return R"~(
SELECT 
 pkgs.fullname AS pkgname, 
 pkgs.name AS pkg, 
 dynlinked.filename AS dynlinked, 
 required.needed ,
 dl2.filename AS filename  
 FROM pkgs 
 INNER JOIN dynlinked ON pkgs.id = dynlinked.pkg_id 
 INNER JOIN required ON dynlinked.id =  required.dynlinked_id
 INNER JOIN dynlinked dl2 ON dl2.soname =  required.needed AND dl2.arch = dynlinked.arch 
 WHERE dl2.dirname IN( 
 SELECT dirname FROM lddirs  
 UNION SELECT dirname FROM ldlnkdirs 
 UNION SELECT dirname FROM ldusrdirs
 UNION SELECT lddir from rrunpath 
 INNER JOIN dynlinked ON dynlinked.id = rrunpath.dynlinked_id
 WHERE dynlinked.soname=?1 AND rrunpath.lddir IS NOT NULL 
 AND rrunpath.lddir NOT IN 
   ( SELECT dirname FROM lddirs UNION SELECT dirname FROM ldlnkdirs UNION SELECT dirname FROM ldusrdirs )
 ) 
 AND required.needed =?1 
 AND dynlinked.arch=?2
;
  
)~";

  /*" AND pkgs.name NOT IN ("
  " SELECT name from pkgs "
  " INNER JOIN dynlinked ON dynlinked.pkg_id = pkgs.id"
  " WHERE dynlinked.soname = ?1"
  ")"*/
/*  " ORDER BY  pkg , dynlinked.filename , needed" */


// for special filter or sort, just replace the last ";" with the wanted stuff, no need to do it here
}






std::string
CacheSQL::replaceORIGIN(const std::string& originstr, const std::string& fromfile)
{
  sbbdep::PathName destfile(fromfile);
  using boost::algorithm::replace_first_copy;
  std::string result=replace_first_copy(originstr,"$ORIGIN/..",destfile.getDir()) ;
  return replace_first_copy(result ,"$ORIGIN", destfile.getURL()) ;
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
    
    std::string result=CacheSQL::replaceORIGIN(filepath, homepath);
    
    //void sqlite3_result_text(sqlite3_context*, const char*, int, void(*)(void*));
    sqlite3_result_text(context, result.c_str(), -1, SQLITE_TRANSIENT);
    
  }

  static void make_realpath_func(sqlite3_context *context, int argc, sqlite3_value **argv)
  {
    if (argc != 1)
      {
        static const std::string errmsg = "incorrect count of arguments, should be 1";
        sqlite3_result_error(context,errmsg.c_str(), errmsg.size() ) ;
      }

    Path p((const char*)sqlite3_value_text(argv[0]));
    p.makeAbsolute();
    p.makeRealPath();

    if ( p.isValid() )
      sqlite3_result_text(context, p.getURL().c_str(), -1, SQLITE_TRANSIENT);
    else
      sqlite3_result_null(context);
  }

}

void 
CacheSQL::register_own_sql_functions(sqlite3* db)
{
  sqlite3_create_function(db, "replaceOrigin", 2, 0,0, &replace_origin_func , 0 , 0 );
  sqlite3_create_function(db, "mkRealPath", 1, 0,0, &make_realpath_func , 0 , 0 );
  // mkRealPath( replaceOrigin("$ORIGIN/../where/ever", dirOfFile) )

}












}
