
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <string>

#include <sbbdep/lddirs.hpp>
#include <sbbdep/error.hpp>
#include <sbbdep/pathname.hpp>
#include <sbbdep/dircontent.hpp>


struct Foo{
  enum X {a,b,c} ;
};

void a(Foo::X )
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;
}

void a(int )
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;
}


int main(int argc, char* argv[])
{
  (void)(argc) , (void)(argv);

  a(1) ;
  a(Foo::a);

  int b = Foo::a ;
  a(b);

}

