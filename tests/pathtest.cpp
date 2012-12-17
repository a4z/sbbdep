

#include <a4z/testsuitebuilder.hpp>

#include <sbbdep/path.hpp>

#include <string>
#include <iostream>
#include <cstdio>

namespace sbbdep {
namespace test_path {

// move this func to lib if required...
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
//--------------------------------------------------------------------------------------------------
void
TestFindInPath()
{
  sbbdep::Path path = FindBinPath("ldd");
  
  BOOST_CHECK(path.isValid()) ;
  
  sbbdep::Path path1 = FindBinPath("doesnotexist");
  BOOST_CHECK(!path1.isValid()) ;
  
  
  
}
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------


void TestDefaults()
{
  Path p1;
  BOOST_REQUIRE( p1.isEmpty() ) ;
  BOOST_REQUIRE( !p1.isValid() ) ;
  
  // take some file that should exist...
  p1 = "/etc/fstab" ;
  BOOST_REQUIRE( !p1.isEmpty() ) ;
  BOOST_REQUIRE( p1.isValid() ) ;  
  BOOST_REQUIRE( p1.isRegularFile() ) ;
  
  PathName pn = p1; 
  
  BOOST_REQUIRE( pn.isPath() ) ;
  BOOST_REQUIRE( pn.isAbsolute() ) ;
  BOOST_REQUIRE( !pn.isRelative() ) ;
  
  BOOST_REQUIRE( pn == p1 ) ;
  BOOST_REQUIRE( p1 == pn ) ;
  
}
//--------------------------------------------------------------------------------------------------

void TestDirBase()
{
/*
"/usr/lib"    "/usr"    "lib"
"/usr/"       "/"       "usr"
"usr"         "."       "usr"
"/"           "/"       "/"
"."           "."       "."
".."          "."       ".."
*/
  
  Path p;
  p= "/usr/lib";
  BOOST_REQUIRE (p.getDir() ==  "/usr" ) ;
  BOOST_REQUIRE (p.getBase() ==  "lib" ) ;
  
  p= "/usr/";
  BOOST_REQUIRE (p.getDir() ==  "/" ) ;
  BOOST_REQUIRE (p.getBase() ==  "usr" ) ;  
  
  p= "usr";
  BOOST_REQUIRE (p.getDir() ==  "." ) ;
  BOOST_REQUIRE (p.getBase() ==  "usr" ) ;  
  
  p= ".";
  BOOST_REQUIRE (p.getDir() ==  "." ) ;
  BOOST_REQUIRE (p.getBase() ==  "." ) ;  

  p= "..";
  BOOST_REQUIRE (p.getDir() ==  "." ) ;
  BOOST_REQUIRE (p.getBase() ==  ".." ) ;  
  
}
//--------------------------------------------------------------------------------------------------

void TestDiv()
{
  
  PathName pn = "nix" ;
  
  BOOST_REQUIRE ( !pn.isAbsolute()  ) ;
  BOOST_REQUIRE ( !pn.isRelative()  ) ;
  BOOST_REQUIRE ( !pn.isPath()  ) ;
  
  PathName pathtonowhere = "/etc/../path/to/nowhere" ;
  {
    Path p(pathtonowhere);
    BOOST_REQUIRE_NO_THROW( p.makeAbsolute() );
    BOOST_REQUIRE_NO_THROW( p.makeRealPath() );
    BOOST_REQUIRE ( !p.isAbsolute()  ) ;
    BOOST_REQUIRE ( !p.isRelative()  ) ;
    BOOST_REQUIRE ( !p.isPath()  ) ;
    BOOST_REQUIRE ( !p.isValid() ) ;
  }
  {
    Path p(pathtonowhere);
    BOOST_CHECK(p.makeRealPath()==false);
  }
  {
    Path p(pathtonowhere);
    BOOST_CHECK(p.makeAbsolute()==false);
  }
  {
    Path p(pathtonowhere);
    BOOST_CHECK(p.makeRealPath()==false);
    BOOST_CHECK(p.makeAbsolute()==false);
  }

}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
struct PathSuite : public a4z::TestSuiteBuilder< >
{
  
  void
  assambleCases()
  {
    A4Z_TEST_ADDCASEFUNC( TestDefaults );
    A4Z_TEST_ADDCASEFUNC( TestDirBase );
    
    A4Z_TEST_ADDCASEFUNC( TestDiv );
    
    A4Z_TEST_ADDCASEFUNC( TestFindInPath );
    
  }
  
};
A4Z_TEST_CHECK_IN ( PathSuite , path );

}
}
