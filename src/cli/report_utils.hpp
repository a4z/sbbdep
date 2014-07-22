/*
--------------Copyright (c) 2010-2014 H a r a l d  A c h i t z---------------
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


#ifndef SBBDEP_CLI_REPORT_UTILS_HPP_
#define SBBDEP_CLI_REPORT_UTILS_HPP_


#include <a4sqlt3/dataset.hpp>

#include <string>
#include <vector>
#include <set>
#include <map>


namespace sbbdep{
namespace cli{
namespace utils{ // ??


using StringVec = std::vector<std::string>;
using StringSet = std::set<std::string>;
using StringMap = std::map<std::string, std::string>;


class ReportSet : public a4sqlt3::Dataset
{
public:
  ReportSet(const std::vector<std::string>& fieldnames);

  void addFields(a4sqlt3::DbValueList fields);

} ;
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------

auto no_conversion = [](const std::string& val) -> std::string { return val;};

template<typename T>
std::string joinToString(T container,  const std::string join,
    std::function<std::string(const std::string&)> converter = no_conversion )
{

  std::string retval;

  if( container.empty() )
    return retval;

  auto pos = container.begin();
  retval+=converter(*pos);

  while( ++pos != container.end())
    {
      retval += join  + converter (*pos) ;
    }

  return retval;

}
//--------------------------------------------------------------------------------------------------

template<typename T>
StringSet getKeySet(const T& keyvalmap){
  StringSet retval;
  for(auto val: keyvalmap) retval.insert(val.first);
  return retval;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------



//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
struct ReportElement{

  using Node =  std::map< std::string, ReportElement >  ;

  Node node ;

  ReportElement() = default;

  ReportElement( std::string s, ReportElement e );

  void add(StringVec path);

};
//--------------------------------------------------------------------------------------------------


struct ReportTree
{
  ReportElement::Node node ;

  void add(StringVec path) ;

};
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

#ifdef DEBUG
void printTree(ReportTree& tree);
#endif


bool isRRunPath(const std::string& dirname);
bool isLinkPath(const std::string& dirname);




} // utils
}}


#endif
