

#include "a4testing.hpp"

#include "sbbdep/cache.hpp"



namespace sbbdep {
namespace test_replorig {



void CheckFunctions()
{
  
  Cache c{"memory:"};

  a4sqlt3::DbValue result(a4sqlt3::DbValueType::Text);
  
  std::string c1sql = "SELECT replaceOrigin('$ORIGIN/../lib', '/usr/lib')" ;
  std::string c2sql = "SELECT replaceOrigin('$ORIGIN/local/lib', '/usr')" ;
  
  BOOST_REQUIRE_NO_THROW(result = c.selectValue(c1sql)) ;
  
  BOOST_REQUIRE_EQUAL( result.getText() , "/usr/lib"  );
  
  BOOST_REQUIRE_NO_THROW(result = c.selectValue(c2sql)) ;
  BOOST_REQUIRE_EQUAL( result.getText() , "/usr/local/lib"  );

  
}

a4TestSimple("replorig", CheckFunctions ) ;

}
}
