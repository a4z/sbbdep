/*
 --------------Copyright (c) 2013-2014 H a r a l d  A c h i t z---------------
 -----------< h a r a l d dot a c h i t z at g m a i l dot c o m >------------
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 -----------------------------------------------------------------------------
 */

#include <sbbdep/lddmap.hpp>

#include <sstream>
#include <cstdio>

namespace sbbdep {

namespace {
using StringMap = std::map<std::string, std::string>;
}

StringMap
getLddMap(const sbbdep::PathName& f)
{
  StringMap retval;

  const std::string cmd = "ldd " + f.Str();
  FILE* popin = popen(cmd.c_str(), "r");
  std::stringstream output;
  if( popin )
    {
      char buff[512];
      while (std::fgets(buff, sizeof( buff ), popin) != NULL)
        output << buff;

      pclose(popin);
    }

  std::string line;
  const std::string splitter = " => ";
  while (std::getline(output, line))
    {
      using size_t = std::size_t;
      size_t npos = std::string::npos;

      size_t begin = line.find_first_not_of(" \t");
      if( begin == npos )
        continue;

      size_t splitt = line.find(splitter, begin);
      if( splitt == npos )
        continue;

      size_t end = line.find(" ", splitt + splitter.size());
      if( begin == npos )
        continue;

      //std::cout << line.substr( begin, splitt - begin ) ;
      //std::cout << line.substr( splitt+splitter.size(), end - splitt -splitter.size() ) ;

      retval.insert(
          StringMap::value_type(line.substr(begin, splitt - begin),
              line.substr(splitt + splitter.size(), end - splitt - splitter.size())));
    }

  return retval;
}

}
