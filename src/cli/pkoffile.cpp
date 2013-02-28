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


#include "pkoffile.hpp"


#include <sbbdep/cache.hpp>
#include <sbbdep/cachesql.hpp>
#include <sbbdep/pathname.hpp>

#include <sbbdep/log.hpp>

#include <a4sqlt3/sqlcommand.hpp>
#include <a4sqlt3/parameters.hpp>
#include <a4sqlt3/dbvalue.hpp>

#include <a4sqlt3/rowhandler.hpp>
#include <a4sqlt3/columns.hpp>
#include <a4sqlt3/error.hpp>

#include <iostream>
//#include <algorithm>
//#include <iterator>

namespace sbbdep {
// pkfofsoname would be the better name ... ore create these commands as stored command on db...
class PkOfFile::Cmd : public a4sqlt3::SqlCommand
{
  
public:
  Cmd() :
    a4sqlt3::SqlCommand(CacheSQL::SearchPgkOfSoNameSQL())
  {
  }//-----------------------------------------------------------------------------------------------

  void
  setSerachVal( const std::string& soname, int arch  )
  {
      Parameters().Nr(1).set(soname);
      Parameters().Nr(2).set(arch);
            
  }//-----------------------------------------------------------------------------------------------

};
//--------------------------------------------------------------------------------------------------


class PkOfFile::CmdRequiredBy : public a4sqlt3::SqlCommand
{
  
public:
  CmdRequiredBy() :
    a4sqlt3::SqlCommand(CacheSQL::SearchRequiredByLib())
  {
  }//-----------------------------------------------------------------------------------------------

  void
  setSerachVal( const std::string& soname, int arch  )
  {
    Parameters().Nr(1).set(soname);
    Parameters().Nr(2).set(arch);
            
  }//-----------------------------------------------------------------------------------------------

};
//--------------------------------------------------------------------------------------------------



//--------------------------------------------------------------------------------------------------
struct PkOfFile::CmdRH : public a4sqlt3::RowHandler
{
  
  StringList ResultList;

  bool
  OnHandleRow( a4sqlt3::Columns& rowcols, sqlite3_stmt* stmt )
  {
   
    //Log::Debug() << "RH: " <<  rowcols.getString(0) << std::endl; 
    ResultList.push_back(rowcols[0].get<std::string>());
    
    return true;
  }//-----------------------------------------------------------------------------------------------  
  

  void
  Reset()
  {
    ResultList.clear();
    a4sqlt3::RowHandler::Reset();
  }
  
};
//--------------------------------------------------------------------------------------------------


PkOfFile::PkOfFile() :
  m_cmd(0), m_cmdreqiredby(0)
{
  
  Cmd* cm = new Cmd();
  try
    {
      Cache::get()->DB().CompileCommand(cm);
    }
  catch ( ... )
    {
      delete cm; cm = 0 ; 
      throw; 
    }
  m_cmd = cm; 
  
  CmdRequiredBy* cmdreqiredby = new CmdRequiredBy();
  try
    {
      Cache::get()->DB().CompileCommand(cmdreqiredby);
    }
  catch ( ... )
    {
      delete cmdreqiredby; cmdreqiredby = 0 ; 
      throw; 
    }  
  
  m_cmdreqiredby = cmdreqiredby;
  
}
//--------------------------------------------------------------------------------------------------


PkOfFile::~PkOfFile()
{
  if (m_cmd) delete m_cmd;
  if (m_cmdreqiredby) delete m_cmdreqiredby;
}
//--------------------------------------------------------------------------------------------------

void
PkOfFile::search( const std::string& soname, int arch , StringList& resultlist )
{
  
  if (!m_cmd) ;// TODO , throw something, partially done cause throws in c'tor, but review

  CmdRH rh;
  //Log::Debug() << m_cmd->SQL() << pn.getDir() << " - " << pn.getBase() << std::endl; 
      
  m_cmd->setSerachVal(soname , arch);
  
  m_cmd->Run(&rh);
//  std::cout << "pkgsearch: ";    
//  std::copy(rh.ResultList.begin(), rh.ResultList.end(), std::ostream_iterator<std::string>(std::cout, ", "));  
  resultlist.insert(resultlist.end(), rh.ResultList.begin(), rh.ResultList.end());
  
}

//--------------------------------------------------------------------------------------------------

void 
PkOfFile::searchRequiredBy( const std::string& soname, int arch , StringList& resultlist ) 
{
  
  struct RH : public a4sqlt3::RowHandler
  {
    StringList ResultList; //----------------------------------------

    bool OnHandleRow( a4sqlt3::Columns& rowcols, sqlite3_stmt* stmt )
    {
      ResultList.push_back(rowcols[0].get< std::string >());
      return true;
    }//---------------------------------------------------------------  
    void Reset()
    {
      ResultList.clear();
      a4sqlt3::RowHandler::Reset();
    }//---------------------------------------------------------------
  };

  RH rh;
  
  m_cmdreqiredby->setSerachVal(soname , arch);
  
  m_cmdreqiredby->Run(&rh);
  
  resultlist.insert(resultlist.end(), rh.ResultList.begin(), rh.ResultList.end());
  
}  
//--------------------------------------------------------------------------------------------------


}
