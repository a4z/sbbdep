/*
--------------Copyright (c) 2010-2013 H a r a l d  A c h i t z---------------
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


#ifndef SBBDEP_LOG_HPP_
#define SBBDEP_LOG_HPP_

#include <a4z/logsystemomp.hpp>
#include <a4z/single.hpp>
#include <a4z/errostream.hpp>

#include <boost/iostreams/stream.hpp>

namespace sbbdep {






class Log : public a4z::LogSystemOMP<char> , public a4z::Single<sbbdep::Log>
{
    friend class a4z::Single<Log>;
    

    
public:
    //typedef a4z::LogSystemOMP< char >::ChannelType ChannelType;
    
    struct ChannelId{  enum ChannelNames    {
      Debug = 1 , Info = 2, Error = 3, AppMessage = 4
    } ;   };

    
    struct Level {  enum LogLevelValues   {
      NDebug = 0, Debug = 1
    }; };    
    
    
  Log() :
#ifdef DEBUG      
      a4z::LogSystemOMP<char>(Level::Debug) 
#else
      a4z::LogSystemOMP<char>(Level::NDebug) 
#endif
  {
    // Error/Debug = cerr , Info = cout

  }
  ~Log();
  

  static ChannelType Debug(int loglevel = Level::Debug){ 
    return sbbdep::Log::get()->Channel( ChannelId::Debug , loglevel ) ; 
  }  


  static Log::ChannelType Info(int loglevel = Level::NDebug){ 
    return sbbdep::Log::get()->Channel( ChannelId::Info , loglevel ); 
  }  
  

  static Log::ChannelType Error(int loglevel = Level::NDebug){ 
    return sbbdep::Log::get()->Channel( ChannelId::Error , loglevel); 
  }  

  static Log::ChannelType AppMessage(int loglevel = Level::NDebug){
    try
      { // needs to be setup by the application
        return sbbdep::Log::get()->Channel(ChannelId::AppMessage, loglevel);
      }
    catch (a4z::LogError& e)
      {
        Error()<< e << "\n";
        throw;
      }
  }

  
};


//lets see which syntax I will prefer...
inline
Log::ChannelType LogDebug(){ 
  return Log::get()->Debug(); 
}  

inline
Log::ChannelType LogInfo(){ 
  return Log::get()->Info(); 
}  

inline
Log::ChannelType LogError(){ 
  return Log::get()->Error(); 
}  

inline
Log::ChannelType WriteAppMsg(){
  return Log::get()->AppMessage();
}



typedef Log::ChannelType LogChannelType; 


// this should go to log base class
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
static  DevNullStream DevNull  ;



}

#endif /* SBBDEP_LOG_HPP_ */
