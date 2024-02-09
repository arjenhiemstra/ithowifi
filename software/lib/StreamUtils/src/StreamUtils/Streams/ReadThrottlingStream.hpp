// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include "../Policies/ReadThrottlingPolicy.hpp"
#include "../Policies/WriteForwardingPolicy.hpp"
#include "StreamProxy.hpp"

#ifdef ARDUINO
#include "../Ports/ArduinoThrottler.hpp"
#endif

namespace StreamUtils {

template <typename TThrottler>
class BasicReadThrottlingStream
    : public StreamProxy<ReadThrottlingPolicy<TThrottler>,
                         WriteForwardingPolicy> {
 public:
  BasicReadThrottlingStream(Stream& upstream,
                            TThrottler throttler = TThrottler())
      : StreamProxy<ReadThrottlingPolicy<TThrottler>, WriteForwardingPolicy>(
            upstream, ReadThrottlingPolicy<TThrottler>(throttler), {}) {}

  const TThrottler& throttler() const {
    return this->_reader.throttler();
  }
};

#ifdef ARDUINO
using ReadThrottlingStream = BasicReadThrottlingStream<ArduinoThrottler>;
#endif
}  // namespace StreamUtils