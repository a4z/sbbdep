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

#include <sbbdep/cachecmds.hpp>

#include <sbbdep/cachesql.hpp>

namespace sbbdep {

using namespace a4sqlt3;


CacheCmd::CacheCmd( const std::string& sql )
: SqlParamCommand(sql)
,m_emptystr("")
{

}
//--------------------------------------------------------------------------------------------------

CacheCmd::~CacheCmd()
{

}
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------


InsertPkg::InsertPkg()
: CacheCmd(CacheSQL::InsertPkgSQL())
{
}
//--------------------------------------------------------------------------------------------------

InsertPkg::~InsertPkg()
{
}
//--------------------------------------------------------------------------------------------------

void InsertPkg::Compile()
{
  CacheCmd::Compile();

  Parameters()->Nr(1)->setType<ParameterStringRef>( m_emptystr ) ; // pkgname.FullName()
  Parameters()->Nr(2)->setType<ParameterStringRef>( m_emptystr ) ; // pkgname.Name()
  Parameters()->Nr(3)->setType<ParameterStringRef>( m_emptystr ) ; // pkgname.Version()
  Parameters()->Nr(4)->setType<ParameterStringRef>( m_emptystr ) ; // pkgname.Arch()
  Parameters()->Nr(5)->setType<ParameterInt>( 0 ) ; //pkgname.Build().Num()
  Parameters()->Nr(6)->setType<ParameterStringRef>( m_emptystr ) ; // pkgname.Build().Tag()
  Parameters()->Nr(7)->setType<ParameterInt64>( 0 ) ; // timestamp
}
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------


InsertDynLinked::InsertDynLinked()
: CacheCmd(CacheSQL::InsertDynLinkedSQL())
{
}
//--------------------------------------------------------------------------------------------------

InsertDynLinked::~InsertDynLinked()
{
}
//--------------------------------------------------------------------------------------------------

void InsertDynLinked::Compile()
{
  CacheCmd::Compile();

  Parameters()->Nr(1)->setType<a4sqlt3::ParameterInt64>( 0 ); // pkgid
  Parameters()->Nr(2)->setType<a4sqlt3::ParameterStringRef>( m_emptystr ); //dli.filename
  Parameters()->Nr(3)->setType<a4sqlt3::ParameterString>( m_emptystr ); // dli.filename.getDir()
  Parameters()->Nr(4)->setType<a4sqlt3::ParameterString>( m_emptystr ); // dli.filename.getBase()
  // 5 setNull (default), soname
  Parameters()->Nr(6)->setType<a4sqlt3::ParameterInt>( 0 ); // dli.arch

}
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

InsertRequired::InsertRequired()
: CacheCmd(CacheSQL::InsertRequiredSQL())
{
}
//--------------------------------------------------------------------------------------------------

InsertRequired::~InsertRequired()
{
}
//--------------------------------------------------------------------------------------------------

void InsertRequired::Compile()
{
  CacheCmd::Compile();

  Parameters()->Nr(1)->setType<a4sqlt3::ParameterInt64>(0); // file id
  Parameters()->Nr(2)->setType<a4sqlt3::ParameterStringRef>( m_emptystr ); // needed
}
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

InsertRRunPath::InsertRRunPath()
: CacheCmd(CacheSQL::InsertRRunPathSQL())
{
}
//--------------------------------------------------------------------------------------------------

InsertRRunPath::~InsertRRunPath()
{
}
//--------------------------------------------------------------------------------------------------

void InsertRRunPath::Compile()
{
  CacheCmd::Compile();

  Parameters()->Nr(1)->setType<a4sqlt3::ParameterInt64>(0); // file id
  Parameters()->Nr(2)->setType<a4sqlt3::ParameterStringRef>( m_emptystr ); // runpath
}
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

InsertLdDir::InsertLdDir()
: CacheCmd(CacheSQL::InsertLdDirSQL())
{
}
//--------------------------------------------------------------------------------------------------

InsertLdDir::~InsertLdDir()
{
}
//--------------------------------------------------------------------------------------------------

void InsertLdDir::Compile()
{
  CacheCmd::Compile();
  Parameters()->Nr(1)->setType<a4sqlt3::ParameterStringRef>(m_emptystr);

}
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

InsertLdLnkDir::InsertLdLnkDir()
: CacheCmd(CacheSQL::InsertLdLnkDirSQL())
{
}
//--------------------------------------------------------------------------------------------------

InsertLdLnkDir::~InsertLdLnkDir()
{
}
//--------------------------------------------------------------------------------------------------

void
InsertLdLnkDir::Compile()
{
  CacheCmd::Compile();
  Parameters()->Nr(1)->setType<a4sqlt3::ParameterStringRef>(m_emptystr);
}
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------


DeletePkgByFullName::DeletePkgByFullName()
:  CacheCmd(CacheSQL::DeletePkgByFullnameSQL())
{
}
//--------------------------------------------------------------------------------------------------

DeletePkgByFullName::~DeletePkgByFullName()
{
}
//--------------------------------------------------------------------------------------------------

void
DeletePkgByFullName::Compile()
{
  CacheCmd::Compile();
  Parameters()->Nr(1)->setType<a4sqlt3::ParameterStringRef>(m_emptystr);
}
//--------------------------------------------------------------------------------------------------

void
DeletePkgByFullName::setFullName(const std::string& val)
{
  Parameters()->Nr(1)->setValue(val);
}
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------



} /* namespace sbbdep */
