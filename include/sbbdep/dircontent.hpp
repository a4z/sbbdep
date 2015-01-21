/*
--------------Copyright (c) 2010-2015 H a r a l d  A c h i t z---------------
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


#ifndef SBBDEP_DIRCONTENT_HPP_
#define SBBDEP_DIRCONTENT_HPP_

#include <vector>
#include <string>
#include <functional>

namespace sbbdep {


/**
 * \brief Helper to get or iterate over entries in a directory
 *
 */
class Dir
{
public:

  ///Callback functor, taking the directory and the file name.
  using ContentCall  = std::function< bool(const std::string&,
                                             const std::string&)> ;

  /// filter function.
  /// returns true it the given name shall be ignored, false otherwise
  using IgnorFilter = std::function<bool(const std::string&)> ;

  /**
   * \brief c'tor.
   *
   */
  Dir(std::string name);

  /// property access
  const std::string&
  getName() const ;


  /**
   * \brief A default filter function
   *
   * ignores
   *    - hidden files/dirs .*  \n
   *    - backup files *~ \n
   *    - emacs backup files #*# \n
   *
   * \sa getContent \sa apply
   */
  static bool
  defaultFilter(const std::string& f) ;


  /**
   * \brief get a list of all entries in a directory.
   *
   * \throw ErrGeneric if the directory can not be opened
   *
   * \return list of names within the direcotry
   */
  std::vector<std::string>
  getContent(IgnorFilter = defaultFilter ) const ;

  /**
   * \brief apply a function to all entries.
   *
   * \throw ErrGeneric if the directory can not be opened
   * \throw whatever ContentCall might throw
   *
   * \param cb, if of the callback is false, function exits
   */
  void
  forEach(ContentCall cb, IgnorFilter = defaultFilter) const ;


private:
  
   std::string _name;

   
};

static const Dir PkgAdmDir {"/var/adm/packages"} ; // TODO

}

#endif /* SBBDEP_DIRCONTENT_HPP_ */
