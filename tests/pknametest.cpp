

#include "a4testing.hpp"

#include <sbbdep/pkgname.hpp>

#include <iostream>

namespace sbbdep {
namespace test_pkname {



  void PrintInfo(const sbbdep::PkgName& pkn)
  {
    std::cout <<"FullName       : "<< pkn.FullName() <<"\n";
    std::cout <<"Name           : "<< pkn.Name() <<"\n";
    std::cout <<"Version        : "<< pkn.Version() <<"\n";
    std::cout <<"Arch           : "<< pkn.Arch() <<"\n";
    std::cout <<"BuildStr       : "<< pkn.BuildStr() <<"\n";
    std::cout <<"Build().Tag    : "<< pkn.Build().Tag() <<"\n";
    std::cout <<"Build().Num    : "<< pkn.Build().Num() <<"\n";
    std::cout << "------------------------------------" << std::endl; 
  }
  
  void RunDefault()
  {
    PkgName pkn1("mesa-7.8.1-i486-1");
    BOOST_REQUIRE( pkn1.Name() == "mesa" );
    BOOST_REQUIRE( pkn1.Version() == "7.8.1" );
    BOOST_REQUIRE( pkn1.Arch() == "i486" );
    BOOST_REQUIRE( pkn1.BuildStr() == "1" );
    BOOST_REQUIRE( pkn1.Build().Tag() == "" );
    BOOST_REQUIRE( pkn1.Build().Num() == 1 );
    
    PkgName pkn2 = "xvidcore-1.2.2-i686-2gv"; 
    BOOST_REQUIRE( pkn2.BuildStr() == "2gv" );
    BOOST_REQUIRE( pkn2.Build().Tag() == "gv" );
    BOOST_REQUIRE( pkn2.Build().Num() == 2 );
    
    PkgName pkn3 = pkn2; 
    BOOST_REQUIRE( pkn3.Name() == pkn2.Name() );
    BOOST_REQUIRE( pkn3.Version() == pkn2.Version() );
    BOOST_REQUIRE( pkn3.Arch() == pkn2.Arch() );
    BOOST_REQUIRE( pkn3.BuildStr() == pkn2.BuildStr() );
    BOOST_REQUIRE( pkn3.Build().Tag() == pkn2.Build().Tag() );
    BOOST_REQUIRE( pkn3.Build().Num() == pkn2.Build().Num() );


    // just test div assignments
    pkn3 = pkn1;
    BOOST_REQUIRE( pkn3.Name() == "mesa" );
    pkn3 = pkn2.FullName();
    BOOST_REQUIRE( pkn3.Version() == "1.2.2" );
    pkn3 = pkn1.FullName().c_str();
    // just test div compair stuff
    BOOST_REQUIRE( pkn3 == pkn1 );
    BOOST_REQUIRE( pkn3 == pkn1.FullName() );
    BOOST_REQUIRE( pkn3 == pkn1.FullName().c_str() );
    BOOST_REQUIRE( pkn3 == "mesa-7.8.1-i486-1" );
    
    BOOST_REQUIRE( pkn3.Name() == "mesa" );
    BOOST_REQUIRE( pkn3.Version() == "7.8.1" );
    BOOST_REQUIRE( pkn3.Arch() == "i486" );
    BOOST_REQUIRE( pkn3.BuildStr() == "1" );
    BOOST_REQUIRE( pkn3.Build().Tag() == "" );
    BOOST_REQUIRE( pkn3.Build().Num() == 1 );    
    
    // for maps
    BOOST_REQUIRE( pkn1 < pkn2 );
    
  }


  void RunExtra()
  {
    {
      PkgName pkn1("mesa-mesa-7.8.1-i486-1");
      BOOST_REQUIRE( pkn1.Name() == "mesa-mesa" );
      BOOST_REQUIRE( pkn1.Version() == "7.8.1" );
      BOOST_REQUIRE( pkn1.Arch() == "i486" );
      BOOST_REQUIRE( pkn1.BuildStr() == "1" );
      BOOST_REQUIRE( pkn1.Build().Tag() == "" );
      BOOST_REQUIRE( pkn1.Build().Num() == 1 );
    }


    {
      PkgName pkn1("mesa-mesa-mesa-7.8.1-i486-1");
      BOOST_REQUIRE( pkn1.Name() == "mesa-mesa-mesa" );
      BOOST_REQUIRE( pkn1.Version() == "7.8.1" );
      BOOST_REQUIRE( pkn1.Arch() == "i486" );
      BOOST_REQUIRE( pkn1.BuildStr() == "1" );
      BOOST_REQUIRE( pkn1.Build().Tag() == "" );
      BOOST_REQUIRE( pkn1.Build().Num() == 1 );
    }


  }



a4TestSimple("pkgname", RunDefault) ;
a4TestSimple("pkgname2", RunExtra) ;
}
}
