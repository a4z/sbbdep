

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


void pkgDiffName()
{

  SyncData::StringVec v1 =
      {"a-1.2.3-x86_64-1",
        "b-1.2.3-x86_64-1",
        "b-1.2.3-x86_64-2" } ;

  SyncData::StringVec v2 =
       {"a-1.2.4-x86_64-1",
         "b-1.2.3-x86_64-2"
       } ;

  auto diff = SyncData::pkgNameDiff (v1, v2) ;
  BOOST_CHECK (not diff.empty()) ;

  BOOST_CHECK (diff == SyncData::StringVec {"b-1.2.3-x86_64-2"}) ;

  auto diff2 = SyncData::pkgNameDiff (v2, v1) ;
  BOOST_CHECK (diff2.empty ()) ;

  auto same = SyncData::pkgNameDiff (v1, diff) ;


  BOOST_CHECK(same.size () == v2.size ()) ;

  auto mismatch = std::mismatch (begin(same), end(same), begin(v2),
                                  [](const std::string& a,const std::string& b)
  {
    return PkgName(a).name() == PkgName(b).name() ;
  }
  );

  BOOST_CHECK(mismatch.first == end(same)) ;
  BOOST_CHECK(mismatch.second == end(v2)) ;
}


a4TestAdd(
    a4test::suite("cachestuff")
   // .addTest("ownFuncs", ownFuncs)
    .addTest("pkgDiffName", pkgDiffName)
    );

}
}
