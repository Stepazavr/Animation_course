#pragma once

#include <functional>

template<typename T>
struct Event
{
  using Delegate = std::function<void (const T &)>;
  std::vector<Delegate> callbacks;

  Event &operator+=(Delegate &&delegate)
  {
    callbacks.emplace_back(std::move(delegate));
    return *this;
  }

  void operator()(const T & event) const
  {
    for (const Delegate& delegate : callbacks)
      delegate(event);
  }
};

// Event struct serve as a simple event system.

// How to use:
// 1) Define an event:
// Event<ListenedType> onSomeEvent;
// 2) Add a listener:
// onSomeEvent += [](const ListenedType &event) { /* do something */ };
// onSomeEvent += [](const ListenedType &event) { /* do something else */ };
// 3) Trigger the event:
// onSomeEvent(event);
