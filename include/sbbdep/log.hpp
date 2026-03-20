/*
--------------Copyright (c) 2009-2018 H a r a l d  A c h i t z---------------
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

#ifndef SBBDEB_LOGSYSTEM_HPP_
#define SBBDEB_LOGSYSTEM_HPP_

#include <memory>
#include <ostream>

namespace sbbdep
{

  typedef std::basic_ostream<char, typename std::char_traits<char>>
      log_stream_type;

  // the whole thing is a kind of hack, but for now it has to replace the
  // former a4z::LogSystem

  class LogChannel
  {
    LogChannel (std::shared_ptr<log_stream_type> stm)
    : _stm (stm)
    {
    }

    friend LogChannel LogDebug ();
    friend LogChannel LogError ();
    friend LogChannel LogInfo ();
    friend LogChannel LogMsg ();

  public:
    ~LogChannel () { *_stm << '\n'; }

    template <typename T>
    LogChannel&
    operator<< (const T& val)
    {
      *_stm << val;
      return *this;
    } //---------------------------------------------------------------------------------------------

    LogChannel&
    operator<< (log_stream_type& (*pf) (log_stream_type&))
    {
      *_stm << pf;
      return *this;
    } //---------------------------------------------------------------------------------------------

    LogChannel&
    operator<< (std::ios& (*pf) (std::ios&))
    {
      *_stm << pf;
      return *this;
    } //---------------------------------------------------------------------------------------------

    LogChannel&
    operator<< (std::ios_base& (*pf) (std::ios_base&))
    {
      *_stm << pf;
      return *this;
    } //---------------------------------------------------------------------------------------------

  private:
    std::shared_ptr<log_stream_type> _stm;
  };
  //--------------------------------------------------------------------------------------------------

  class LogSetup
  {
    struct Setup
    {
      std::ostream& appstm;
      bool          quiet;
    };
    static std::unique_ptr<Setup> _setup;

  public:
    static void create (std::ostream& appstm, bool quiet);

    static bool          Quiet ();
    static std::ostream& AppStream ();
  };

  LogChannel LogDebug ();
  LogChannel LogError ();
  LogChannel LogInfo ();
  LogChannel LogMsg ();

} // ns

#endif
