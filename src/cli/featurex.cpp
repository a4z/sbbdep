/*
 --------------Copyright (c) 2013-2013 H a r a l d  A c h i t z---------------
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

#include "featurex.hpp"

#include <sbbdep/log.hpp>

#include <a4sqlt3/sqlcommand.hpp>
#include <a4sqlt3/dataset.hpp>
#include <a4sqlt3/dbvalue.hpp>

#include <iostream>
#include <map>
#include <memory>
#include <algorithm>
#include <functional>


std::ostream&
operator<<(std::ostream& stm, const a4sqlt3::DbValue& v)
{

  using namespace a4sqlt3 ;
  switch( v.getStorageType() )
    {

    case DbValueType::Null:
      stm << "<NULL>" ;
      break ;

    case DbValueType::Int64:
      stm << v.getInt64() ;
      break ;

    case DbValueType::Real:
      stm << v.getDouble() ;
      break ;

    case DbValueType::Text:
      stm << v.getString() ;
      break ;

    case DbValueType::Blob:
      stm << "<BLOB>" ;
      break ;
    default:
      stm << "unknown storage type !!" ;
      break ;
    }

  return stm ;
}

namespace sbbdep
{
namespace cli
{

namespace
{

}

using namespace a4sqlt3 ;

struct HRowSet
{
  std::map<std::string, int> names ;

  struct Row
  {
    Row()
        : childs(new HRowSet())
    {
    }
    Row(DbValueList flds)
        : childs(new HRowSet()),
          fields
            {std::move(flds)}
    {
    }

    std::unique_ptr<HRowSet> childs ;

    DbValueList fields ;

  } ;

  std::vector<Row> rows ;

  int
  field(const std::string name)
  {
    auto fi = names.find(name) ;
    if( fi == std::end(names) )
      throw "TODO" ;

    return fi->second ;
  }
} ;

void
printHRow(const HRowSet& rs, int level = 0)
{

  std::string indent("..", level) ;
  for( const auto& row : rs.rows )
    {
      std::cout << indent ;
      for( const auto& f : row.fields )
        {
          std::cout << f << " / " ;
        }
      std::cout << std::endl ;

      printHRow(*row.childs, level + 1) ;

//      for(const auto& r : row.childs->rows)
//        {
//          printHRow(r, level+1);
//        }

    }

}

//-------------------------------------------------------------------------------------------------
struct HRow
{
  HRow()
  {
  }
  HRow(DbValueList flds)
      : fields{std::move(flds)}
  {
  }

  DbValueList fields ;

  std::vector<HRow> childs ;

} ;

void
printHRow(const HRow& hr, int level = 0)
{
  std::string indent("..", level) ;

  std::cout << indent ;
  for( const auto& f : hr.fields )
    {
      std::cout << f << " / " ;
    }
  std::cout << std::endl ;

  for( const auto& r : hr.childs )
    {
      printHRow(r, level + 1) ;
    }

}
//-------------------------------------------------------------------------------------------------

void
runFx()
{
  
  LogInfo() << " internal test function " << __PRETTY_FUNCTION__ << "\n" ;

#ifndef DEBUG
  LogInfo() << "not enabled in non debug builds\n" ;
  return ;
#endif
/*
    {
      HRowSet rs ;

      rs.rows.push_back(HRowSet::Row
        {
          {DbValue(1), DbValue("eins")}}) ;

      HRowSet& childs = *rs.rows.rbegin()->childs ;
      childs.rows.push_back(
          HRowSet::Row{{DbValue("eins"), DbValue("eins")}}
      ) ;
      childs.rows.push_back(
          HRowSet::Row{{DbValue("eins"), DbValue("zwei")}}
      ) ;

      printHRow(rs) ;
    }

    {
      HRow r ;

      r.fields =
        { DbValue(1), DbValue("eins")} ;

      r.childs.push_back(HRow(
        {DbValue("eins"), DbValue("eins")})) ;
      r.childs.push_back(HRow(
        {DbValue("eins"), DbValue("zwei")})) ;

      printHRow(r) ;

    }
*/

    const std::string cmd = "/sbin/ldconfig -p";
    FILE* popin = popen(cmd.c_str(), "r");
    if( popin )
      {
        char buff[512];
        while (std::fgets(buff, sizeof( buff ), popin) != NULL)
          {
            const char* buff_start = buff;
            const char* buff_end = buff + sizeof( buff ) ;

            const char* soname_begin = std::find_if( buff_start,  buff_end,
                [](const char c)->bool{ return c != '\t' ; }
                );

            const char* soname_end = std::find_if( soname_begin,  buff_end,
                [](const char c)->bool{ return c == ' '; }
            );

            // TODO if anything is end, exit with a message of problem line

            const std::string arrow = "=> ";

            const char* file_begin = std::search( soname_end, buff_end,
                std::begin(arrow), std::end(arrow)) + arrow.size();

            const std::string lineend = "\n";
            const char* file_end = std::search( file_begin, buff_end,
                std::begin(lineend), std::end(lineend)) ;

            std::cout << "here: '" << std::string(soname_begin, soname_end) << "'" ;
            if(file_end != buff_end)
              std::cout << " to '" << std::string(file_begin, file_end) << "'" ;

            std::cout << std::endl ;

          }


        pclose(popin);
      }

    // TODO for the tree option check http://linux.about.com/library/cmd/blcmdl8_ld.so.htm
    // and review elffile

}

}
} // ns
