

#include <a4z/testsuitebuilder.hpp>
#include <a4sqlt3/database.hpp>


#include <sbbdep/cachesql.hpp>

#include <iostream>

namespace sbbdep {
namespace test_replorig {

struct TmpDB : public a4sqlt3::Database
{
  TmpDB() : a4sqlt3::Database(":memory:")
  {
  }
  
  void createExtendedFunctions()
  {
    CacheSQL::register_own_sql_functions( _sql3db );
  }
  
};


void RunDefault()
{
  TmpDB db;
  db.Open() ;
  db.createExtendedFunctions() ;
  
  a4sqlt3::DbValue result(a4sqlt3::DbValueType::Text);
  
  std::string c1sql = "SELECT replaceOrigin('$ORIGIN/../lib', '/usr/lib')" ;
  std::string c2sql = "SELECT replaceOrigin('$ORIGIN/local/lib', '/usr')" ;
  
  result = db.selectSingleValue(c1sql) ;
  
  BOOST_REQUIRE_EQUAL( result.getString() , "/usr/lib"  );
  
  result = db.selectSingleValue(c2sql) ;
  BOOST_REQUIRE_EQUAL( result.getString() , "/usr/local/lib"  );
  
  
}


struct ReplOrigSuite : public a4z::TestSuiteBuilder< >
{
  
  void
  assembleCases()
  {
    A4Z_TEST_ADDCASEFUNC( RunDefault );
  }
  
};
A4Z_TEST_CHECK_IN ( ReplOrigSuite , replorig );


}
}
