/*
 --------------Copyright (c) 2012-2012 H a r a l d  A c h i t z---------------
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

#ifndef PKGADMDIR_HPP_
#define PKGADMDIR_HPP_

#include <functional>
#include <string>
#include <sbbdep/dircontent.hpp>

namespace sbbdep {



/**
 * \brief Object for iterating over files in a directory.
 *
 * Runs a functor for each file in the directory.
 * Iteration is not recursive.
 */
class PkgAdmDir
{

public:

   ///Callback functor, taking the dir path and the file name.
  typedef std::function<bool(const std::string&, const std::string&&)> DirContentCall;

  /// default filter action, returns true if entry shall be ignored
  typedef std::function<bool(const std::string&)> IgnorFilterCall;


  /**
   * \brief c'tor.
   * \param path default set to /var/adm/packages
   * \param filter default set to DefaultIgnorFilter
   */
  PkgAdmDir(std::string path = "/var/adm/packages", IgnorFilterCall filter = DefaultIgnorFilter);

  /// d'tor
  ~PkgAdmDir();


  const std::string getDirName(){ return m_dir.getDirName() ;}

  /**
   * \brief Runs the submitted function
   * Runs the submitted function and calls it for each file entry if
   * the filter function does not return true
   */
  void apply(DirContentCall call) ;

  ///default filter implementation.
  static bool DefaultIgnorFilter(const std::string& f){

    if(f[0] == '.' ) return true; // hidden and dirs, hidden should not exist in var/adm/package
    if(*(f.rbegin()) == '~') return true; // backuo files,
    if(f[0] == '#' && *(f.rbegin()) == '#') return true; // emacs backup files

    return false;
  }

private:
  DirContent m_dir;
  IgnorFilterCall m_ignoreFilter;
};

} /* namespace sbbdep */
#endif /* PKGADMDIR_HPP_ */
