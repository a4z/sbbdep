

#include "a4testing.hpp"

#include <sbbdep/cachesql.hpp>
#include <sbbdep/cachedb.hpp>

#include <a4sqlt3/dataset.hpp>

#include <iostream>

namespace sbbdep {
namespace test_cachever {

//using namespace a4sqlt3;

class DB : public CacheDB{
  public:

  DB(): CacheDB(":memory:"){}

  sqlite3* getDb() { return _sql3db; }

  void MakeTestTables()
  {
    // shreinked down version of required tables..
    std::string sql =
    "CREATE TABLE dynlinked ( "
    "    id INTEGER PRIMARY KEY NOT NULL, "
    "    dirname TEXT NOT NULL "
    ");"
    "CREATE TABLE rrunpath( "
    "    id INTEGER PRIMARY KEY NOT NULL,"
    "    dynlinked_id INTEGER NOT NULL,     "
    "    ldpath TEXT NOT NULL "
    ");"
    ;
    Execute(sql);
  }

};

struct Case
{

  bool haveTestEnv ;
  DB db;
  Case()
  : haveTestEnv(false)
  , db()
  {}

  void mkTestEnviroment()
  {
    try
      {
        db.Open();
        db.MakeTestTables();
        CacheSQL::register_own_sql_functions(db.getDb());
        //insert some data...
        std::string sql = "insert into dynlinked(id, dirname) values(1,'/usr');"
            "insert  into rrunpath(id, dynlinked_id, ldpath) values(1,1,'$ORIGIN/lib');"
            "insert  into rrunpath(id, dynlinked_id, ldpath) values(2,1,'$ORIGIN/../usr/lib');"
            "insert  into rrunpath(id, dynlinked_id, ldpath) values(3,1,'$ORIGIN/../no/where');"
            ;
        db.Execute(sql);
      }
    catch (...)
      {
        BOOST_FAIL("mkTestEnviroment failed");
        return ;
      }
    haveTestEnv = true;
  }

  void CheckAlterTable()
  {
    BOOST_REQUIRE(haveTestEnv);

    std::string sql="ALTER TABLE rrunpath ADD COLUMN lddir TEXT;" ;
    BOOST_REQUIRE_NO_THROW(db.Execute(sql));
    {
      a4sqlt3::DbValue cnt = db.selectSingleValue("select count(*) from rrunpath;");
      BOOST_CHECK( cnt.getInt() == 3 ) ;
    }

  }

  void UpdateData()
  {
    BOOST_REQUIRE(haveTestEnv);
    std::string sql="update rrunpath "
        "set lddir = mkRealPath("
        "replaceOrigin(ldpath,(SELECT dirname FROM dynlinked WHERE id=dynlinked_id))"
        ") ;" ;
    BOOST_REQUIRE_NO_THROW(db.Execute(sql));

    a4sqlt3::DbValue wanted = db.selectSingleValue("select lddir from rrunpath where id=1;");
    BOOST_CHECK( wanted.getText() == "/usr/lib" ) ;

    wanted = db.selectSingleValue("select lddir from rrunpath where id=2;");
    BOOST_CHECK( wanted.getText() == "/usr/lib" ) ;

    wanted = db.selectSingleValue("select lddir from rrunpath where id=3;");
    BOOST_CHECK( wanted.isNull() ) ;
  }

};

/* this is obsolete, since the decision was taken to recreate the cache
a4TestAdd(
    a4test::suite<Case>("cacheup_1_2")
    .addTest("mktestenvironment", &Case::mkTestEnviroment)
    .addTest("check alter table", &Case::CheckAlterTable)
    .addTest("update data", &Case::UpdateData)
    );
*/

}
}
