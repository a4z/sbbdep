

#include "a4z/testsuitebuilder.hpp"

#include "sbbdep/path.hpp"

#include <string>
#include <iostream>
#include <cstdio>

namespace sbbdep {
namespace test_findbinbath {

sbbdep::Path
FindBinPath( const std::string& binname )
{
  const std::string envpath = std::getenv("PATH") ;
  //const std::string envpath = "foo:bar:" ;
  //const std::string envpath = "foo:bar" ;
  //const std::string envpath = "foo::::bar" ;
  //const std::string envpath = ":" ;
  //const std::string envpath = ":foo:bar" ;
  //const std::string envpath = ":foo:" ;
  //const std::string envpath = "" ;
  
  std::size_t spos = 0, epos = std::string::npos;
  do
    {
      std::string bindir;
      epos = envpath.find(":", spos);
      if (epos != std::string::npos)
        {
          bindir = envpath.substr(spos, epos - spos) ;
          spos = epos + 1;
        }
      else if (spos < envpath.length())
        { // get also bar out of "/foo:/bar" but do nothing if just "/foo:"
          bindir = envpath.substr(spos, epos) ;
        }

      if ( !bindir.empty())
        {
          sbbdep::Path binpath( bindir + "/" + binname ); 
          if ( binpath.isRegularFile() && binpath.isUserX() ) return binpath ; 
        }
    }
  while (epos != std::string::npos);

  // if noting found so far, return an invalid Path;
  return sbbdep::Path(""); 
}

void
RunDefault()
{
  sbbdep::Path path = FindBinPath("ldd");
  
  BOOST_CHECK(path.isValid()) ;
  
  sbbdep::Path path1 = FindBinPath("doesnotexist");
  BOOST_CHECK(!path1.isValid()) ;
  
}

struct FindBinPathSuite : public a4z::TestSuiteBuilder< >
{
  
  void
  assambleCases()
  {
    A4Z_TEST_ADDCASEFUNC( RunDefault );
  }
  
};
A4Z_TEST_CHECK_IN ( FindBinPathSuite , findbinpath );

}
}
