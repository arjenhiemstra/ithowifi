/*
   Servicetool bridge - wire protocol helpers (header-only, no Arduino deps)
   Part of NRG itho wifi firmware

   Emulates the serial framing of the Itho Daalderop "interface cable"
   (Interf.kabel in ID_Servicetool):
     - frames:  DLE STX <raw I2C frame, every 0x10 doubled> DLE ETX
     - ping:    DLE SYN  (10 16), answered with the same two bytes
     - ACK/NAK: DLE ACK / DLE NAK (10 06 / 10 15), parsed but not required

   A raw I2C frame is:
     [0] destination  [1] source  [2..3] msg class (BE)  [4] msg type
     [5] data length  [6..] data  [last] checksum (two's complement of sum)

   Example - device type query from the servicetool (0x80) to the Itho (0x82),
   msg class 0x10E0 with the 0x8000 bit the servicetool always sets, type 4
   (query), no data:

     raw frame:   82 80 90 E0 04 00 8A
     on the wire: 10 02 82 80 90 E0 04 00 8A 10 03

   A 0x10 inside the frame is sent twice (10 10) so it can't be mistaken for
   the start of a DLE control sequence. The checksum is computed over the
   unstuffed frame.

   Msg types: 0 query->broadcast, 1 report->broadcast, 2 change->broadcast,
              4 query->source, 5 report->source, 6 change->source

   Kept free of Arduino/ESP-IDF includes so it can be unit tested natively
   (see test/test_native_servicetool).
*/

#pragma once

#include <cstddef>
#include <cstdint>

namespace servicetool
{

  constexpr uint8_t DLE = 0x10;
  constexpr uint8_t SOH = 0x01;
  constexpr uint8_t STX = 0x02;
  constexpr uint8_t ETX = 0x03;
  constexpr uint8_t ACK = 0x06;
  constexpr uint8_t NAK = 0x15;
  constexpr uint8_t SYN = 0x16;

  // Largest I2C frame the servicetool produces: 6 header + 255 data + 1 cs.
  constexpr size_t MAX_FRAME = 262;

  inline uint8_t frameChecksum(const uint8_t *buf, size_t len)
  {
    uint8_t sum = 0;
    for (size_t i = 0; i < len; i++)
      sum += buf[i];
    return static_cast<uint8_t>(-sum);
  }

  // Length the frame should have according to its data-length byte.
  inline size_t expectedFrameLength(const uint8_t *frame, size_t len)
  {
    if (len < 6)
      return 0;
    return static_cast<size_t>(frame[5]) + 7;
  }

  // Complete, consistent frame with a correct checksum.
  inline bool frameValid(const uint8_t *frame, size_t len)
  {
    if (len < 7 || len > MAX_FRAME)
      return false;
    if (expectedFrameLength(frame, len) != len)
      return false;
    return frameChecksum(frame, len - 1) == frame[len - 1];
  }

  // Message class without the 0x8000 bit the servicetool always sets.
  inline uint16_t frameClass(const uint8_t *frame)
  {
    return static_cast<uint16_t>(((frame[2] << 8) | frame[3]) & 0x7FFF);
  }

  inline uint8_t frameType(const uint8_t *frame) { return frame[4]; }

  // Firmware / bootloader traffic is refused by default: an interrupted
  // flash over WiFi can brick a board.
  //   0x4035..0x4038  RF-plus firmware update blocks (RfPlusUpdater)
  //   0x4F35..0x4F38  APS / Argus bootloader (APSbootLoader, ArgusBootLoader)
  inline bool classBlocked(uint16_t msgClass)
  {
    return (msgClass >= 0x4035 && msgClass <= 0x4038) ||
           (msgClass >= 0x4F35 && msgClass <= 0x4F38);
  }

  // How long to wait for the first byte of a reply, based on the message
  // type of the request only. The servicetool itself waits 200..2000 ms.
  // The bridge aborts the wait as soon as the servicetool sends its next
  // frame, so a generous window does not delay anything.
  //   0 query->broadcast, 4 query->source  : a reply is expected
  //   6 change->source                     : sometimes answered (0x2410)
  //   1/2/5 report/change broadcast        : no reply expected
  // Note: the destination does NOT matter. The servicetool's very first
  // node identification (0x10E0) goes to address 0x00 (general call) and
  // is answered by the Itho; treating 0x00 as "no reply" broke detection.
  inline uint32_t replyWindowMs(const uint8_t *frame)
  {
    switch (frameType(frame))
    {
    case 0:
    case 4:
      return 2000; // = longest ResponseTijd the servicetool uses
    case 6:
      return 1000;
    default:
      return 50;
    }
  }

  // Wrap a raw frame for the serial line. Returns bytes written, 0 if `out`
  // is too small. Worst case output size is 2 * len + 4.
  inline size_t dleEncode(const uint8_t *in, size_t len, uint8_t *out, size_t cap)
  {
    size_t o = 0;
    auto put = [&](uint8_t b) -> bool
    {
      if (o >= cap)
        return false;
      out[o++] = b;
      return true;
    };
    if (!put(DLE) || !put(STX))
      return 0;
    for (size_t i = 0; i < len; i++)
    {
      if (!put(in[i]))
        return 0;
      if (in[i] == DLE && !put(DLE))
        return 0;
    }
    if (!put(DLE) || !put(ETX))
      return 0;
    return o;
  }

  // Byte-at-a-time decoder for the stream coming from the servicetool.
  class DleDecoder
  {
  public:
    enum class Event
    {
      None,
      Ping,
      Frame,
      Ack,
      Nak,
      Overflow
    };

    Event feed(uint8_t b)
    {
      if (escaped_)
      {
        escaped_ = false;
        switch (b)
        {
        case DLE: // stuffed 0x10 inside a frame
          append(DLE);
          return Event::None;
        case STX:
          inFrame_ = true;
          len_ = 0;
          overflow_ = false;
          return Event::None;
        case ETX:
          if (!inFrame_)
            return Event::None;
          inFrame_ = false;
          if (overflow_)
            return Event::Overflow;
          return len_ ? Event::Frame : Event::None;
        case SYN:
          return Event::Ping;
        case ACK:
          return Event::Ack;
        case NAK:
          return Event::Nak;
        case SOH:
          reset();
          return Event::None;
        default:
          return Event::None; // unknown escape: ignore
        }
      }
      if (b == DLE)
      {
        escaped_ = true;
        return Event::None;
      }
      append(b);
      return Event::None;
    }

    const uint8_t *data() const { return buf_; }
    size_t size() const { return len_; }

    void reset()
    {
      escaped_ = false;
      inFrame_ = false;
      overflow_ = false;
      len_ = 0;
    }

  private:
    void append(uint8_t b)
    {
      if (!inFrame_)
        return; // bytes outside a frame are noise
      if (len_ < MAX_FRAME)
        buf_[len_++] = b;
      else
        overflow_ = true;
    }

    uint8_t buf_[MAX_FRAME]{};
    size_t len_ = 0;
    bool escaped_ = false;
    bool inFrame_ = false;
    bool overflow_ = false;
  };

  // Split what the I2C slave collected during one receive window into
  // separate frames. The slave peripheral strips the address byte, so the
  // receive code prepends our own address (0x80) to the *first* frame only;
  // any following frame in the same window lacks its destination byte and
  // gets `ownAddress` prepended here. Calls cb(frame, len) for each complete
  // frame; returns the number of frames found. Trailing garbage is dropped.
  template <typename Callback>
  size_t splitReceived(const uint8_t *buf, size_t len, uint8_t ownAddress, Callback cb)
  {
    size_t pos = 0;
    size_t count = 0;
    uint8_t tmp[MAX_FRAME];

    // first frame: complete, including destination
    if (len < 7)
      return 0;
    size_t flen = expectedFrameLength(buf, len);
    if (flen == 0 || flen > len || flen > MAX_FRAME)
      return 0;
    cb(buf, flen);
    count++;
    pos = flen;

    // following frames: destination missing
    while (len - pos >= 6)
    {
      size_t f = static_cast<size_t>(buf[pos + 4]) + 7; // incl. synthesized dest
      if (f - 1 > len - pos || f > MAX_FRAME)
        break;
      tmp[0] = ownAddress;
      for (size_t i = 0; i < f - 1; i++)
        tmp[i + 1] = buf[pos + i];
      cb(tmp, f);
      count++;
      pos += f - 1;
    }
    return count;
  }

} // namespace servicetool
