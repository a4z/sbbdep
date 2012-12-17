

#include <a4z/testsuitebuilder.hpp>

#include <sbbdep/cachesql.hpp>
#include <sbbdep/cachedb.hpp>

#include <a4sqlt3/onevalresult.hpp>
#include <a4sqlt3/dataset.hpp>

#include <iostream>

namespace sbbdep {
namespace test_cachever {

//using namespace a4sqlt3;

class DB : public CacheDB{
  public:

  DB(): CacheDB(":memory:"){}

  sqlite3* getDb() { return m_sql3db; }

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
      a4sqlt3::OneValResult<int> cnt;
      db.Execute("select count(*) from rrunpath;", & cnt);
      BOOST_CHECK(cnt.isValid() && cnt.Val() == 3 ) ;
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

    a4sqlt3::OneValResult<std::string> wanted;
    db.Execute("select lddir from rrunpath where id=1;", &wanted);
    BOOST_CHECK(wanted.isValid() && wanted.Val() == "/usr/lib" ) ;
    wanted.Reset();
    db.Execute("select lddir from rrunpath where id=2;", &wanted);
    BOOST_CHECK(wanted.isValid() && wanted.Val() == "/usr/lib" ) ;
    wanted.Reset();
    db.Execute("select lddir from rrunpath where id=3;", &wanted);
    BOOST_CHECK(wanted.isValid() && wanted.isNull() ) ;
  }

};




struct CacheVersionSuite : public a4z::TestSuiteBuilder< Case >
{
  
  void
  assambleCases()
  {
    A4Z_TEST_ADDCLASSCASE( Case::mkTestEnviroment );
    A4Z_TEST_ADDCLASSCASE( Case::CheckAlterTable );
    A4Z_TEST_ADDCLASSCASE( Case::UpdateData );
  }
  
};
A4Z_TEST_CHECK_IN( CacheVersionSuite , up1_2 );


}
}
