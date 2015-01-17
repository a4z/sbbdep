/*
--------------Copyright (c) 2009-2015 H a r a l d  A c h i t z---------------
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

#ifndef SBBDEP_ERROR_HPP_
#define SBBDEP_ERROR_HPP_


#include <iosfwd>
#include <string>

namespace sbbdep{


  /// map error reason to enum value
  enum class ErrCode
  {
     GENERIC    = 1 ,
     ASSERT ,
     TODO       = 98 ,
     UNEXPECTED = 99 
  };
  


  class Error
  {

  public:
    Error( std::string info ) ;
    virtual ~Error() noexcept ;

    virtual ErrCode id() const = 0 ;

    const std::string& info() const ;

  protected:
    const std::string _info;

    friend std::ostream& operator<<(std::ostream& os, const Error& e);

    virtual void toStream(std::ostream& os) const;
  };

  std::ostream& operator<< (std::ostream& os, const Error& e);


  template<ErrCode ec>
  class ErrType: public Error
  {

  public:
    ErrType( std::string msg = "" )
      : Error( std::move(msg) )
    {
    }

    virtual ~ErrType() noexcept {}

    ErrCode id() const override  { return ec ; }

  };
  
  using ErrGeneric      =  ErrType<ErrCode::GENERIC>;
  using ErrAssert       =  ErrType<ErrCode::ASSERT>;
  using ErrToDo         =  ErrType<ErrCode::TODO>;
  using ErrUnexpected   =  ErrType<ErrCode::UNEXPECTED>;





} // ns 

#define SBBASSERT( exp ) if ( !( exp ) ) \
  throw  sbbdep::ErrAssert( std::string(" assertion in ")\
            + sbbdep::PathName ( __FILE__ ).base() + ":" \
            +  __PRETTY_FUNCTION__ + ": " + #exp )


#endif /* ...ERROR_HPP_ */
