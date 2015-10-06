

#include "a4testing.hpp"

#include "sbbdep/cache.hpp"



namespace sbbdep {
namespace test_replorig {



void CheckFunctions()
{
  
  Cache c{"memory:"};

  sl3::DbValue result(sl3::Type::Text);
  
  std::string c1sql = "SELECT replaceOrigin('$ORIGIN/../lib', '/usr/lib')" ;
  std::string c2sql = "SELECT replaceOrigin('$ORIGIN/local/lib', '/usr')" ;
  
  BOOST_REQUIRE_NO_THROW(result = c.selectValue(c1sql)) ;
  
  BOOST_REQUIRE_EQUAL( result.getText() , "/usr/lib"  );
  
  BOOST_REQUIRE_NO_THROW(result = c.selectValue(c2sql)) ;
  BOOST_REQUIRE_EQUAL( result.getText() , "/usr/local/lib"  );

  
  std::string c3sql = "SELECT mkRealPath('/usr/local/../local/bin');" ;

  BOOST_REQUIRE_NO_THROW(result = c.selectValue(c3sql)) ;
  BOOST_REQUIRE_EQUAL( result.getText() , "/usr/local/bin"  );


}


a4TestSimple("replorig", CheckFunctions ) ;

}
}
