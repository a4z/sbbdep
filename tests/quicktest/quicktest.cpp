
#include <iostream>
#include <iomanip>
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

  std::cout << std::setw(10) << std::left
                << "foo" << std::setw(0) << "bar" << std::endl;

}

