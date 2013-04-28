/*
--------------Copyright (c) 2010-2012 H a r a l d  A c h i t z---------------
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


#ifndef SBBDEP_CACHESQL_HPP_
#define SBBDEP_CACHESQL_HPP_


#include <string>


struct sqlite3;

namespace sbbdep {

struct CacheSQL
{
  // TODO check what of these chould  be const char* / constexpr
  
  // cache stuff 
  static std::string CreateSchemaSQL();
  
  static std::string CreateVersion(int major, int minor, int patchlevel);
  
  static std::string CheckVersion(int major, int minor, int patchlevel);

  // create indexes...
  static std::string CreateIndexes();
  
  static std::string InsertPkgSQL();
  
  static std::string InsertDynLinkedSQL();
  
  static std::string InsertRequiredSQL();
  
  static std::string InsertRRunPathSQL();
  
  static std::string InsertLdDirSQL();
  
  static std::string InsertLdLnkDirSQL();
  
  static std::string DeletePkgByFullnameSQL();
  
  static std::string MaxPkgTimeStamp();
  

  constexpr const char* CreateViews();



  //depfinder
  static std::string SearchPgkOfSoNameSQL();
  

  static std::string SearchPgkOfFile(); // dir , name

  static std::string SearchRequiredByLib() ; // 1 soname, 2 arch

  // this is for rpath $ORIGIN replacement,
  static std::string replaceORIGIN(const std::string& originstr, const std::string& fromfile);
  static void register_own_sql_functions(sqlite3* db);
  


};

}

#endif /* CACHESQL_HPP_ */
