#ifndef SBBDEP_CONCURRENTPEEK
#define SBBDEP_CONCURRENTPEEK

#include <mutex>
#include <vector>

namespace sbbdep
{

  /**
   * utility for picking out elements of a vector concurrently
   *
   *
   */
  template <typename T> struct ConcurrentPeek
  {
    ConcurrentPeek (std::vector<T> data, const T& eolElem = T ())
    : _data (std::move (data))
    , _pos (_data.begin ())
    , _eolElem (eolElem)
    {
    }

    T
    operator() ()
    {
      std::unique_lock<std::mutex> lock (_mtx);
      return _pos == _data.end () ? _eolElem : std::move (*_pos++);
    }

    T
    pop ()
    {
      std::unique_lock<std::mutex> lock (_mtx);
      return _pos == _data.end () ? _eolElem : std::move (*_pos++);
    }

  private:
    std::vector<T>                    _data;
    typename std::vector<T>::iterator _pos;
    T                                 _eolElem;
    std::mutex                        _mtx;
  };

} // ns

#endif // SBBDEP_CONCURRENTPEEK
