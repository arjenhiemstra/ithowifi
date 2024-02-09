// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

namespace StreamUtils {
namespace Polyfills {

template <typename T>
struct remove_reference {
  using type = T;
};

template <typename T>
struct remove_reference<T &> {
  using type = T;
};

template <typename T>
typename remove_reference<T>::type &&move(T &&t) {
  return static_cast<typename remove_reference<T>::type &&>(t);
}

// poor man's std::function<void()>
class function {
  struct callable_base {
    virtual void operator()() = 0;
    virtual ~callable_base() {}
  };

  template <typename Functor>
  struct callable : callable_base {
    Functor functor;
    callable(Functor functor) : functor(functor) {}
    virtual void operator()() {
      functor();
    }
  };

  callable_base *_callable;

 public:
  template <typename Functor>
  function(Functor f) {
    _callable = new callable<Functor>(f);
  }
  function(function &&src) {
    _callable = src._callable, src._callable = 0;
  }
  ~function() {
    delete _callable;
  }
  void operator()() const {
    (*_callable)();
  }
};

}  // namespace Polyfills
}  // namespace StreamUtils