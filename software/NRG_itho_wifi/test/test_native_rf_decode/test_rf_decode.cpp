// Native unit tests for the RAMSES-II message decoder rework.
//
// Verifies that the optimized LUT-based block decoder (messageDecodeNew) is
// BYTE-IDENTICAL to the original bit-loop decoder (messageDecodeOrig) for every
// input — valid encodings AND arbitrary garbage — and that a full encode->decode
// roundtrip reproduces the original payload.
//
// The original messageEncode/messageDecode/checksum bodies are copied verbatim
// from IthoCC1101.cpp (only the class prefix removed) so this test is the
// reference oracle for the firmware change.
//
// Note: the RAMSES redundancy/framing bits are NOT validated. Real Itho/Orcon
// frames do not carry the exact (bit,~bit) complement + "10" framing that this
// firmware's messageEncode emits (confirmed on live RF traffic: real frames
// pass the additive checksum but not a complement/framing check), so those bits
// are not a usable integrity signal and the decoder ignores them, as the
// original did.

#include <unity.h>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#define MAX_RAW 162
#define MAX_PAYLOAD 64
#define MAX_DECODED (MAX_PAYLOAD + 18) // 82
#define STARTBYTE 0

struct RawPacket
{
  uint8_t length = 0;
  uint8_t data[MAX_RAW]{};
};

struct DecodedPacket
{
  uint8_t dataDecoded[MAX_DECODED]{};
  uint8_t length{0};
};

// Oversized buffer for the ENCODE side only: messageEncode needs
// 14 + ceil(2.5*L) bytes, which exceeds MAX_RAW for L >= ~57.
struct BigPacket
{
  uint8_t length = 0;
  uint8_t data[256]{};
};

// --------------------------------------------------------------------------
// Verbatim originals (IthoCC1101.cpp:1258-1429, pre-rework)
// --------------------------------------------------------------------------
static uint8_t checksum(const uint8_t *dataDecoded, uint8_t len)
{
  uint8_t val = 0;
  for (uint8_t i = 0; i < len; i++)
    val += dataDecoded[i];
  return 0 - val;
}

template <typename PKT>
static uint8_t messageEncode(const uint8_t *dataDecoded, uint8_t length, PKT *packet)
{
  uint8_t out_bytecounter = 14;
  uint8_t out_bitcounter = 0;
  uint8_t out_patterncounter = 0;
  uint8_t bitSelect = 4;
  uint8_t out_shift = 7;

  for (unsigned int i = out_bytecounter; i < sizeof(packet->data) / sizeof(packet->data[0]); i++)
    packet->data[i] = 0;

  for (uint8_t dataByte = 0; dataByte < length; dataByte++)
  {
    for (uint8_t dataBit = 0; dataBit < 8; dataBit++)
    {
      if (out_bitcounter == 8)
      {
        out_bytecounter++;
        out_bitcounter = 0;
      }
      if (out_patterncounter == 8)
      {
        out_patterncounter = 0;
        packet->data[out_bytecounter] = packet->data[out_bytecounter] | 1 << out_shift;
        out_shift--;
        out_bitcounter++;
        packet->data[out_bytecounter] = packet->data[out_bytecounter] | 0 << out_shift;
        if (out_shift == 0)
          out_shift = 8;
        out_shift--;
        out_bitcounter++;
      }
      if (out_bitcounter == 8)
      {
        out_bytecounter++;
        out_bitcounter = 0;
      }
      uint8_t bit = (dataDecoded[dataByte] & (1 << bitSelect)) >> bitSelect;
      bitSelect++;
      if (bitSelect == 8)
        bitSelect = 0;
      packet->data[out_bytecounter] = packet->data[out_bytecounter] | bit << out_shift;
      out_shift--;
      out_bitcounter++;
      out_patterncounter++;
      bit = ~bit & 0b00000001;
      packet->data[out_bytecounter] = packet->data[out_bytecounter] | bit << out_shift;
      if (out_shift == 0)
        out_shift = 8;
      out_shift--;
      out_bitcounter++;
      out_patterncounter++;
    }
  }
  if (out_bitcounter < 8)
  {
    for (uint8_t i = out_bitcounter; i < 8; i += 2)
    {
      packet->data[out_bytecounter] = packet->data[out_bytecounter] | 1 << out_shift;
      out_shift--;
      packet->data[out_bytecounter] = packet->data[out_bytecounter] | 0 << out_shift;
      if (out_shift == 0)
        out_shift = 8;
      out_shift--;
    }
  }
  return out_bytecounter;
}

static void messageDecodeOrig(RawPacket *packet, DecodedPacket *packetPtr)
{
  packetPtr->length = 0;
  int lenInbuf = packet->length;
  lenInbuf -= STARTBYTE;
  while (lenInbuf >= 5)
  {
    lenInbuf -= 5;
    packetPtr->length += 2;
  }
  if (lenInbuf >= 3)
    packetPtr->length++;

  for (unsigned int i = 0; i < sizeof(packetPtr->dataDecoded) / sizeof(packetPtr->dataDecoded[0]); i++)
    packetPtr->dataDecoded[i] = 0;

  uint8_t out_i = 0;
  uint8_t out_j = 4;
  uint8_t in_bitcounter = 0;

  for (int i = STARTBYTE; i < packet->length; i++)
  {
    for (int j = 7; j > -1; j--)
    {
      if (in_bitcounter == 0 || in_bitcounter == 2 || in_bitcounter == 4 || in_bitcounter == 6)
      {
        uint8_t x = packet->data[i];
        x = x >> j;
        x = x & 0b00000001;
        x = x << out_j;
        packetPtr->dataDecoded[out_i] = packetPtr->dataDecoded[out_i] | x;
        out_j += 1;
        if (out_j > 7)
          out_j = 0;
        if (out_j == 4)
          out_i += 1;
      }
      in_bitcounter += 1;
      if (in_bitcounter > 9)
        in_bitcounter = 0;
    }
  }
}

// --------------------------------------------------------------------------
// New LUT-based block decoder (matches the firmware messageDecode)
// --------------------------------------------------------------------------
static uint8_t decode_lut[256];
static void init_decode_lut()
{
  for (int c = 0; c < 256; c++)
    decode_lut[c] = (uint8_t)(((c >> 7) & 1) | (((c >> 5) & 1) << 1) |
                              (((c >> 3) & 1) << 2) | (((c >> 1) & 1) << 3));
}

static void messageDecodeNew(RawPacket *packet, DecodedPacket *packetPtr)
{
  packetPtr->length = 0;
  int lenInbuf = packet->length;
  lenInbuf -= STARTBYTE;
  while (lenInbuf >= 5)
  {
    lenInbuf -= 5;
    packetPtr->length += 2;
  }
  if (lenInbuf >= 3)
    packetPtr->length++;

  memset(packetPtr->dataDecoded, 0, sizeof(packetPtr->dataDecoded));

  const uint8_t *in = &packet->data[STARTBYTE];
  int n = (int)packet->length - STARTBYTE;
  if (n < 0)
    n = 0;
  int blocks = n / 5;
  int rem = n - blocks * 5;
  uint8_t *out = packetPtr->dataDecoded;

  for (int b = 0; b < blocks; b++, in += 5, out += 2)
  {
    uint32_t hi = ((uint32_t)in[0] << 24) | ((uint32_t)in[1] << 16) |
                  ((uint32_t)in[2] << 8) | (uint32_t)in[3];
    uint32_t lo = in[4];
    uint8_t c0 = (uint8_t)(hi >> 24);
    uint8_t c1 = (uint8_t)((hi >> 14) & 0xFF);
    uint8_t c2 = (uint8_t)((hi >> 4) & 0xFF);
    uint8_t c3 = (uint8_t)(((hi << 6) | (lo >> 2)) & 0xFF);
    out[0] = (uint8_t)((decode_lut[c0] << 4) | decode_lut[c1]);
    out[1] = (uint8_t)((decode_lut[c2] << 4) | decode_lut[c3]);
  }

  if (rem)
  {
    uint8_t out_i = 0;
    uint8_t out_j = 4;
    uint8_t in_bitcounter = 0;
    for (int i = 0; i < rem; i++)
    {
      uint8_t byte = in[i];
      for (int j = 7; j > -1; j--)
      {
        if ((in_bitcounter & 1) == 0 && in_bitcounter < 8)
        {
          uint8_t x = (uint8_t)((byte >> j) & 1u);
          out[out_i] = out[out_i] | (uint8_t)(x << out_j);
          out_j += 1;
          if (out_j > 7)
            out_j = 0;
          if (out_j == 4)
            out_i += 1;
        }
        in_bitcounter += 1;
        if (in_bitcounter > 9)
          in_bitcounter = 0;
      }
    }
  }
}

// --------------------------------------------------------------------------
// New LUT-based block encoder (matches the firmware messageEncode)
// --------------------------------------------------------------------------
// Each nibble m=(m3 m2 m1 m0) -> a fixed 10-bit group [m0 ~m0 m1 ~m1 m2 ~m2 m3
// ~m3 1 0] (data bits low-to-nibble-high, each followed by its complement, then
// the "10" framing). A byte = high-nibble group (10 bits) then low-nibble group
// (10 bits) = 20 bits; two bytes = exactly 5 raw bytes.
static uint16_t encode_group_lut[16];
static void init_encode_lut()
{
  for (int m = 0; m < 16; m++)
  {
    int m0 = m & 1, m1 = (m >> 1) & 1, m2 = (m >> 2) & 1, m3 = (m >> 3) & 1;
    encode_group_lut[m] = (uint16_t)((m0 << 9) | ((m0 ^ 1) << 8) | (m1 << 7) | ((m1 ^ 1) << 6) |
                                     (m2 << 5) | ((m2 ^ 1) << 4) | (m3 << 3) | ((m3 ^ 1) << 2) |
                                     (1 << 1) | 0);
  }
}

template <typename PKT>
static uint8_t messageEncodeNew(const uint8_t *dataDecoded, uint8_t length, PKT *packet)
{
  for (unsigned int i = 14; i < sizeof(packet->data) / sizeof(packet->data[0]); i++)
    packet->data[i] = 0;

  int outIdx = 14;
  uint64_t acc = 0;
  int accBits = 0;
  for (uint8_t k = 0; k < length; k++)
  {
    uint8_t B = dataDecoded[k];
    uint32_t bits20 = ((uint32_t)encode_group_lut[B >> 4] << 10) | encode_group_lut[B & 0x0F];
    acc = (acc << 20) | bits20;
    accBits += 20;
    while (accBits >= 8)
    {
      packet->data[outIdx++] = (uint8_t)((acc >> (accBits - 8)) & 0xFF);
      accBits -= 8;
    }
  }
  // closing framing: fill the remaining bits of the last partial byte with "10"
  if (accBits > 0)
  {
    while (accBits < 8)
    {
      acc = (acc << 2) | 0x2; // "1 0"
      accBits += 2;
    }
    packet->data[outIdx++] = (uint8_t)((acc >> (accBits - 8)) & 0xFF);
    accBits -= 8;
  }
  return (uint8_t)(outIdx - 1); // index of last written byte, as the original returns
}

// --------------------------------------------------------------------------
// deterministic PRNG
// --------------------------------------------------------------------------
static uint64_t rng_state;
static void rng_seed(uint64_t s) { rng_state = s ? s : 0x243F6A8885A308D3ull; }
static uint32_t xrand()
{
  rng_state ^= rng_state >> 12;
  rng_state ^= rng_state << 25;
  rng_state ^= rng_state >> 27;
  return (uint32_t)((rng_state * 0x2545F4914F6CDD1Dull) >> 32);
}

static bool decoders_agree(RawPacket &rx)
{
  RawPacket a = rx, b = rx;
  DecodedPacket da, db;
  messageDecodeOrig(&a, &da);
  messageDecodeNew(&b, &db);
  return da.length == db.length &&
         memcmp(da.dataDecoded, db.dataDecoded, MAX_DECODED) == 0;
}

static void encode_payload(const uint8_t *payload, uint8_t len, RawPacket &rx)
{
  uint8_t buf[MAX_DECODED]{};
  memcpy(buf, payload, len);
  BigPacket tx;
  uint8_t last = messageEncode(buf, len, &tx);
  int n = (int)last + 1 - 14;
  memset(&rx, 0, sizeof(rx));
  rx.length = (uint8_t)n;
  memcpy(rx.data, &tx.data[14], (size_t)n);
}

// --------------------------------------------------------------------------
// Tests
// --------------------------------------------------------------------------
void test_equivalence_valid_encodings(void)
{
  rng_seed(0x1111);
  for (int len = 1; len <= MAX_PAYLOAD; len++)
    for (int iter = 0; iter < 60; iter++)
    {
      uint8_t payload[MAX_DECODED]{};
      for (int i = 0; i < len; i++)
        payload[i] = (uint8_t)xrand();
      if (len >= 2)
        payload[len - 1] = checksum(payload, (uint8_t)(len - 1));
      RawPacket rx;
      encode_payload(payload, (uint8_t)len, rx);
      TEST_ASSERT_TRUE_MESSAGE(decoders_agree(rx), "new decoder differs from original on a valid encoding");
    }
}

void test_equivalence_garbage(void)
{
  rng_seed(0x2222);
  for (int len = 0; len <= MAX_RAW; len++)
    for (int iter = 0; iter < 60; iter++)
    {
      RawPacket rx;
      rx.length = (uint8_t)len;
      for (int i = 0; i < MAX_RAW; i++)
        rx.data[i] = (uint8_t)xrand();
      TEST_ASSERT_TRUE_MESSAGE(decoders_agree(rx), "new decoder differs from original on garbage input");
    }
}

void test_equivalence_patterns(void)
{
  const uint8_t pats[] = {0x00, 0xFF, 0xAA, 0x55, 0xB2, 0x4D};
  for (int len = 0; len <= MAX_RAW; len++)
    for (unsigned p = 0; p < sizeof(pats); p++)
    {
      RawPacket rx;
      rx.length = (uint8_t)len;
      memset(rx.data, pats[p], MAX_RAW);
      TEST_ASSERT_TRUE_MESSAGE(decoders_agree(rx), "new decoder differs from original on pattern input");
    }
}

void test_roundtrip_reproduces_payload(void)
{
  rng_seed(0x3333);
  for (int len = 1; len <= MAX_PAYLOAD; len++)
    for (int iter = 0; iter < 40; iter++)
    {
      uint8_t payload[MAX_DECODED]{};
      for (int i = 0; i < len; i++)
        payload[i] = (uint8_t)xrand();
      RawPacket rx;
      encode_payload(payload, (uint8_t)len, rx);
      DecodedPacket back;
      messageDecodeNew(&rx, &back);
      TEST_ASSERT_EQUAL_UINT8(len, back.length);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, back.dataDecoded, len);
    }
}

void test_encode_equivalence(void)
{
  // New LUT encoder must be byte-identical to the original messageEncode,
  // including the returned last-index, for every payload length.
  rng_seed(0x6666);
  for (int len = 1; len <= MAX_PAYLOAD; len++)
    for (int iter = 0; iter < 60; iter++)
    {
      uint8_t payload[MAX_DECODED]{};
      for (int i = 0; i < len; i++)
        payload[i] = (uint8_t)xrand();
      BigPacket a, b;
      uint8_t ra = messageEncode(payload, (uint8_t)len, &a);
      uint8_t rb = messageEncodeNew(payload, (uint8_t)len, &b);
      TEST_ASSERT_EQUAL_UINT8_MESSAGE(ra, rb, "encoder return (last index) differs");
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(a.data, b.data, sizeof(a.data), "encoder output bytes differ");
    }
}

void test_encode_decode_roundtrip_new(void)
{
  // New encoder + new decoder must reproduce the payload.
  rng_seed(0x7777);
  for (int len = 1; len <= MAX_PAYLOAD; len++)
    for (int iter = 0; iter < 40; iter++)
    {
      uint8_t payload[MAX_DECODED]{};
      for (int i = 0; i < len; i++)
        payload[i] = (uint8_t)xrand();
      BigPacket tx;
      uint8_t last = messageEncodeNew(payload, (uint8_t)len, &tx);
      int n = (int)last + 1 - 14;
      RawPacket rx;
      memset(&rx, 0, sizeof(rx));
      rx.length = (uint8_t)n;
      memcpy(rx.data, &tx.data[14], (size_t)n);
      DecodedPacket back;
      messageDecodeNew(&rx, &back);
      TEST_ASSERT_EQUAL_UINT8(len, back.length);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, back.dataDecoded, len);
    }
}

void setUp(void) {}
void tearDown(void) {}

int main()
{
  init_decode_lut();
  init_encode_lut();
  UNITY_BEGIN();
  RUN_TEST(test_equivalence_valid_encodings);
  RUN_TEST(test_equivalence_garbage);
  RUN_TEST(test_equivalence_patterns);
  RUN_TEST(test_roundtrip_reproduces_payload);
  RUN_TEST(test_encode_equivalence);
  RUN_TEST(test_encode_decode_roundtrip_new);
  return UNITY_END();
}
