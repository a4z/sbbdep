

#include "a4testing.hpp"

#include <sbbdep/pkgname.hpp>

#include <iostream>

namespace sbbdep {
namespace test_pkname {



  void PrintInfo(const sbbdep::PkgName& pkn)
  {
    std::cout <<"FullName       : "<< pkn.fullName() <<"\n";
    std::cout <<"Name           : "<< pkn.name() <<"\n";
    std::cout <<"Version        : "<< pkn.version() <<"\n";
    std::cout <<"Arch           : "<< pkn.arch() <<"\n";
    std::cout <<"BuildStr       : "<< pkn.buildStr() <<"\n";
    std::cout <<"Build().Tag    : "<< pkn.build().Tag() <<"\n";
    std::cout <<"Build().Num    : "<< pkn.build().Num() <<"\n";
    std::cout << "------------------------------------" << std::endl; 
  }
  
  void RunDefault()
  {
    PkgName pkn1("mesa-7.8.1-i486-1");
    BOOST_REQUIRE( pkn1.name() == "mesa" );
    BOOST_REQUIRE( pkn1.version() == "7.8.1" );
    BOOST_REQUIRE( pkn1.arch() == "i486" );
    BOOST_REQUIRE( pkn1.buildStr() == "1" );
    BOOST_REQUIRE( pkn1.build().Tag() == "" );
    BOOST_REQUIRE( pkn1.build().Num() == 1 );
    
    PkgName pkn2 = "xvidcore-1.2.2-i686-2gv"; 
    BOOST_REQUIRE( pkn2.buildStr() == "2gv" );
    BOOST_REQUIRE( pkn2.build().Tag() == "gv" );
    BOOST_REQUIRE( pkn2.build().Num() == 2 );
    
    PkgName pkn3 = pkn2; 
    BOOST_REQUIRE( pkn3.name() == pkn2.name() );
    BOOST_REQUIRE( pkn3.version() == pkn2.version() );
    BOOST_REQUIRE( pkn3.arch() == pkn2.arch() );
    BOOST_REQUIRE( pkn3.buildStr() == pkn2.buildStr() );
    BOOST_REQUIRE( pkn3.build().Tag() == pkn2.build().Tag() );
    BOOST_REQUIRE( pkn3.build().Num() == pkn2.build().Num() );


    // just test div assignments
    pkn3 = pkn1;
    BOOST_REQUIRE( pkn3.name() == "mesa" );
    pkn3 = pkn2.fullName();
    BOOST_REQUIRE( pkn3.version() == "1.2.2" );
    pkn3 = pkn1.fullName().c_str();
    // just test div compair stuff
    BOOST_REQUIRE( pkn3 == pkn1 );
    BOOST_REQUIRE( pkn3 == pkn1.fullName() );
    BOOST_REQUIRE( pkn3 == pkn1.fullName().c_str() );
    BOOST_REQUIRE( pkn3 == "mesa-7.8.1-i486-1" );
    
    BOOST_REQUIRE( pkn3.name() == "mesa" );
    BOOST_REQUIRE( pkn3.version() == "7.8.1" );
    BOOST_REQUIRE( pkn3.arch() == "i486" );
    BOOST_REQUIRE( pkn3.buildStr() == "1" );
    BOOST_REQUIRE( pkn3.build().Tag() == "" );
    BOOST_REQUIRE( pkn3.build().Num() == 1 );    
    
    // for maps
    BOOST_REQUIRE( pkn1 < pkn2 );
    
  }


  void RunExtra()
  {
    {
      PkgName pkn1("mesa-mesa-7.8.1-i486-1");
      BOOST_REQUIRE( pkn1.name() == "mesa-mesa" );
      BOOST_REQUIRE( pkn1.version() == "7.8.1" );
      BOOST_REQUIRE( pkn1.arch() == "i486" );
      BOOST_REQUIRE( pkn1.buildStr() == "1" );
      BOOST_REQUIRE( pkn1.build().Tag() == "" );
      BOOST_REQUIRE( pkn1.build().Num() == 1 );
    }


    {
      PkgName pkn1("mesa-mesa-mesa-7.8.1-i486-1");
      BOOST_REQUIRE( pkn1.name() == "mesa-mesa-mesa" );
      BOOST_REQUIRE( pkn1.version() == "7.8.1" );
      BOOST_REQUIRE( pkn1.arch() == "i486" );
      BOOST_REQUIRE( pkn1.buildStr() == "1" );
      BOOST_REQUIRE( pkn1.build().Tag() == "" );
      BOOST_REQUIRE( pkn1.build().Num() == 1 );
    }


  }



a4TestSimple("pkgname", RunDefault) ;
a4TestSimple("pkgname2", RunExtra) ;
}
}
