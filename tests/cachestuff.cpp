

#include "a4testing.hpp"

#include <sbbdep/cache.hpp>



namespace sbbdep {
namespace test_cachever {


void ownFuncs()
{
  Cache c (":memory:") ;

  auto v = c.selectValue (
      "SELECT replaceOrigin( '$ORIGIN/../foo/bar', '/foo/bar')" ) ;

  BOOST_CHECK(v.getText() == "/foo/bar/../foo/bar") ;




}


a4TestAdd(
    a4test::suite("pathtests")
    .addTest("ownFuncs", ownFuncs)

    );

}
}
