

#include "a4z/testsuitebuilder.hpp"
#include "a4sqlt3/database.hpp"
#include "a4sqlt3/onevalresult.hpp"
#include "sbbdep/cachesql.hpp"

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
    CacheSQL::register_replaceOrigin_function( m_sql3db );
  }
  
};


void RunDefault()
{
  TmpDB db;
  db.Open() ;
  db.createExtendedFunctions() ;
  
  a4sqlt3::OneValResult<std::string> result; 
  
  std::string c1sql = "SELECT replaceOrigin('$ORIGIN/../lib', '/usr/lib')" ;
  std::string c2sql = "SELECT replaceOrigin('$ORIGIN/local/lib', '/usr')" ;
  
  db.Execute(c1sql, &result) ;
  
  BOOST_REQUIRE_EQUAL( result.Val() , "/usr/lib"  );
  
  result.Reset();
  
  db.Execute(c2sql, &result) ;
  BOOST_REQUIRE_EQUAL( result.Val() , "/usr/local/lib"  );
  
  
}


struct ReplOrigSuite : public a4z::TestSuiteBuilder< >
{
  
  void
  assambleCases()
  {
    A4Z_TEST_ADDCASEFUNC( RunDefault );
  }
  
};
A4Z_TEST_CHECK_IN ( ReplOrigSuite , replorig );


}
}
