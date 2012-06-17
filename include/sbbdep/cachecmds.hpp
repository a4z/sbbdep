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

#ifndef SBBDEP_CACHECMDS_HPP_
#define SBBDEP_CACHECMDS_HPP_

#include <a4sqlt3/sqlparamcommand.hpp>
#include <a4sqlt3/parameters.hpp>


namespace sbbdep {


class CacheCmd : public a4sqlt3::SqlParamCommand
{

public:

  enum PreDefindSql{
    InsertPkg,
  };

  CacheCmd( const std::string& sql );
  virtual ~CacheCmd();

  virtual void Compile() { a4sqlt3::SqlParamCommand::Compile() ;}

protected:
  const std::string m_emptystr ;
};


// CacheSQL::InsertPkgSQL()
class InsertPkg : public CacheCmd
{
public:
  InsertPkg();
  ~InsertPkg();
  void Compile();

};

// CacheSQL::InsertDynLinkedSQL()
class InsertDynLinked : public CacheCmd
{
public:
  InsertDynLinked();
  ~InsertDynLinked();
  void Compile();

};

// CacheSQL::InsertRequiredSQL()
class InsertRequired : public CacheCmd
{
public:
  InsertRequired();
  ~InsertRequired();
  void Compile();

};

// CacheSQL::InsertRRunPathSQL()
class InsertRRunPath : public CacheCmd
{
public:
  InsertRRunPath();
  ~InsertRRunPath();
  void Compile();

};

// CacheSQL::InsertLdDirSQL()
class InsertLdDir : public CacheCmd
{
public:
  InsertLdDir();
  ~InsertLdDir();
  void Compile();

};

// CacheSQL::InsertLdLnkDirSQL()
class InsertLdLnkDir : public CacheCmd
{
public:
  InsertLdLnkDir();
  ~InsertLdLnkDir();
  void Compile();

};


class DeletePkgByFullName : public CacheCmd
{
public:
  DeletePkgByFullName() ;
  ~DeletePkgByFullName() ;
  void Compile();
  void setFullName(const std::string& val);

};





} /* namespace sbbdep */
#endif
