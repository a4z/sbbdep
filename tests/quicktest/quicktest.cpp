
#include <iostream>
#include <chrono>
#include <ctime>
#include <string>

#include <sbbdep/lddirs.hpp>
#include <sbbdep/error.hpp>
#include <sbbdep/pathname.hpp>
#include <sbbdep/dircontent.hpp>





int main(int argc, char* argv[])
{
  (void)(argc) , (void)(argv);

  try
  {
      int x = 5 ;
      SBBASSERT( x < 3 ) ;
  }
  catch (const sbbdep::Error& e)
  {
      std::cerr << e << "\n";
  }
}

