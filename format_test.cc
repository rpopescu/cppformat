/*
 Formatting library tests.
 Author: Victor Zverovich
 */

#include <cfloat>
#include <climits>
#include <cstring>
#include <gtest/gtest.h>
#include "format.h"

using std::size_t;
using std::sprintf;

using fmt::Formatter;
using fmt::Format;
using fmt::FormatError;

#define FORMAT_TEST_THROW_(statement, expected_exception, message, fail) \
  GTEST_AMBIGUOUS_ELSE_BLOCKER_ \
  if (::testing::internal::ConstCharPtr gtest_msg = "") { \
    bool gtest_caught_expected = false; \
    std::string actual_message; \
    try { \
      GTEST_SUPPRESS_UNREACHABLE_CODE_WARNING_BELOW_(statement); \
    } \
    catch (expected_exception const& e) { \
      gtest_caught_expected = true; \
      actual_message = e.what(); \
    } \
    catch (...) { \
      gtest_msg.value = \
          "Expected: " #statement " throws an exception of type " \
          #expected_exception ".\n  Actual: it throws a different type."; \
      goto GTEST_CONCAT_TOKEN_(gtest_label_testthrow_, __LINE__); \
    } \
    if (!gtest_caught_expected) { \
      gtest_msg.value = \
          "Expected: " #statement " throws an exception of type " \
          #expected_exception ".\n  Actual: it throws nothing."; \
      goto GTEST_CONCAT_TOKEN_(gtest_label_testthrow_, __LINE__); \
    } else if (actual_message != message) {\
      gtest_msg.value = \
          "Expected: " #statement " throws an exception of type " \
          #expected_exception "."; \
      goto GTEST_CONCAT_TOKEN_(gtest_label_testthrow_, __LINE__); \
    } \
  } else \
    GTEST_CONCAT_TOKEN_(gtest_label_testthrow_, __LINE__): \
      fail(gtest_msg.value)

#define EXPECT_THROW_MSG(statement, expected_exception, expected_message) \
  FORMAT_TEST_THROW_(statement, expected_exception, expected_message, \
      GTEST_NONFATAL_FAILURE_)

class TestString {
 private:
  std::string value_;

 public:
  explicit TestString(const char *value = "") : value_(value) {}

  friend std::ostream &operator<<(std::ostream &os, const TestString &s) {
    os << s.value_;
    return os;
  }
};

TEST(ArrayTest, Ctor) {
  fmt::Array<char, 123> array;
  EXPECT_EQ(0, array.size());
  EXPECT_EQ(123, array.capacity());
}

TEST(ArrayTest, Access) {
  fmt::Array<char, 10> array;
  array[0] = 11;
  EXPECT_EQ(11, array[0]);
  array[3] = 42;
  EXPECT_EQ(42, *(&array[0] + 3));
  const fmt::Array<char, 10> &carray = array;
  EXPECT_EQ(42, carray[3]);
}

TEST(ArrayTest, Resize) {
  fmt::Array<char, 123> array;
  array[10] = 42;
  EXPECT_EQ(42, array[10]);
  array.resize(20);
  EXPECT_EQ(20, array.size());
  EXPECT_EQ(123, array.capacity());
  EXPECT_EQ(42, array[10]);
  array.resize(5);
  EXPECT_EQ(5, array.size());
  EXPECT_EQ(123, array.capacity());
  EXPECT_EQ(42, array[10]);
}

TEST(ArrayTest, Grow) {
  fmt::Array<int, 10> array;
  array.resize(10);
  for (int i = 0; i < 10; ++i)
    array[i] = i * i;
  array.resize(20);
  EXPECT_EQ(20, array.size());
  EXPECT_EQ(20, array.capacity());
  for (int i = 0; i < 10; ++i)
    EXPECT_EQ(i * i, array[i]);
}

TEST(ArrayTest, Clear) {
  fmt::Array<char, 10> array;
  array.resize(20);
  array.clear();
  EXPECT_EQ(0, array.size());
  EXPECT_EQ(20, array.capacity());
}

TEST(ArrayTest, PushBack) {
  fmt::Array<int, 10> array;
  array.push_back(11);
  EXPECT_EQ(11, array[0]);
  EXPECT_EQ(1, array.size());
  array.resize(10);
  array.push_back(22);
  EXPECT_EQ(22, array[10]);
  EXPECT_EQ(11, array.size());
  EXPECT_EQ(15, array.capacity());
}

TEST(ArrayTest, Append) {
  fmt::Array<char, 10> array;
  const char *test = "test";
  array.append(test, test + 5);
  EXPECT_STREQ("test", &array[0]);
  EXPECT_EQ(5, array.size());
  array.resize(10);
  array.append(test, test + 2);
  EXPECT_EQ('t', array[10]);
  EXPECT_EQ('e', array[11]);
  EXPECT_EQ(12, array.size());
  EXPECT_EQ(15, array.capacity());
}

TEST(FormatterTest, Escape) {
  EXPECT_EQ("{", str(Format("{{")));
  EXPECT_EQ("before {", str(Format("before {{")));
  EXPECT_EQ("{ after", str(Format("{{ after")));
  EXPECT_EQ("before { after", str(Format("before {{ after")));

  EXPECT_EQ("}", str(Format("}}")));
  EXPECT_EQ("before }", str(Format("before }}")));
  EXPECT_EQ("} after", str(Format("}} after")));
  EXPECT_EQ("before } after", str(Format("before }} after")));

  EXPECT_EQ("{}", str(Format("{{}}")));
  EXPECT_EQ("{42}", str(Format("{{{0}}}") << 42));
}

TEST(FormatterTest, UnmatchedBraces) {
  EXPECT_THROW_MSG(Format("{"), FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(Format("}"), FormatError, "unmatched '}' in format");
  EXPECT_THROW_MSG(Format("{0{}"), FormatError, "unmatched '{' in format");
}

TEST(FormatterTest, NoArgs) {
  EXPECT_EQ("test", str(Format("test")));
}

TEST(FormatterTest, ArgsInDifferentPositions) {
  EXPECT_EQ("42", str(Format("{0}") << 42));
  EXPECT_EQ("before 42", str(Format("before {0}") << 42));
  EXPECT_EQ("42 after", str(Format("{0} after") << 42));
  EXPECT_EQ("before 42 after", str(Format("before {0} after") << 42));
  EXPECT_EQ("answer = 42", str(Format("{0} = {1}") << "answer" << 42));
  EXPECT_EQ("42 is the answer",
      str(Format("{1} is the {0}") << "answer" << 42));
  EXPECT_EQ("abracadabra", str(Format("{0}{1}{0}") << "abra" << "cad"));
}

TEST(FormatterTest, ArgErrors) {
  EXPECT_THROW_MSG(Format("{"), FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(Format("{}"), FormatError,
      "missing argument index in format string");
  EXPECT_THROW_MSG(Format("{0"), FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(Format("{0}"), FormatError,
      "argument index is out of range in format");
  char format[256];
  std::sprintf(format, "{%u", UINT_MAX);
  EXPECT_THROW_MSG(Format(format), FormatError, "unmatched '{' in format");
  std::sprintf(format, "{%u}", UINT_MAX);
  EXPECT_THROW_MSG(Format(format), FormatError,
      "argument index is out of range in format");
  if (ULONG_MAX > UINT_MAX) {
    std::sprintf(format, "{%lu", UINT_MAX + 1l);
    EXPECT_THROW_MSG(Format(format), FormatError, "unmatched '{' in format");
    std::sprintf(format, "{%lu}", UINT_MAX + 1l);
    EXPECT_THROW_MSG(Format(format),
        FormatError, "number is too big in format");
  } else {
    std::sprintf(format, "{%u0", UINT_MAX);
    EXPECT_THROW_MSG(Format(format), FormatError, "unmatched '{' in format");
    std::sprintf(format, "{%u0}", UINT_MAX);
    EXPECT_THROW_MSG(Format(format),
        FormatError, "number is too big in format");
  }
}

TEST(FormatterTest, EmptySpecs) {
  EXPECT_EQ("42", str(Format("{0:}") << 42));
}

TEST(FormatterTest, PlusFlag) {
  EXPECT_EQ("+42", str(Format("{0:+}") << 42));
  EXPECT_EQ("-42", str(Format("{0:+}") << -42));
  EXPECT_EQ("+42", str(Format("{0:+}") << 42));
  EXPECT_THROW_MSG(Format("{0:+}") << 42u,
      FormatError, "format specifier '+' requires signed argument");
  EXPECT_EQ("+42", str(Format("{0:+}") << 42l));
  EXPECT_THROW_MSG(Format("{0:+}") << 42ul,
      FormatError, "format specifier '+' requires signed argument");
  EXPECT_EQ("+42", str(Format("{0:+}") << 42.0));
  EXPECT_EQ("+42", str(Format("{0:+}") << 42.0l));
  EXPECT_THROW_MSG(Format("{0:+") << 'c',
      FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(Format("{0:+}") << 'c',
      FormatError, "format specifier '+' requires numeric argument");
  EXPECT_THROW_MSG(Format("{0:+}") << "abc",
      FormatError, "format specifier '+' requires numeric argument");
  EXPECT_THROW_MSG(Format("{0:+}") << reinterpret_cast<void*>(0x42),
      FormatError, "format specifier '+' requires numeric argument");
  EXPECT_THROW_MSG(Format("{0:+}") << TestString(),
      FormatError, "format specifier '+' requires numeric argument");
}

TEST(FormatterTest, ZeroFlag) {
  EXPECT_EQ("42", str(Format("{0:0}") << 42));
  EXPECT_EQ("-0042", str(Format("{0:05}") << -42));
  EXPECT_EQ("00042", str(Format("{0:05}") << 42u));
  EXPECT_EQ("-0042", str(Format("{0:05}") << -42l));
  EXPECT_EQ("00042", str(Format("{0:05}") << 42ul));
  EXPECT_EQ("-0042", str(Format("{0:05}") << -42.0));
  EXPECT_EQ("-0042", str(Format("{0:05}") << -42.0l));
  EXPECT_THROW_MSG(Format("{0:0") << 'c',
      FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(Format("{0:05}") << 'c',
      FormatError, "format specifier '0' requires numeric argument");
  EXPECT_THROW_MSG(Format("{0:05}") << "abc",
      FormatError, "format specifier '0' requires numeric argument");
  EXPECT_THROW_MSG(Format("{0:05}") << reinterpret_cast<void*>(0x42),
      FormatError, "format specifier '0' requires numeric argument");
  EXPECT_THROW_MSG(Format("{0:05}") << TestString(),
      FormatError, "format specifier '0' requires numeric argument");
}

TEST(FormatterTest, Width) {
  char format[256];
  if (ULONG_MAX > UINT_MAX) {
    std::sprintf(format, "{0:%lu", INT_MAX + 1l);
    EXPECT_THROW_MSG(Format(format), FormatError, "unmatched '{' in format");
    std::sprintf(format, "{0:%lu}", UINT_MAX + 1l);
    EXPECT_THROW_MSG(Format(format) << 0,
        FormatError, "number is too big in format");
  } else {
    std::sprintf(format, "{0:%u0", UINT_MAX);
    EXPECT_THROW_MSG(Format(format), FormatError, "unmatched '{' in format");
    std::sprintf(format, "{0:%u0}", UINT_MAX);
    EXPECT_THROW_MSG(Format(format) << 0,
        FormatError, "number is too big in format");
  }
  std::sprintf(format, "{0:%u", INT_MAX + 1u);
  EXPECT_THROW_MSG(Format(format), FormatError, "unmatched '{' in format");
  std::sprintf(format, "{0:%u}", INT_MAX + 1u);
  EXPECT_THROW_MSG(Format(format) << 0,
      FormatError, "number is too big in format");
  EXPECT_EQ(" -42", str(Format("{0:4}") << -42));
  EXPECT_EQ("   42", str(Format("{0:5}") << 42u));
  EXPECT_EQ("   -42", str(Format("{0:6}") << -42l));
  EXPECT_EQ("     42", str(Format("{0:7}") << 42ul));
  EXPECT_EQ("   -1.23", str(Format("{0:8}") << -1.23));
  EXPECT_EQ("    -1.23", str(Format("{0:9}") << -1.23l));
  EXPECT_EQ("    0xcafe",
      str(Format("{0:10}") << reinterpret_cast<void*>(0xcafe)));
  EXPECT_EQ("x          ", str(Format("{0:11}") << 'x'));
  EXPECT_EQ("str         ", str(Format("{0:12}") << "str"));
  EXPECT_EQ("test         ", str(Format("{0:13}") << TestString("test")));
}

TEST(FormatterTest, Precision) {
  char format[256];
  if (ULONG_MAX > UINT_MAX) {
    std::sprintf(format, "{0:.%lu", INT_MAX + 1l);
    EXPECT_THROW_MSG(Format(format), FormatError, "unmatched '{' in format");
    std::sprintf(format, "{0:.%lu}", UINT_MAX + 1l);
    EXPECT_THROW_MSG(Format(format) << 0,
        FormatError, "number is too big in format");
  } else {
    std::sprintf(format, "{0:.%u0", UINT_MAX);
    EXPECT_THROW_MSG(Format(format), FormatError, "unmatched '{' in format");
    std::sprintf(format, "{0:.%u0}", UINT_MAX);
    EXPECT_THROW_MSG(Format(format) << 0,
        FormatError, "number is too big in format");
  }

  std::sprintf(format, "{0:.%u", INT_MAX + 1u);
  EXPECT_THROW_MSG(Format(format), FormatError, "unmatched '{' in format");
  std::sprintf(format, "{0:.%u}", INT_MAX + 1u);
  EXPECT_THROW_MSG(Format(format) << 0,
      FormatError, "number is too big in format");

  EXPECT_THROW_MSG(Format("{0:.") << 0,
      FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(Format("{0:.}") << 0,
      FormatError, "missing precision in format");
  EXPECT_THROW_MSG(Format("{0:.2") << 0,
      FormatError, "unmatched '{' in format");

  EXPECT_THROW_MSG(Format("{0:.2}") << 42,
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(Format("{0:.2f}") << 42,
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(Format("{0:.2}") << 42u,
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(Format("{0:.2f}") << 42u,
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(Format("{0:.2}") << 42l,
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(Format("{0:.2f}") << 42l,
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(Format("{0:.2}") << 42ul,
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(Format("{0:.2f}") << 42ul,
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_EQ("1.2", str(Format("{0:.2}") << 1.2345));
  EXPECT_EQ("1.2", str(Format("{0:.2}") << 1.2345l));

  EXPECT_THROW_MSG(Format("{0:.2}") << reinterpret_cast<void*>(0xcafe),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(Format("{0:.2f}") << reinterpret_cast<void*>(0xcafe),
      FormatError, "precision specifier requires floating-point argument");

  EXPECT_THROW_MSG(Format("{0:.2}") << 'x',
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(Format("{0:.2f}") << 'x',
      FormatError, "precision specifier requires floating-point argument");

  EXPECT_THROW_MSG(Format("{0:.2}") << "str",
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(Format("{0:.2f}") << "str",
      FormatError, "precision specifier requires floating-point argument");

  EXPECT_THROW_MSG(Format("{0:.2}") << TestString(),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(Format("{0:.2f}") << TestString(),
      FormatError, "precision specifier requires floating-point argument");
}

template <typename T>
void CheckUnknownTypes(
    const T &value, const char *types, const char *type_name) {
  char format[256], message[256];
  const char *special = ".0123456789}";
  for (int c = CHAR_MIN; c <= CHAR_MAX; ++c) {
    if (std::strchr(types, c) || std::strchr(special, c) || !c) continue;
    sprintf(format, "{0:1%c}", c, type_name);
    if (std::isprint(c))
      sprintf(message, "unknown format code '%c' for %s", c, type_name);
    else
      sprintf(message, "unknown format code '\\x%02x' for %s", c, type_name);
    EXPECT_THROW_MSG(Format(format) << value, FormatError, message)
      << format << " " << message;
  }
}

TEST(FormatterTest, FormatInt) {
  EXPECT_THROW_MSG(Format("{0:v") << 42,
      FormatError, "unmatched '{' in format");
  CheckUnknownTypes(42, "doxX", "integer");
}

TEST(FormatterTest, FormatDec) {
  EXPECT_EQ("0", str(Format("{0}") << 0));
  EXPECT_EQ("42", str(Format("{0}") << 42));
  EXPECT_EQ("42", str(Format("{0:d}") << 42));
  EXPECT_EQ("42", str(Format("{0}") << 42u));
  EXPECT_EQ("-42", str(Format("{0}") << -42));
  EXPECT_EQ("12345", str(Format("{0}") << 12345));
  EXPECT_EQ("67890", str(Format("{0}") << 67890));
  char buffer[256];
  sprintf(buffer, "%d", INT_MIN);
  EXPECT_EQ(buffer, str(Format("{0}") << INT_MIN));
  sprintf(buffer, "%d", INT_MAX);
  EXPECT_EQ(buffer, str(Format("{0}") << INT_MAX));
  sprintf(buffer, "%u", UINT_MAX);
  EXPECT_EQ(buffer, str(Format("{0}") << UINT_MAX));
  sprintf(buffer, "%ld", 0ul - LONG_MIN);
  EXPECT_EQ(buffer, str(Format("{0}") << LONG_MIN));
  sprintf(buffer, "%ld", LONG_MAX);
  EXPECT_EQ(buffer, str(Format("{0}") << LONG_MAX));
  sprintf(buffer, "%lu", ULONG_MAX);
  EXPECT_EQ(buffer, str(Format("{0}") << ULONG_MAX));
}

TEST(FormatterTest, FormatHex) {
  EXPECT_EQ("0", str(Format("{0:x}") << 0));
  EXPECT_EQ("42", str(Format("{0:x}") << 0x42));
  EXPECT_EQ("42", str(Format("{0:x}") << 0x42u));
  EXPECT_EQ("-42", str(Format("{0:x}") << -0x42));
  EXPECT_EQ("12345678", str(Format("{0:x}") << 0x12345678));
  EXPECT_EQ("90abcdef", str(Format("{0:x}") << 0x90abcdef));
  EXPECT_EQ("12345678", str(Format("{0:X}") << 0x12345678));
  EXPECT_EQ("90ABCDEF", str(Format("{0:X}") << 0x90ABCDEF));
  char buffer[256];
  sprintf(buffer, "-%x", 0u - INT_MIN);
  EXPECT_EQ(buffer, str(Format("{0:x}") << INT_MIN));
  sprintf(buffer, "%x", INT_MAX);
  EXPECT_EQ(buffer, str(Format("{0:x}") << INT_MAX));
  sprintf(buffer, "%x", UINT_MAX);
  EXPECT_EQ(buffer, str(Format("{0:x}") << UINT_MAX));
  sprintf(buffer, "-%lx", 0ul - LONG_MIN);
  EXPECT_EQ(buffer, str(Format("{0:x}") << LONG_MIN));
  sprintf(buffer, "%lx", LONG_MAX);
  EXPECT_EQ(buffer, str(Format("{0:x}") << LONG_MAX));
  sprintf(buffer, "%lx", ULONG_MAX);
  EXPECT_EQ(buffer, str(Format("{0:x}") << ULONG_MAX));
}

TEST(FormatterTest, FormatOct) {
  EXPECT_EQ("0", str(Format("{0:o}") << 0));
  EXPECT_EQ("42", str(Format("{0:o}") << 042));
  EXPECT_EQ("42", str(Format("{0:o}") << 042u));
  EXPECT_EQ("-42", str(Format("{0:o}") << -042));
  EXPECT_EQ("12345670", str(Format("{0:o}") << 012345670));
  char buffer[256];
  sprintf(buffer, "-%o", 0u - INT_MIN);
  EXPECT_EQ(buffer, str(Format("{0:o}") << INT_MIN));
  sprintf(buffer, "%o", INT_MAX);
  EXPECT_EQ(buffer, str(Format("{0:o}") << INT_MAX));
  sprintf(buffer, "%o", UINT_MAX);
  EXPECT_EQ(buffer, str(Format("{0:o}") << UINT_MAX));
  sprintf(buffer, "-%lo", 0ul - LONG_MIN);
  EXPECT_EQ(buffer, str(Format("{0:o}") << LONG_MIN));
  sprintf(buffer, "%lo", LONG_MAX);
  EXPECT_EQ(buffer, str(Format("{0:o}") << LONG_MAX));
  sprintf(buffer, "%lo", ULONG_MAX);
  EXPECT_EQ(buffer, str(Format("{0:o}") << ULONG_MAX));
}

TEST(FormatterTest, FormatDouble) {
  CheckUnknownTypes(1.2, "eEfFgG", "double");
  EXPECT_EQ("0", str(Format("{0:}") << 0.0));
  EXPECT_EQ("0.000000", str(Format("{0:f}") << 0.0));
  EXPECT_EQ("392.65", str(Format("{0:}") << 392.65));
  EXPECT_EQ("392.65", str(Format("{0:g}") << 392.65));
  EXPECT_EQ("392.65", str(Format("{0:G}") << 392.65));
  EXPECT_EQ("392.650000", str(Format("{0:f}") << 392.65));
  EXPECT_EQ("392.650000", str(Format("{0:F}") << 392.65));
  EXPECT_EQ("3.926500e+02", str(Format("{0:e}") << 392.65));
  EXPECT_EQ("3.926500E+02", str(Format("{0:E}") << 392.65));
  EXPECT_EQ("+0000392.6", str(Format("{0:+010.4g}") << 392.65));
}

TEST(FormatterTest, FormatLongDouble) {
  EXPECT_EQ("0", str(Format("{0:}") << 0.0l));
  EXPECT_EQ("0.000000", str(Format("{0:f}") << 0.0l));
  EXPECT_EQ("392.65", str(Format("{0:}") << 392.65l));
  EXPECT_EQ("392.65", str(Format("{0:g}") << 392.65l));
  EXPECT_EQ("392.65", str(Format("{0:G}") << 392.65l));
  EXPECT_EQ("392.650000", str(Format("{0:f}") << 392.65l));
  EXPECT_EQ("392.650000", str(Format("{0:F}") << 392.65l));
  EXPECT_EQ("3.926500e+02", str(Format("{0:e}") << 392.65l));
  EXPECT_EQ("3.926500E+02", str(Format("{0:E}") << 392.65l));
  EXPECT_EQ("+0000392.6", str(Format("{0:+010.4g}") << 392.65l));
}

TEST(FormatterTest, FormatChar) {
  CheckUnknownTypes('a', "c", "char");
  EXPECT_EQ("a", str(Format("{0}") << 'a'));
  EXPECT_EQ("z", str(Format("{0:c}") << 'z'));
}

TEST(FormatterTest, FormatCString) {
  CheckUnknownTypes("test", "s", "string");
  EXPECT_EQ("test", str(Format("{0}") << "test"));
  EXPECT_EQ("test", str(Format("{0:s}") << "test"));
}

TEST(FormatterTest, FormatPointer) {
  CheckUnknownTypes(reinterpret_cast<void*>(0x1234), "p", "pointer");
  EXPECT_EQ("0x0", str(Format("{0}") << reinterpret_cast<void*>(0)));
  EXPECT_EQ("0x1234", str(Format("{0}") << reinterpret_cast<void*>(0x1234)));
  EXPECT_EQ("0x1234", str(Format("{0:p}") << reinterpret_cast<void*>(0x1234)));
}

TEST(FormatterTest, FormatString) {
  EXPECT_EQ("test", str(Format("{0}") << std::string("test")));
}

class Date {
  int year_, month_, day_;
 public:
  Date(int year, int month, int day) : year_(year), month_(month), day_(day) {}

  friend std::ostream &operator<<(std::ostream &os, const Date &d) {
    os << d.year_ << '-' << d.month_ << '-' << d.day_;
    return os;
  }
};

TEST(FormatterTest, FormatCustom) {
  EXPECT_EQ("a string", str(Format("{0}") << TestString("a string")));
  std::string s = str(fmt::Format("The date is {0}") << Date(2012, 12, 9));
  EXPECT_EQ("The date is 2012-12-9", s);
  Date date(2012, 12, 9);
  CheckUnknownTypes(date, "", "object");
}

TEST(FormatterTest, FormatStringFromSpeedTest) {
  EXPECT_EQ("1.2340000000:0042:+3.13:str:0x3e8:X:%",
      str(Format("{0:0.10f}:{1:04}:{2:+g}:{3}:{4}:{5}:%")
          << 1.234 << 42 << 3.13 << "str"
          << reinterpret_cast<void*>(1000) << 'X'));
}

TEST(FormatterTest, FormatterCtor) {
  Formatter format;
  EXPECT_EQ(0, format.size());
  EXPECT_STREQ("", format.data());
  EXPECT_STREQ("", format.c_str());
  EXPECT_EQ("", format.str());
  format("part{0}") << 1;
  format("part{0}") << 2;
  EXPECT_EQ("part1part2", format.str());
}

TEST(FormatterTest, FormatterAppend) {
  Formatter format;
  format("part{0}") << 1;
  EXPECT_EQ(strlen("part1"), format.size());
  EXPECT_STREQ("part1", format.c_str());
  EXPECT_STREQ("part1", format.data());
  EXPECT_EQ("part1", format.str());
  format("part{0}") << 2;
  EXPECT_EQ(strlen("part1part2"), format.size());
  EXPECT_STREQ("part1part2", format.c_str());
  EXPECT_STREQ("part1part2", format.data());
  EXPECT_EQ("part1part2", format.str());
}

TEST(FormatterTest, FormatterExample) {
  Formatter format;
  format("Current point:\n");
  format("({0:+f}, {1:+f})\n") << -3.14 << 3.14;
  EXPECT_EQ("Current point:\n(-3.140000, +3.140000)\n", format.str());
}

TEST(FormatterTest, ArgInserter) {
  Formatter format;
  EXPECT_EQ("1", str(format("{0}") << 1));
  EXPECT_STREQ("12", c_str(format("{0}") << 2));
}

struct CallCheck {
  bool &called;

  CallCheck(bool &called) : called(called) {}

  void operator()(const Formatter &) const {
    called = true;
  }
};

TEST(ActiveFormatterTest, Action) {
  bool called = false;
  {
    fmt::ActiveFormatter<CallCheck> af("test", CallCheck(called));
    EXPECT_FALSE(called);
  }
  EXPECT_TRUE(called);
}

TEST(ActiveFormatterTest, ArgLifetime) {
  // The following code is for testing purposes only. It is a definite abuse
  // of the API and shouldn't be used in real applications.
  const fmt::ActiveFormatter<fmt::Ignore> &af = fmt::Format("{0}");
  const_cast<fmt::ActiveFormatter<fmt::Ignore>&>(af) << std::string("test");
  // String object passed as an argument to ActiveFormatter has
  // been destroyed, but ArgInserter dtor hasn't been called yet.
  // But that's OK since the Arg's dtor takes care of this and
  // calls Format.
  EXPECT_EQ("test", str(af));
}

struct PrintError {
  void operator()(const fmt::Formatter &f) const {
    //std::cerr << "Error: " << f.str() << std::endl;
  }
};

fmt::ActiveFormatter<PrintError> ReportError(const char *format) {
  fmt::ActiveFormatter<PrintError> af(format);
  return af;
}

TEST(ActiveFormatterTest, Example) {
  std::string path = "somefile";
  ReportError("File not found: {0}") << path;
}