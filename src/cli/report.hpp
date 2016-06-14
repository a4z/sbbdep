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


#ifndef SBBDEP_CLI_REPORT_HPP_
#define SBBDEP_CLI_REPORT_HPP_

#include <sl3/dataset.hpp>

#include<functional>
#include<map>
#include<set>
#include<string>
#include<vector>

namespace sbbdep{

  class Cache;
  class Path ;
  class PathName ;
  class Pkg;
  struct SyncData;

namespace cli{

  using StringVec = std::vector<std::string> ;

  void
  printSyncReport(Cache& cache,
                  const SyncData& syncdata);

  void
  printRequired(Cache& cache,
                const Pkg& pkg,
                const StringVec& ignores,
                bool shortNames,
                bool xdl ,
                bool ldd) ;


  void
  printWhoNeed(Cache& cache,
               const Pkg& pkg,
               bool shortNames,
               bool xdl ) ;


  void
  bdTree (Cache& cache, const Pkg& pkg, bool shortNames) ;



  bool
  fileInPackages (const sbbdep::Path& filepath);

  void
  lookupInPackages (const std::string& serach);



  namespace utils
  {
    using StringVec = std::vector<std::string> ;
    using StringSet = std::set<std::string> ;


    //--------------------------------------------------------------------------

    using convert_function = std::function<std::string(const std::string&)> ;


    template<typename T>
    std::string joinToString(const T& container,
                             const std::string join = ", ",
                             convert_function convert = // do nothing default
                                 [](const std::string& val) -> std::string
                                 { return val;} )
    {

      std::string retval;

      if (container.empty ())
        return retval;

      auto pos = container.begin ();
      retval+=convert(*pos);

      while (++pos != container.end ())
        {
          retval += join  + convert (*pos) ;
        }

      return retval;

    }
    //--------------------------------------------------------------------------

    template<typename T>
    StringSet getKeySet(const T& keyvalmap){
      StringSet retval;
      for(auto& val: keyvalmap)
        retval.insert(val.first);
      return retval;
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    struct ReportElement{

      using Node =  std::map< std::string, ReportElement >  ;

      Node node ;

      ReportElement() = default;

      ReportElement( std::string s, ReportElement e );

      void add(const StringVec& path);
    };
    //--------------------------------------------------------------------------

    struct ReportTree
    {
      ReportElement::Node node ;

      void add(const StringVec& path) ;
    };
    //--------------------------------------------------------------------------

    sl3::Dataset
    getPkgsOfFile (Cache& cache,const PathName& fname, int arch);

    //--------------------------------------------------------------------------

#ifdef DEBUG
    void printTree(const ReportTree& tree) ;
#endif // DEBUG


  } // ns utils



}
}


#endif
