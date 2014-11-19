
#include <iostream>
#include <chrono>
#include <ctime>
#include <string>

#include <sbbdep/lddirs.hpp>

#include <sbbdep/dircontent.hpp>



template<typename T>
void printCointainer(const T& c, const std::string& del = "\n"){
  for(auto&& v : c)
    std::cout << v << del ;
}

int main(int argc, char* argv[])
{
  (void)(argc) , (void)(argv);

  std::chrono::time_point<std::chrono::system_clock> start, end;
  start = std::chrono::system_clock::now();


  auto ldd = sbbdep::getLDDirs();
  auto a = ldd.getLdDirs();
  auto b = ldd.getLdLnkDirs();

  end = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = end-start;
  std::time_t end_time = std::chrono::system_clock::to_time_t(end);

  std::cout << "finished computation at " << std::ctime(&end_time)
            << "\nelapsed time: " << elapsed_seconds.count() << "s\n";


  printCointainer(a) ;
  printCointainer(b) ;


}

/*
int main(int argc, char* argv[])
{
  (void)(argc) , (void)(argv);

  //auto dc = sbbdep::Dir("/").getContent();

  for(auto v : sbbdep::Dir("/").getContent())
    std::cout  << v << std::endl;

}
*/
