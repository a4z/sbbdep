#ifndef SBBDEP_BACKGROUNDJOB
#define SBBDEP_BACKGROUNDJOB

#include <functional>
#include <memory> 
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>


namespace sbbdep
{

template<typename T>
class BackgroundJob
{


public:

  using Task = std::function<void(const T&)> ;
  
  BackgroundJob(Task);
  ~BackgroundJob();

  BackgroundJob(const BackgroundJob&) = delete;
  BackgroundJob& operator=(const BackgroundJob&)= delete ;
  BackgroundJob(const BackgroundJob&&) = delete;
  BackgroundJob& operator=(const BackgroundJob&&)= delete ;


  void stop();

  void setCleanup(bool);

  bool cleanup();

  bool isRunning();

  bool push(T&&);

  bool push(const T&);


private:

  using Messages = std::vector<T> ;
  using LockGuard = std::unique_lock<std::mutex> ;


  //called only intern,
  void consume(const Messages&);

  Task _task;

  struct j
  {
    std::vector<T> messages;
    std::mutex mtx;
    std::condition_variable condt;
    std::atomic_bool running; // should e {false} but problem ongcc 4.8.2
    bool newData;

  }; j _queue;

  bool _cleanUp {true};

  std::thread _workerThread ;


};
//------------------------------------------------------------------------------


template<typename T>
BackgroundJob<T>::BackgroundJob(Task task)
  : _task(task), _queue()
{
  auto worker = [this]()
    {
      auto& queue = this->_queue;
      queue.running = true;

      Messages messages;
      while (queue.running)
        {
          LockGuard lock(queue.mtx);
          while (not queue.newData)
            {
              queue.condt.wait( lock );
            }

          using std::swap;
          swap(messages, queue.messages);
          lock.unlock();
          this->consume(messages);
          messages.clear();
        }
    };

  std::thread t (worker);
  _workerThread.swap (t);
    
}
//------------------------------------------------------------------------------


template<typename T>
BackgroundJob<T>::~BackgroundJob()
{
  stop();
}
//------------------------------------------------------------------------------


template<typename T>
void
BackgroundJob<T>::stop()
{
  if(_queue.running)
    {
      _queue.running = false ;
      _queue.condt.notify_all();
      if(_workerThread.joinable())
        {
          _workerThread.join();
        }

    }


  if(cleanup())
    {
      LockGuard lock(_queue.mtx);
      consume(_queue.messages);
      lock.unlock();
    }

}
//------------------------------------------------------------------------------

// simply process all messages, rest must have been done by others
template<typename T>
void
BackgroundJob<T>::consume(const Messages& messages)
{
  for(auto&& m : messages)
    {
      _task(m);
    }

}
    //------------------------------------------------------------------------------

template<typename T>
bool
BackgroundJob<T>::isRunning()
{
   return  _queue.running  ;
}
//------------------------------------------------------------------------------

template<typename T>
void
BackgroundJob<T>::setCleanup(bool val)
{
  _cleanUp = val;
}
//------------------------------------------------------------------------------


template<typename T>
bool BackgroundJob<T>::cleanup()
{
  return _cleanUp ;
}
//------------------------------------------------------------------------------

template<typename T>
bool
BackgroundJob<T>::push(T&& val)
{

  bool retval{false};

  if(_queue.running)
    {
      LockGuard lock(_queue.mtx);
      _queue.messages.emplace_back(val);
      _queue.condt.notify_one();
      lock.unlock();
      retval = true;
    }

  return retval;

}
//------------------------------------------------------------------------------

template<typename T>
bool
BackgroundJob<T>::push(const T& val)
{

  bool retval{false};

  if(_queue.running)
    {
      LockGuard lock(_queue.mtx);
      _queue.messages.push_back(val);
      _queue.newData=true;
      _queue.condt.notify_one();
      lock.unlock();
      retval = true;
    }

  return retval;

}
//------------------------------------------------------------------------------

} // ns


#endif // SBBDEP_BACKGROUNDJOB
