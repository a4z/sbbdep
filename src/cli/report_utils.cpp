/*
--------------Copyright (c) 2010-2013 H a r a l d  A c h i t z---------------
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


#include "report_utils.hpp"

#include <sbbdep/log.hpp>
#include <sbbdep/cache.hpp>

#include <a4sqlt3/sqlcommand.hpp>


namespace sbbdep {
namespace cli{
namespace utils{


//--------------------------------------------------------------------------------------------------
ReportSet::ReportSet(const std::vector<std::string> fieldnames)
{
  for(std::size_t i = 0; i < fieldnames.size(); ++i)
      _namemap.insert( NameMap::value_type(fieldnames[i], i) ) ;
}

void
ReportSet::addFields(a4sqlt3::DbValueList fields)
{
  if( fields.size() != _namemap.size() )
    throw "TODO"; //TODO

  _rows.emplace_back(fields);

  this->_rowcount = _rows.size();
}
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------






ReportElement::ReportElement( std::string s, ReportElement e ) : node {{s,e}}
{
}

void
ReportElement::add(StringVec path)
{
  if(not path.empty())
    node[*(path.begin())].add( StringVec(  ++(path.begin()), path.end() ) ) ;
}



void
ReportTree::add(StringVec path)
{
  if(not path.empty())
    node[*(path.begin())].add( StringVec(  ++(path.begin()), path.end() ) ) ;
}



#ifdef DEBUG
void printTree(ReportTree& tree)
{

  std::function<void(ReportElement, int)> printChild = [&printChild]( ReportElement elem , int level ){

    for(auto node: elem.node){
       for(int i = 0; i < level; ++i)
            LogInfo() << " " ;

      LogDebug() << node.first << "\n";
      printChild(node.second, level+2);
    }

  } ;

  for( auto elem : tree.node )
  {
      LogDebug() << elem.first << std::endl;
    printChild(elem.second, 2) ;
  }
}
#endif // DEBUG
//--------------------------------------------------------------------------------------------------


bool isRRunPath(const std::string& dirname)
{
  using namespace a4sqlt3;
  SqlCommand* cmd = Cache::getInstance()->DB().getCommand("isRRunpathDirectory");
  if( cmd == nullptr )
    {
      std::string sql = "SELECT count(*)  FROM rrunpath WHERE rrunpath.lddir NOT IN "
          " (SELECT dirname FROM lddirs UNION SELECT dirname FROM ldlnkdirs) "
          " AND rrunpath.lddir = ? ;" ;

      cmd = Cache::getInstance()->DB().createStoredCommand(
          "isRRunpathDirectory" ,  sql );
    }

  cmd->Parameters().Nr(1).set(dirname);

  Dataset ds;
  Cache::getInstance()->DB().Execute(cmd, ds);
  return ds.getField(0).getInt64() > 0 ;

}
//--------------------------------------------------------------------------------------------------

bool isLinkPath(const std::string& dirname)
{
  using namespace a4sqlt3;
  SqlCommand* cmd = Cache::getInstance()->DB().getCommand("isLinkPathDirectory");
  if( cmd == nullptr )
    {
      std::string sql =" SELECT count(*) FROM "
         " (SELECT dirname FROM lddirs WHERE dirname = ? "
            " UNION SELECT dirname FROM ldlnkdirs WHERE dirname = ?) ";

      cmd = Cache::getInstance()->DB().createStoredCommand(
          "isLinkPathDirectory" ,  sql );
    }

  cmd->Parameters().setValues( {DbValue(dirname), DbValue(dirname)} ) ;

  Dataset ds;
  Cache::getInstance()->DB().Execute(cmd, ds);
  return ds.getField(0).getInt64() > 0 ;

}
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
}
}} // ns
