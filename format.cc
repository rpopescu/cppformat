/*
 Small, safe and fast string formatting library for C++
 Author: Victor Zverovich
 */

#include "format.h"

#include <stdint.h>

#include <cassert>
#include <climits>
#include <cstring>
#include <algorithm>

using std::size_t;

namespace {

// Flags.
enum { PLUS_FLAG = 1, ZERO_FLAG = 2, HEX_PREFIX_FLAG = 4 };

// Throws Exception(message) if format contains '}', otherwise throws
// FormatError reporting unmatched '{'. The idea is that unmatched '{'
// should override other errors.
void ReportError(const char *s, const std::string &message) {
  for (int num_open_braces = 1; *s; ++s) {
    if (*s == '{') {
      ++num_open_braces;
    } else if (*s == '}') {
      if (--num_open_braces == 0)
        throw fmt::FormatError(message);
    }
  }
  throw fmt::FormatError("unmatched '{' in format");
}

void ReportUnknownType(char code, const char *type) {
  if (std::isprint(code)) {
    throw fmt::FormatError(
        str(fmt::Format("unknown format code '{0}' for {1}") << code << type));
  }
  throw fmt::FormatError(
      str(fmt::Format("unknown format code '\\x{0:02x}' for {1}")
        << static_cast<unsigned>(code) << type));
}

// Parses an unsigned integer advancing s to the end of the parsed input.
// This function assumes that the first character of s is a digit.
unsigned ParseUInt(const char *&s) {
  assert('0' <= *s && *s <= '9');
  unsigned value = 0;
  do {
    unsigned new_value = value * 10 + (*s++ - '0');
    if (new_value < value)  // Check if value wrapped around.
      ReportError(s, "number is too big in format");
    value = new_value;
  } while ('0' <= *s && *s <= '9');
  return value;
}

// Maps an integer type T to its unsigned counterpart.
template <typename T>
struct GetUnsigned;

template <>
struct GetUnsigned<int> {
  typedef unsigned Type;
};

template <>
struct GetUnsigned<unsigned> {
  typedef unsigned Type;
};

template <>
struct GetUnsigned<long> {
  typedef unsigned long Type;
};

template <>
struct GetUnsigned<unsigned long> {
  typedef unsigned long Type;
};

template <typename T>
struct IsLongDouble { enum {VALUE = 0}; };

template <>
struct IsLongDouble<long double> { enum {VALUE = 1}; };
}

template <typename T>
void fmt::Formatter::FormatInt(T value, unsigned flags, int width, char type) {
  int size = 0;
  char sign = 0;
  typedef typename GetUnsigned<T>::Type UnsignedType;
  UnsignedType abs_value = value;
  if (value < 0) {
    sign = '-';
    ++size;
    abs_value = -value;
  } else if ((flags & PLUS_FLAG) != 0) {
    sign = '+';
    ++size;
  }
  char fill = (flags & ZERO_FLAG) != 0 ? '0' : ' ';
  size_t start = buffer_.size();
  char *p = 0;
  switch (type) {
  case 0: case 'd': {
    UnsignedType n = abs_value;
    do {
      ++size;
    } while ((n /= 10) != 0);
    width = std::max(width, size);
    p = GrowBuffer(width) + width - 1;
    n = abs_value;
    do {
      *p-- = '0' + (n % 10);
    } while ((n /= 10) != 0);
    break;
  }
  case 'x': case 'X': {
    UnsignedType n = abs_value;
    bool print_prefix = (flags & HEX_PREFIX_FLAG) != 0;
    if (print_prefix) size += 2;
    do {
      ++size;
    } while ((n >>= 4) != 0);
    width = std::max(width, size);
    p = GrowBuffer(width) + width - 1;
    n = abs_value;
    const char *digits = type == 'x' ? "0123456789abcdef" : "0123456789ABCDEF";
    do {
      *p-- = digits[n & 0xf];
    } while ((n >>= 4) != 0);
    if (print_prefix) {
      *p-- = type;
      *p-- = '0';
    }
    break;
  }
  case 'o': {
    UnsignedType n = abs_value;
    do {
      ++size;
    } while ((n >>= 3) != 0);
    width = std::max(width, size);
    p = GrowBuffer(width) + width - 1;
    n = abs_value;
    do {
      *p-- = '0' + (n & 7);
    } while ((n >>= 3) != 0);
    break;
  }
  default:
    ReportUnknownType(type, "integer");
    break;
  }
  if (sign) {
    if ((flags & ZERO_FLAG) != 0)
      buffer_[start++] = sign;
    else
      *p-- = sign;
  }
  std::fill(&buffer_[start], p + 1, fill);
}

template <typename T>
void fmt::Formatter::FormatDouble(
    T value, unsigned flags, int width, int precision, char type) {
  // Check type.
  switch (type) {
  case 0:
    type = 'g';
    break;
  case 'e': case 'E': case 'f': case 'F': case 'g': case 'G':
    break;
  default:
    ReportUnknownType(type, "double");
    break;
  }

  // Build format string.
  enum { MAX_FORMAT_SIZE = 9}; // longest format: %+0*.*Lg
  char format[MAX_FORMAT_SIZE];
  char *format_ptr = format;
  *format_ptr++ = '%';
  if ((flags & PLUS_FLAG) != 0)
    *format_ptr++ = '+';
  if ((flags & ZERO_FLAG) != 0)
    *format_ptr++ = '0';
  if (width > 0)
    *format_ptr++ = '*';
  if (precision >= 0) {
    *format_ptr++ = '.';
    *format_ptr++ = '*';
  }
  if (IsLongDouble<T>::VALUE)
    *format_ptr++ = 'L';
  *format_ptr++ = type;
  *format_ptr = '\0';

  // Format using snprintf.
  size_t offset = buffer_.size();
  for (;;) {
    size_t size = buffer_.capacity() - offset;
    int n = 0;
    if (width <= 0) {
      n = precision < 0 ?
          snprintf(&buffer_[offset], size, format, value) :
          snprintf(&buffer_[offset], size, format, precision, value);
    } else {
      n = precision < 0 ?
          snprintf(&buffer_[offset], size, format, width, value) :
          snprintf(&buffer_[offset], size, format, width, precision, value);
    }
    if (n >= 0 && offset + n < buffer_.capacity()) {
      GrowBuffer(n);
      return;
    }
    buffer_.reserve(n >= 0 ? offset + n + 1 : 2 * buffer_.capacity());
  }
}

void fmt::Formatter::DoFormat() {
  const char *start = format_;
  format_ = 0;
  const char *s = start;
  while (*s) {
    char c = *s++;
    if (c != '{' && c != '}') continue;
    if (*s == c) {
      buffer_.append(start, s);
      start = ++s;
      continue;
    }
    if (c == '}')
      throw FormatError("unmatched '}' in format");
    buffer_.append(start, s - 1);

    // Parse argument index.
    if (*s < '0' || *s > '9')
      ReportError(s, "missing argument index in format string");
    unsigned arg_index = ParseUInt(s);
    if (arg_index >= args_.size())
      ReportError(s, "argument index is out of range in format");
    const Arg &arg = *args_[arg_index];

    unsigned flags = 0;
    int width = 0;
    int precision = -1;
    char type = 0;
    if (*s == ':') {
      ++s;
      if (*s == '+') {
        ++s;
        if (arg.type > LAST_NUMERIC_TYPE)
          ReportError(s, "format specifier '+' requires numeric argument");
        if (arg.type == UINT || arg.type == ULONG) {
          ReportError(s,
              "format specifier '+' requires signed argument");
        }
        flags |= PLUS_FLAG;
      }
      if (*s == '0') {
        ++s;
        if (arg.type > LAST_NUMERIC_TYPE)
          ReportError(s, "format specifier '0' requires numeric argument");
        flags |= ZERO_FLAG;
      }

      // Parse width.
      if ('0' <= *s && *s <= '9') {
        unsigned value = ParseUInt(s);
        if (value > INT_MAX)
          ReportError(s, "number is too big in format");
        width = value;
      }

      // Parse precision.
      if (*s == '.') {
        ++s;
        precision = 0;
        if ('0' <= *s && *s <= '9') {
          unsigned value = ParseUInt(s);
          if (value > INT_MAX)
            ReportError(s, "number is too big in format");
          precision = value;
        } else {
          ReportError(s, "missing precision in format");
        }
        if (arg.type != DOUBLE && arg.type != LONG_DOUBLE) {
          ReportError(s,
              "precision specifier requires floating-point argument");
        }
      }

      // Parse type.
      if (*s != '}' && *s)
        type = *s++;
    }

    if (*s++ != '}')
      throw FormatError("unmatched '{' in format");
    start = s;

    // Format argument.
    switch (arg.type) {
    case INT:
      FormatInt(arg.int_value, flags, width, type);
      break;
    case UINT:
      FormatInt(arg.uint_value, flags, width, type);
      break;
    case LONG:
      FormatInt(arg.long_value, flags, width, type);
      break;
    case ULONG:
      FormatInt(arg.ulong_value, flags, width, type);
      break;
    case DOUBLE:
      FormatDouble(arg.double_value, flags, width, precision, type);
      break;
    case LONG_DOUBLE:
      FormatDouble(arg.long_double_value, flags, width, precision, type);
      break;
    case CHAR: {
      if (type && type != 'c')
        ReportUnknownType(type, "char");
      char *out = GrowBuffer(std::max(width, 1));
      *out++ = arg.int_value;
      if (width > 1)
        std::fill_n(out, width - 1, ' ');
      break;
    }
    case STRING: {
      if (type && type != 's')
        ReportUnknownType(type, "string");
      const char *str = arg.string_value;
      size_t size = arg.size;
      if (size == 0 && *str)
        size = std::strlen(str);
      char *out = GrowBuffer(std::max<size_t>(width, size));
      out = std::copy(str, str + size, out);
      if (width > size)
        std::fill_n(out, width - size, ' ');
      break;
    }
    case POINTER:
      if (type && type != 'p')
        ReportUnknownType(type, "pointer");
      FormatInt(reinterpret_cast<uintptr_t>(
          arg.pointer_value), HEX_PREFIX_FLAG, width, 'x');
      break;
    case CUSTOM:
      if (type)
        ReportUnknownType(type, "object");
      (this->*arg.format)(arg.custom_value, width);
      break;
    default:
      assert(false);
      break;
    }
  }
  buffer_.append(start, s + 1);
  buffer_.resize(buffer_.size() - 1);  // Don't count the terminating zero.
}