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



#ifndef SBBDEP_ERROR_HPP_
#define SBBDEP_ERROR_HPP_

#include <a4z/err.hpp>


namespace sbbdep{

  A4Z_STATICNAME(ErrDomName , "sbbdep") ;
  /// defines the root domain 
  typedef a4z::ErrDomainRoot<ErrDomName> ErrorDomain;
  
  struct ErrorCodes 
  {
    enum Value{
      Generic = 1,
      StatErr = 2, 
      MagicErr = 3,  
      DepCheckErr = 4 ,
      PkgErr = 5 ,
      DirContentErr = 6
    };  
     
  };

  ///defines the err name
  A4Z_STATICNAME(ErrName , "Error") ; 
  ///defines err type
  typedef a4z::ErrType< ErrName, ErrorDomain , ErrorCodes::Value  > Error;


  typedef a4z::ErrObject< Error , ErrorCodes::Generic > ErrGeneric ;
  typedef a4z::ErrObject< Error , ErrorCodes::StatErr > ErrStat ;
  typedef a4z::ErrObject< Error , ErrorCodes::MagicErr > ErrMagic ;
  typedef a4z::ErrObject< Error , ErrorCodes::DepCheckErr > ErrDepCheck ;
  typedef a4z::ErrObject< Error , ErrorCodes::PkgErr > ErrPkg ;
  typedef a4z::ErrObject< Error , ErrorCodes::DirContentErr > ErrDirContent ; 
  
} // ns

#endif /* ERROR_HPP_ */
