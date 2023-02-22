#pragma once

#include <thread>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <vector>
#include <list>

class Scheduler {
public:
  using AsyncFunction = std::function<void()>;

  Scheduler() {
    const auto count = std::max(std::thread::hardware_concurrency(), 2u) - 1;
    m_threads.resize(count);
    for (auto i = 0u; i < count; ++i)
      m_threads[i] = std::thread(&Scheduler::thread_func, this);
  }

  Scheduler(const Scheduler&) = delete;
  Scheduler& operator=(const Scheduler&) = delete;

  ~Scheduler() {
    auto lock = std::unique_lock(m_tasks_mutex);
    m_shutdown = true;
    lock.unlock();
    m_tasks_signal.notify_all();
    for (auto& thread : m_threads)
      thread.join();
  }

  void async(AsyncFunction function) noexcept {
    auto lock = std::unique_lock(m_tasks_mutex);
    m_tasks.push_back({ std::move(function), 1, 1 });
    ++m_tasks_pending;
    lock.unlock();
    m_tasks_signal.notify_all();
  }

  template<typename F> // F(size_t)
  void for_each_parallel(F&& function, size_t count) {
    if (!count)
      return;

    auto exception = std::exception_ptr();
    auto next_index = std::atomic<size_t>{ };
    auto local_remaining = std::atomic<size_t>{ count };
    const auto parallel_function = [&]() noexcept {
      try {
        function(next_index.fetch_add(1));
      }
      catch (...) {
        auto lock = std::lock_guard(m_tasks_mutex);
        exception = std::current_exception();
      }
      local_remaining.fetch_sub(1);
    };

    auto lock = std::unique_lock(m_tasks_mutex);
    m_tasks.push_back({ parallel_function, count, count });
    m_tasks_pending += count;
    m_tasks_signal.notify_all();
    while (next_index < count)
      execute_next(lock);
    
    m_done_signal.wait(lock, [&]() { return (local_remaining.load() == 0); });
    if (exception)
      std::rethrow_exception(exception);
  }

  template<typename It, typename F> // F(*It)
  void for_each_parallel(It begin, It end, F&& function) {
    const auto count = static_cast<size_t>(std::distance(begin, end));
    for_each_parallel(
      [begin, function = std::forward<F>(function)](size_t index) {
        function(*std::next(begin, static_cast<int>(index)));
      }, count);
  }

private:
  void thread_func() {
    auto lock = std::unique_lock(m_tasks_mutex);
    for (;;) {
      m_tasks_signal.wait(lock,
        [&]() { return (m_shutdown || m_tasks_pending); });
      if (m_shutdown)
        break;
      execute_next(lock);
    }
  }

  void execute_next(std::unique_lock<std::mutex>& lock) {
    auto it = m_tasks.begin();
    for (;; ++it) {
      if (it == m_tasks.end())
        return;
      if (it->count > 0) {
        --m_tasks_pending;
        --it->count;
        break;
      }
    }
    lock.unlock();

    it->function();

    lock.lock();
    --it->remaining;
    if (it->remaining == 0) {
      m_tasks.erase(it);
      m_done_signal.notify_all();
    }
  }

  struct Task {
    AsyncFunction function;
    size_t count;
    size_t remaining;
  };

  std::vector<std::thread> m_threads;
  std::condition_variable m_tasks_signal;
  std::condition_variable m_done_signal;
  std::mutex m_tasks_mutex;
  std::list<Task> m_tasks;
  size_t m_tasks_pending{ };
  bool m_shutdown{ };
};
