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


#include "report.hpp"

#include "sbbdep/log.hpp"



namespace sbbdep {
namespace cli{
namespace utils{

ReportElement::ReportElement( std::string s, ReportElement e )
: node {{std::move(s),std::move(e)}}
{
}
//------------------------------------------------------------------------------

void
ReportElement::add(const StringVec& path)
{
  if(not path.empty())
    node[*(path.begin())].add( StringVec(  ++(path.begin()), path.end() ) ) ;
}
//------------------------------------------------------------------------------


void
ReportTree::add(const StringVec& path)
{
  if(not path.empty())
    node[*(path.begin())].add( StringVec(  ++(path.begin()), path.end() ) ) ;
}
//------------------------------------------------------------------------------

#ifdef DEBUG
void printTree(const ReportTree& tree)
{

  std::function<void(ReportElement, int)> printChild =
      [&printChild]( ReportElement elem , int level )
        {
          for(auto node: elem.node){
             for(int i = 0; i < level; ++i)
                  LogInfo() << " " ;

            LogDebug() << node.first << "\n";
            printChild(node.second, level+2);
          }

        };

  for( auto elem : tree.node )
  {
      LogDebug() << elem.first ;
    printChild(elem.second, 2) ;
  }
}
#endif // DEBUG


//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
}
}} // ns
