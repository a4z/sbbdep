/*
--------------Copyright (c) 2009-2013 H a r a l d  A c h i t z---------------
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

#include <sbbdep/log.hpp>
#include <boost/iostreams/stream.hpp>

#include <mutex>


namespace sbbdep
{

  
class LogWriter
{

public:
  

  LogWriter( log_stream_type& stm )
    : _stm( stm )
  {
  }

  std::streamsize
  flush( const char* c, std::streamsize n )
  {
    std::lock_guard<std::mutex> lock(_mtx);  
    typedef std::ostream_iterator<char,char> iter_type;
    std::copy( c, c + n, iter_type( _stm ) );
    return n;
  }//---------------------------------------------------------------------------------------------

private:
  log_stream_type& _stm;
  std::mutex _mtx;
};
  
//--------------------------------------------------------------------------------------------------  
  


class LogSink
{
  std::shared_ptr<class LogWriter> _writer ;
  
public:
  typedef char char_type;
  typedef boost::iostreams::sink_tag category;

  
  LogSink( std::shared_ptr<class LogWriter> writer )
    : _writer( writer )
  {
  }  
 
  std::streamsize
  write( const char_type* c, std::streamsize n )
  {
    return _writer->flush(c,n);
  }

};
//--------------------------------------------------------------------------------------------------

  
class LogStream : public boost::iostreams::stream<LogSink>
{
  LogSink _sink;
public:
  LogStream(std::shared_ptr<class LogWriter> writer): _sink(writer) {open(_sink);}
};  
//--------------------------------------------------------------------------------------------------



struct DevNullSink
{
  typedef char char_type;
  typedef boost::iostreams::sink_tag category;

  std::streamsize
  write( const char_type* c, std::streamsize n )
  {
    return n;
  }
};
//--------------------------------------------------------------------------------------------------


class  DevNullStream : public boost::iostreams::stream< DevNullSink >
{
  DevNullSink m_sink;
public:
  DevNullStream(): m_sink(){ open(m_sink); }
};
//--------------------------------------------------------------------------------------------------




std::unique_ptr<LogSetup::Setup> LogSetup::_setup{nullptr}; 

//--------------------------------------------------------------------------------------------------
void LogSetup::create(std::ostream& appstm, bool quiet)
{
  if( _setup != nullptr )
    throw "TODO"; 
  
  _setup.reset(new Setup{appstm, quiet});
}
//--------------------------------------------------------------------------------------------------
bool LogSetup::Quiet()
{
  return _setup->quiet ; 
}
//--------------------------------------------------------------------------------------------------
std::ostream& LogSetup::AppSream()
{
  return _setup->appstm ; 
}
//--------------------------------------------------------------------------------------------------  
LogChannel LogDebug()
{
#ifdef DEBUG
  static std::shared_ptr<LogWriter> writer = std::make_shared<LogWriter>(std::cerr) ;
  return LogChannel( std::make_shared<DevNullStream>() ) ;  
#else
  return LogChannel( std::make_shared<DevNullStream>() ) ;  
#endif

}
//--------------------------------------------------------------------------------------------------
LogChannel LogError()
{
  
  static std::shared_ptr<LogWriter> writer = std::make_shared<LogWriter>(std::cerr);
  
  return LogChannel(std::make_shared<LogStream>(writer)); 
}
//--------------------------------------------------------------------------------------------------
LogChannel LogInfo()
{
  
  if ( LogSetup::Quiet() )
    return LogChannel( std::make_shared<DevNullStream>() ) ;    
  
  
  static std::shared_ptr<LogWriter> writer = std::make_shared<LogWriter>(std::cout);
  return LogChannel(std::make_shared<LogStream>(writer));  
  
}
//--------------------------------------------------------------------------------------------------

LogChannel WriteAppMsg()
{
  static std::shared_ptr<LogWriter> writer = std::make_shared<LogWriter>(LogSetup::AppSream());
  return LogChannel(std::make_shared<LogStream>(writer));  
  
}
//--------------------------------------------------------------------------------------------------



}// ns



