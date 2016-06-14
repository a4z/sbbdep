/*
--------------Copyright (c) 2010-2016 H a r a l d  A c h i t z---------------
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

#include "sbbdep/cache.hpp"
#include "sbbdep/pkg.hpp"
#include "sbbdep/elffile.hpp"

#include "sbbdep/log.hpp"

#include "sl3/dataset.hpp"


namespace sbbdep {
namespace cli{


//------------------------------------------------------------------------------
constexpr const char*
getWhoNeedFileQuery()
{
  return
  R"~(
  select pkgs.fullname as pkg, 
         dynlinked.filename as filename , 
         required.needed as soname , 
         d2.filename as fromfile
   from  pkgs 
   inner join dynlinked on pkgs.id = dynlinked.pkg_id
   inner join required on dynlinked.id = required.dynlinked_id
  left join rrunpath on dynlinked.id = rrunpath.dynlinked_id 
  left join dynlinked d2 on required.needed = d2.soname 
   where  d2.arch = dynlinked.arch 
  AND 
  ( 
    ( rrunpath.lddir IS NOT NULL AND d2.dirname NOT IN 
    (  SELECT DISTINCT * from lddirs UNION SELECT distinct * from ldlnkdirs )
    AND rrunpath.lddir = d2.dirname )
   OR 
    (
      d2.dirname IN 
       ( select distinct * from lddirs union select distinct * from ldlnkdirs ) 
      AND 
       ( 
        rrunpath.lddir IS  NULL OR rrunpath.lddir IN 
        ( select distinct * from lddirs union select distinct * from ldlnkdirs ) 
       )
    )
  )
  AND d2.filename = ? 
  ;
  )~";
}
//------------------------------------------------------------------------------

constexpr const char*
getWhoNeedPkgQuery()
{

  return R"~(
select pkgs.fullname as pkg, 
       dynlinked.filename as filename , 
       required.needed as soname , 
       d2.filename as fromfile
 from  pkgs 
 inner join dynlinked on pkgs.id = dynlinked.pkg_id
 inner join required on dynlinked.id = required.dynlinked_id
left join rrunpath on dynlinked.id = rrunpath.dynlinked_id 
left join dynlinked d2 on required.needed = d2.soname 
inner join pkgs p2 on p2.id = d2.pkg_id
 where  d2.arch = dynlinked.arch 
AND 
( 
  ( 
    rrunpath.lddir IS NOT NULL AND 
     d2.dirname NOT IN
      ( select distinct * from lddirs union select distinct * from ldlnkdirs )  
     AND rrunpath.lddir = d2.dirname 
  )
  OR 
  (
    d2.dirname in 
      (select distinct * from lddirs union select distinct * from ldlnkdirs ) 
    AND 
    (
      rrunpath.lddir IS  NULL OR 
      rrunpath.lddir IN 
      (select distinct * from lddirs union select distinct * from ldlnkdirs ) 
    )
  )
)
AND p2.fullname  = ? 
;
)~";
}
//------------------------------------------------------------------------------


void
printWhoNeed (Cache& cache, const Pkg& pkg, bool shortNames, bool xdl)
{
  // works for single files and installed package

  using namespace sl3;

  Dataset ds; //{ "pkg", "filename" , "soname" , "fromfile"  }

  if (pkg.getType () == PkgType::BinLib)
    {
      auto& cmd = cache.namedCommand ("WhoNeedFileQuery",
                                      getWhoNeedFileQuery ());
      const DbValues args = { { pkg.getElfFiles () [0].getName () } };
      ds = cmd.select (args);
    }
  else if (pkg.getType () == PkgType::Installed)
    {
      auto& cmd = cache.namedCommand ("WhoNeedPkgQuery", getWhoNeedPkgQuery ());
      const DbValues args = { { pkg.getPath ().base () } };
      ds = cmd.select (args);
    }
  else
    {
      LogInfo () << "whoneeds option for this type of query not supported";
      return;
    }

  if (xdl)
    {
      utils::ReportTree reptree;
      for (const auto& row : ds)
        {
          reptree.add (
              { row.at (3).getText () + " (" + row.at (2).getText () + ")",
                  row.at (0).getText (), row.at (1).getText () });
        }

      std::function<void
      (const utils::ReportElement&, size_t)> printChild =
          [&printChild](const utils::ReportElement& elem , size_t level )
            {
              for(const auto& node: elem.node)
                {
                    { // scope log channel for new line
                      LogMsg ()
                      << std::string(level, ' ')
                      << node.first;
                    }
                  printChild(node.second, level+2);
                }
            };

      for (const auto& elem : reptree.node)
        {
            { // scope log channel for new line
              LogMsg () << elem.first << " is used from:";
            }
          printChild (elem.second, 2);
        }

    }
  else
    {
      utils::StringSet pkgnames;
      for (auto& row : ds)
        {
          pkgnames.insert (
              shortNames ?
                  PkgName (row.at (0).getText ()).name () :
                  row.at (0).getText ());
        }

      LogMsg ()
          << utils::joinToString (pkgnames, shortNames ? ", " : "\n")
          << std::endl;
    }



}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
}} // ns
