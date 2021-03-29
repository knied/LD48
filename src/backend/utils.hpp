////////////////////////////////////////////////////////////////////////////////

#ifndef BACKEND_UTILS_HPP
#define BACKEND_UTILS_HPP

////////////////////////////////////////////////////////////////////////////////

#include <string>
#include <cassert>

////////////////////////////////////////////////////////////////////////////////

namespace utils {

template <typename T>
inline T
byteSwap(T v) {
    static_assert(sizeof(T) == 1 || sizeof(T) == 2 ||
                  sizeof(T) == 4 || sizeof(T) == 8,
                  "byteSwap only supports sizes up to 8 byte");
    switch (sizeof(T)) {
        case 1: return v; break;
        case 2: return __builtin_bswap16(v); break;
        case 4: return __builtin_bswap32(v); break;
        case 8: return __builtin_bswap64(v); break;
    }
}

template <typename T, bool BigEndian>
inline T
read(char const* cursor) {
    T result = *(T const*)cursor;
    if (BigEndian) {
        return byteSwap(result);
    }
    return result;
}

template <typename T>
inline T
read_be(char const* cursor) {
    return read<T, true>(cursor);
}

template <typename T, bool BigEndian>
inline void
write(char* cursor, T value) {
    if (BigEndian) {
        *(T*)cursor = byteSwap(value);
    } else {
        *(T*)cursor = value;
    }
}

template <typename T>
inline void
write_be(char* cursor, T value) {
    write<T, true>(cursor, value);
}

template<typename T>
inline T
rotl(T value, int shift) {
    if ((shift &= sizeof(value)*8 - 1) == 0)
        return value;
    return (value << shift) | (value >> (sizeof(value)*8 - shift));
}

template<typename T>
inline T
rotr(const T value, int shift) {
    if ((shift &= sizeof(value)*8 - 1) == 0)
        return value;
    return (value >> shift) | (value << (sizeof(value)*8 - shift));
}

struct SHA1 {
  char data[20];
};

SHA1
sha1(std::string const& str) {
  std::uint32_t h0 = 0x67452301;
  std::uint32_t h1 = 0xEFCDAB89;
  std::uint32_t h2 = 0x98BADCFE;
  std::uint32_t h3 = 0x10325476;
  std::uint32_t h4 = 0xC3D2E1F0;
  std::uint64_t ml = str.size() * 8;
  std::uint64_t dl = (((ml + 8 + 64) + 511) / 512) * 512;
  assert(dl >= ml + 8 + 64);
  assert(dl % 512 == 0);
  char* data = new char[dl / 8];
  memcpy(data, str.data(), ml / 8);
  data[ml / 8] = 0x80;
  memset(data + (ml / 8 + 1), 0, dl / 8 - ml / 8 - 1 - 8);
  write_be<std::uint64_t>(data + dl / 8 - 8, ml);
  for (char const* chunk = data; chunk < data + dl / 8; chunk += 64) {
    std::uint32_t w[80];
    for (std::uint32_t i = 0; i < 16; ++i) {
      w[i] = read_be<std::uint32_t>(chunk + i * 4);
    }
    for (std::uint32_t i = 16; i < 80; ++i) {
      w[i] = rotl(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
    }
    std::uint32_t a = h0;
    std::uint32_t b = h1;
    std::uint32_t c = h2;
    std::uint32_t d = h3;
    std::uint32_t e = h4;
    for (std::uint32_t i = 0; i < 80; ++i) {
      std::uint32_t f, k;
      if (i <= 19) {
        f = (b & c) | ((~b) & d);
        k = 0x5A827999;
      } else if (i >= 20 && i <= 39) {
        f = b ^ c ^ d;
        k = 0x6ED9EBA1;
      } else if (i >= 40 && i <= 59) {
        f = (b & c) | (b & d) | (c & d);
        k = 0x8F1BBCDC;
      } else {
        f = b ^ c ^ d;
        k = 0xCA62C1D6;
      }
      std::uint32_t temp = rotl(a, 5) + f + e + k + w[i];
      e = d;
      d = c;
      c = rotl(b, 30);
      b = a;
      a = temp;
    }
    h0 = h0 + a;
    h1 = h1 + b;
    h2 = h2 + c;
    h3 = h3 + d;
    h4 = h4 + e;
  }
  SHA1 hash;
  write_be(hash.data, h0);
  write_be(hash.data + 4, h1);
  write_be(hash.data + 8, h2);
  write_be(hash.data + 12, h3);
  write_be(hash.data + 16, h4);
  delete [] data;
  return hash;
}

static char const base64Map[64] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
  'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
  'w', 'x', 'y', 'z', '0', '1', '2', '3',
  '4', '5', '6', '7', '8', '9', '+', '/'
};

inline std::string
base64(char const* data, std::size_t length) {
  unsigned char const* udata = (unsigned char const*)data;
  std::string result;
  std::uint64_t l = (length / 3) * 3;
  for (unsigned char const* it = udata; it < udata + l; it += 3) {
    unsigned char i0 = it[0] >> 2;
    unsigned char i1 = ((it[0] & 0x3) << 4) | (it[1] >> 4);
    unsigned char i2 = ((it[1] & 0xf) << 2) | (it[2] >> 6);
    unsigned char i3 = it[2] & 0x3f;
    result += base64Map[i0];
    result += base64Map[i1];
    result += base64Map[i2];
    result += base64Map[i3];
  }
  if (length - l == 1) {
    unsigned char const* it = udata + length - 1;
    unsigned char i0 = it[0] >> 2;
    unsigned char i1 = (it[0] & 0x3) << 4;
    result += base64Map[i0];
    result += base64Map[i1];
    result += '=';
    result += '=';
  } else if (length - l == 2) {
    unsigned char const* it = udata + length - 2;
    unsigned char i0 = it[0] >> 2;
    unsigned char i1 = ((it[0] & 0x3) << 4) | (it[1] >> 4);
    unsigned char i2 = (it[1] & 0xf) << 2;
    result += base64Map[i0];
    result += base64Map[i1];
    result += base64Map[i2];
    result += '=';
  }
  return result;
}

} // namespace utils

////////////////////////////////////////////////////////////////////////////////

#endif // BACKEND_UTILS_HPP

////////////////////////////////////////////////////////////////////////////////
