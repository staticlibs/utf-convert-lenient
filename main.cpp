
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

#define UTF_CPP_CPLUSPLUS 199711L
#include "utf8.h"

const uint32_t invalid_char_replacement = 0xfffd;
const std::vector<uint8_t> hello_bg_utf8 = { 0xd0, 0x97, 0xd0, 0xb4, 0xd1, 0x80, 0xd0, 0xb0, 0xd0, 0xb2, 0xd0, 0xb5, 0xd0, 0xb9, 0xd1, 0x82, 0xd0, 0xb5 };
const std::vector<uint8_t> invalid_utf8_continuation = { 0x48, 0x80, 0x65 };
const std::vector<uint8_t> incomplete_utf8_sequence = { 0xe0, 0xa0 };
const std::vector<uint8_t> invalid_overlong_utf8 = { 0xf4, 0x90, 0x80, 0x80 };
const std::vector<uint16_t> hello_bg_utf16 = { 0x0417, 0x0434, 0x0440, 0x0430, 0x0432, 0x0435, 0x0439, 0x0442, 0x0435 };
const std::vector<uint16_t> invalid_utf16_surrogate = { 0xdc00, 0xd800 };
const std::vector<uint16_t> incomplete_utf16_surrogate = { 0xd800 };
const std::vector<uint16_t> valid_utf16_surrogate = { 0xd800, 0xdc00 };

static std::vector<uint16_t> utf8_to_utf16_lenient(const uint8_t *in_buf, size_t in_buf_len, const uint8_t** first_invalid_char) {
  std::vector<uint16_t> res;
  std::back_insert_iterator res_bi = std::back_inserter(res);

  const uint8_t *in_buf_end = in_buf + in_buf_len;
  const uint8_t *fic = utf8::find_invalid(in_buf, in_buf_end);

  if (fic == in_buf_end) {
    utf8::unchecked::utf8to16(in_buf, in_buf_end, res_bi);
    if (nullptr != first_invalid_char) {
      *first_invalid_char = nullptr;
    }
  } else {
    std::vector<uint8_t> replaced;
    std::back_insert_iterator replaced_bi = std::back_inserter(replaced);
    utf8::replace_invalid(in_buf, in_buf_end, replaced_bi, invalid_char_replacement);
    utf8::unchecked::utf8to16(replaced.begin(), replaced.end(), res_bi);
    if (nullptr != first_invalid_char) {
      *first_invalid_char = fic;
    }
  }

  return res;
}

template <typename u16bit_iterator>
u16bit_iterator utf16_find_invalid(u16bit_iterator start, u16bit_iterator end) {
  while (start != end) {
    utf8::utfchar32_t cp = utf8::internal::mask16(*start++);
    if (utf8::internal::is_lead_surrogate(cp)) { // Take care of surrogate pairs first
      if (start != end) {
        const utf8::utfchar32_t trail_surrogate = utf8::internal::mask16(*start++);
        if (!utf8::internal::is_trail_surrogate(trail_surrogate)) {
          return start - 1;
        }
      } else {
        return start - 1;
      }
    } else if (utf8::internal::is_trail_surrogate(cp)) { // Lone trail surrogate
      return start;
    }
  }
  return end;
}

template <typename u16bit_iterator, typename output_iterator>
output_iterator utf16_replace_invalid(u16bit_iterator start, u16bit_iterator end, output_iterator out, utf8::utfchar32_t replacement) {
  while (start != end) {
    uint16_t char1 = *start++;
    utf8::utfchar32_t cp = utf8::internal::mask16(char1);
    if (utf8::internal::is_lead_surrogate(cp)) { // Take care of surrogate pairs first
      if (start != end) {
        uint16_t char2 = *start++;
        const utf8::utfchar32_t trail_surrogate = utf8::internal::mask16(char2);
        if (utf8::internal::is_trail_surrogate(trail_surrogate)) {
          *out++ = char1;
          *out++ = char2;
        } else {
          *out++ = replacement;
          *out++ = char2;
        }
      } else {
        *out++ = replacement;
      }
    } else if (utf8::internal::is_trail_surrogate(cp)) { // Lone trail surrogate
      *out++ = replacement;
    } else {
      *out++ = char1;
    }
  }
  return out;
}

static std::vector<uint8_t> utf16_to_utf8_lenient(const uint16_t *in_buf, size_t in_buf_len, const uint16_t** first_invalid_char) {
  std::vector<uint8_t> res;
  std::back_insert_iterator res_bi = std::back_inserter(res);

  const uint16_t *in_buf_end = in_buf + in_buf_len;
  const uint16_t *fic = utf16_find_invalid(in_buf, in_buf_end);

  if (fic == in_buf_end) {
    utf8::unchecked::utf16to8(in_buf, in_buf_end, res_bi);
    if (nullptr != first_invalid_char) {
      *first_invalid_char = nullptr;
    }
  } else {
    std::vector<uint16_t> replaced;
    std::back_insert_iterator replaced_bi = std::back_inserter(replaced);
    utf16_replace_invalid(in_buf, in_buf_end, replaced_bi, invalid_char_replacement);
    utf8::unchecked::utf16to8(replaced.begin(), replaced.end(), res_bi);
    if (nullptr != first_invalid_char) {
      *first_invalid_char = fic;
    }
  }

  return res;
}

static void assert(bool check, const std::string &label) {
  if (!check) {
    std::cerr << "Assertion failed: " << label << std::endl;
    std::exit(1);
  }
}

static void test_utf8_to_utf16() {
  {
    auto res = utf8_to_utf16_lenient(hello_bg_utf8.data(), hello_bg_utf8.size(), nullptr);
    assert(res == hello_bg_utf16, "hello_bg_utf8 1");
  }
  {
    const uint8_t val = 42;
    const uint8_t *ptr = &val;
    auto res = utf8_to_utf16_lenient(hello_bg_utf8.data(), hello_bg_utf8.size(), &ptr);
    assert(res == hello_bg_utf16, "hello_bg_utf8 2 1");
    assert(ptr == nullptr, "hello_bg_utf8 2 2");
  }
  {
    const uint8_t *ptr = nullptr;
    auto res = utf8_to_utf16_lenient(invalid_utf8_continuation.data(), invalid_utf8_continuation.size(), &ptr);
    assert(ptr != nullptr, "invalid_utf8_continuation 1");
    assert(ptr - invalid_utf8_continuation.data() == 1, "invalid_utf8_continuation 2");
  }
  {
    const uint8_t *ptr = nullptr;
    auto res = utf8_to_utf16_lenient(incomplete_utf8_sequence.data(), incomplete_utf8_sequence.size(), &ptr);
    assert(ptr != nullptr, "incomplete_utf8_sequence 1");
    assert(ptr - incomplete_utf8_sequence.data() == 0, "incomplete_utf8_sequence 2");
  }
  {
    const uint8_t *ptr = nullptr;
    auto res = utf8_to_utf16_lenient(invalid_overlong_utf8.data(), invalid_overlong_utf8.size(), &ptr);
    assert(ptr != nullptr, "invalid_overlong_utf8 1");
    assert(ptr - invalid_overlong_utf8.data() == 0, "invalid_overlong_utf8 2");
  }
  {
    const uint8_t *ptr = nullptr;
    std::vector<uint8_t> in_buf;
    std::back_insert_iterator in_buf_bi = std::back_inserter(in_buf);
    std::copy(hello_bg_utf8.begin(), hello_bg_utf8.end(), in_buf_bi);
    std::copy(invalid_utf8_continuation.begin(), invalid_utf8_continuation.end(), in_buf_bi);
    std::copy(hello_bg_utf8.begin(), hello_bg_utf8.end(), in_buf_bi);
    auto res = utf8_to_utf16_lenient(in_buf.data(), in_buf.size(), &ptr);
    assert(ptr != nullptr, "hello invalid_utf8_continuation 1");
    assert((ptr - in_buf.data()) == 19, "hello invalid_utf8_continuation 2");
    assert(std::equal(hello_bg_utf16.begin(), hello_bg_utf16.end(), res.begin()), "hello invalid_utf8_continuation 3");
    assert(std::equal(hello_bg_utf16.begin(), hello_bg_utf16.end(), res.begin() + 12), "hello invalid_utf8_continuation 4");
    assert(res[hello_bg_utf16.size()] == 0x48, "hello invalid_utf8_continuation 5");
    assert(res[hello_bg_utf16.size() + 1] == invalid_char_replacement, "hello invalid_utf8_continuation 6");
    assert(res[hello_bg_utf16.size() + 2] == 0x65, "hello invalid_utf8_continuation 7");
  }
  {
    const uint8_t *ptr = nullptr;
    std::vector<uint8_t> in_buf;
    std::back_insert_iterator in_buf_bi = std::back_inserter(in_buf);
    std::copy(hello_bg_utf8.begin(), hello_bg_utf8.end(), in_buf_bi);
    std::copy(incomplete_utf8_sequence.begin(), incomplete_utf8_sequence.end(), in_buf_bi);
    std::copy(hello_bg_utf8.begin(), hello_bg_utf8.end(), in_buf_bi);
    auto res = utf8_to_utf16_lenient(in_buf.data(), in_buf.size(), &ptr);
    assert(ptr != nullptr, "hello incomplete_utf8_sequence 1");
    assert((ptr - in_buf.data()) == 18, "hello incomplete_utf8_sequence 2");
    assert(std::equal(hello_bg_utf16.begin(), hello_bg_utf16.end(), res.begin()), "hello incomplete_utf8_sequence 3");
    assert(std::equal(hello_bg_utf16.begin(), hello_bg_utf16.end(), res.begin() + 10), "hello incomplete_utf8_sequence 4");
    assert(res[hello_bg_utf16.size()] == invalid_char_replacement, "hello incomplete_utf8_sequence 5");
  }
  {
    const uint8_t *ptr = nullptr;
    std::vector<uint8_t> in_buf;
    std::back_insert_iterator in_buf_bi = std::back_inserter(in_buf);
    std::copy(hello_bg_utf8.begin(), hello_bg_utf8.end(), in_buf_bi);
    std::copy(invalid_overlong_utf8.begin(), invalid_overlong_utf8.end(), in_buf_bi);
    std::copy(hello_bg_utf8.begin(), hello_bg_utf8.end(), in_buf_bi);
    auto res = utf8_to_utf16_lenient(in_buf.data(), in_buf.size(), &ptr);
    assert(ptr != nullptr, "hello invalid_overlong_utf8 1");
    assert((ptr - in_buf.data()) == 18, "hello invalid_overlong_utf8 2");
    assert(std::equal(hello_bg_utf16.begin(), hello_bg_utf16.end(), res.begin()), "hello invalid_overlong_utf8 3");
    assert(std::equal(hello_bg_utf16.begin(), hello_bg_utf16.end(), res.begin() + 10), "hello invalid_overlong_utf8 4");
    assert(res[hello_bg_utf16.size()] == invalid_char_replacement, "hello invalid_overlong_utf8 5");
  }
}

void test_utf16_find_invalid() {
  {
    auto res = utf16_find_invalid(invalid_utf16_surrogate.begin(), invalid_utf16_surrogate.end()); 
    assert(res != invalid_utf16_surrogate.end(), "find invalid_utf16_surrogate 1");
  }
  {
    auto res = utf16_find_invalid(incomplete_utf16_surrogate.begin(), incomplete_utf16_surrogate.end()); 
    assert(res != incomplete_utf16_surrogate.end(), "find incomplete_utf16_surrogate 1");
  }
  {
    auto res = utf16_find_invalid(valid_utf16_surrogate.begin(), valid_utf16_surrogate.end());
    assert(res == valid_utf16_surrogate.end(), "find valid_utf16_surrogate 1");
  }
  {
    std::vector<uint16_t> in_buf;
    std::back_insert_iterator in_buf_bi = std::back_inserter(in_buf);
    std::copy(hello_bg_utf16.begin(), hello_bg_utf16.end(), in_buf_bi);
    std::copy(invalid_utf16_surrogate.begin(), invalid_utf16_surrogate.end(), in_buf_bi);
    std::copy(hello_bg_utf16.begin(), hello_bg_utf16.end(), in_buf_bi);
    auto res = utf16_find_invalid(in_buf.begin(), in_buf.end());
    assert((res - in_buf.begin()) == hello_bg_utf16.size() + 1, "find hello invalid_utf16_surrogate 1");
  }
  {
    std::vector<uint16_t> in_buf;
    std::back_insert_iterator in_buf_bi = std::back_inserter(in_buf);
    std::copy(hello_bg_utf16.begin(), hello_bg_utf16.end(), in_buf_bi);
    std::copy(incomplete_utf16_surrogate.begin(), incomplete_utf16_surrogate.end(), in_buf_bi);
    std::copy(hello_bg_utf16.begin(), hello_bg_utf16.end(), in_buf_bi);
    auto res = utf16_find_invalid(in_buf.begin(), in_buf.end());
    assert((res - in_buf.begin()) == hello_bg_utf16.size() + 1, "find hello incomplete_utf16_surrogate 1");
  }
  {
    std::vector<uint16_t> in_buf;
    std::back_insert_iterator in_buf_bi = std::back_inserter(in_buf);
    std::copy(hello_bg_utf16.begin(), hello_bg_utf16.end(), in_buf_bi);
    std::copy(valid_utf16_surrogate.begin(), valid_utf16_surrogate.end(), in_buf_bi);
    std::copy(hello_bg_utf16.begin(), hello_bg_utf16.end(), in_buf_bi);
    auto res = utf16_find_invalid(in_buf.begin(), in_buf.end());
    assert(res == in_buf.end(), "find hello valid_utf16_surrogate 1");
  }
}

static void test_utf16_replace_invalid() {
  {
    std::vector<uint16_t> replaced;
    std::back_insert_iterator replaced_bi = std::back_inserter(replaced);
    utf16_replace_invalid(invalid_utf16_surrogate.begin(), invalid_utf16_surrogate.end(), replaced_bi, invalid_char_replacement);
    assert(replaced.size() == 2, "replace invalid_utf16_surrogate 1");
    assert(replaced[0] == invalid_char_replacement, "replace invalid_utf16_surrogate 2");
    assert(replaced[1] == invalid_char_replacement, "replace invalid_utf16_surrogate 3");
  }
  {
    std::vector<uint16_t> replaced;
    std::back_insert_iterator replaced_bi = std::back_inserter(replaced);
    utf16_replace_invalid(incomplete_utf16_surrogate.begin(), incomplete_utf16_surrogate.end(), replaced_bi, invalid_char_replacement);
    assert(replaced.size() == 1, "replace incomplete_utf16_surrogate 1");
    assert(replaced[0] == invalid_char_replacement, "replace incomplete_utf16_surrogate 2");
  }
  {
    std::vector<uint16_t> replaced;
    std::back_insert_iterator replaced_bi = std::back_inserter(replaced);
    utf16_replace_invalid(valid_utf16_surrogate.begin(), valid_utf16_surrogate.end(), replaced_bi, invalid_char_replacement);
    assert(replaced == valid_utf16_surrogate, "replace valid_utf16_surrogate 1");
  }
  {
    std::vector<uint16_t> in_buf;
    std::back_insert_iterator in_buf_bi = std::back_inserter(in_buf);
    std::copy(hello_bg_utf16.begin(), hello_bg_utf16.end(), in_buf_bi);
    std::copy(invalid_utf16_surrogate.begin(), invalid_utf16_surrogate.end(), in_buf_bi);
    std::copy(hello_bg_utf16.begin(), hello_bg_utf16.end(), in_buf_bi);
    std::vector<uint16_t> replaced;
    std::back_insert_iterator replaced_bi = std::back_inserter(replaced);
    utf16_replace_invalid(in_buf.begin(), in_buf.end(), replaced_bi, invalid_char_replacement);
    assert(std::equal(hello_bg_utf16.begin(), hello_bg_utf16.end(), replaced.begin()), "replace hello invalid_utf16_surrogate 1");
    assert(std::equal(hello_bg_utf16.begin(), hello_bg_utf16.end(), replaced.begin() + 11), "replace hello invalid_utf16_surrogate 2");
    assert(replaced[9] == invalid_char_replacement, "replace hello invalid_utf16_surrogate 3");
    assert(replaced[10] == invalid_char_replacement, "replace hello invalid_utf16_surrogate 4");
  }
  {
    std::vector<uint16_t> in_buf;
    std::back_insert_iterator in_buf_bi = std::back_inserter(in_buf);
    std::copy(hello_bg_utf16.begin(), hello_bg_utf16.end(), in_buf_bi);
    std::copy(incomplete_utf16_surrogate.begin(), incomplete_utf16_surrogate.end(), in_buf_bi);
    std::copy(hello_bg_utf16.begin(), hello_bg_utf16.end(), in_buf_bi);
    std::vector<uint16_t> replaced;
    std::back_insert_iterator replaced_bi = std::back_inserter(replaced);
    utf16_replace_invalid(in_buf.begin(), in_buf.end(), replaced_bi, invalid_char_replacement);
    assert(std::equal(hello_bg_utf16.begin(), hello_bg_utf16.end(), replaced.begin()), "replace hello incomplete_utf16_surrogate 1");
    assert(std::equal(hello_bg_utf16.begin(), hello_bg_utf16.end(), replaced.begin() + 10), "replace hello incomplete_utf16_surrogate 2");
    assert(replaced[9] == invalid_char_replacement, "replace hello incomplete_utf16_surrogate 3");
  }
  {
    std::vector<uint16_t> in_buf;
    std::back_insert_iterator in_buf_bi = std::back_inserter(in_buf);
    std::copy(hello_bg_utf16.begin(), hello_bg_utf16.end(), in_buf_bi);
    std::copy(valid_utf16_surrogate.begin(), valid_utf16_surrogate.end(), in_buf_bi);
    std::copy(hello_bg_utf16.begin(), hello_bg_utf16.end(), in_buf_bi);
    std::vector<uint16_t> replaced;
    std::back_insert_iterator replaced_bi = std::back_inserter(replaced);
    utf16_replace_invalid(in_buf.begin(), in_buf.end(), replaced_bi, invalid_char_replacement);
    assert(replaced == in_buf, "replace hello valid_utf16_surrogate 1");
  }
}

static void test_utf16_to_utf8() {
  {
    auto res = utf16_to_utf8_lenient(hello_bg_utf16.data(), hello_bg_utf16.size(), nullptr);
    assert(res == hello_bg_utf8, "hello_bg_utf16 1");
  }
  {
    const uint16_t val = 42;
    const uint16_t *ptr = &val;
    auto res = utf16_to_utf8_lenient(hello_bg_utf16.data(), hello_bg_utf16.size(), &ptr);
    assert(res == hello_bg_utf8, "hello_bg_utf16 2 1");
    assert(ptr == nullptr, "hello_bg_utf16 2 2");
  }
  {
    const uint16_t *ptr = nullptr;
    auto res = utf16_to_utf8_lenient(invalid_utf16_surrogate.data(), invalid_utf16_surrogate.size(), &ptr);
    assert(ptr != nullptr, "invalid_utf16_surrogate 1");
    assert(ptr - invalid_utf16_surrogate.data() == 1, "invalid_utf16_surrogate 2");
  }
  {
    const uint16_t *ptr = nullptr;
    auto res = utf16_to_utf8_lenient(incomplete_utf16_surrogate.data(), incomplete_utf16_surrogate.size(), &ptr);
    assert(ptr != nullptr, "incomplete_utf16_surrogate 1");
    assert(ptr - incomplete_utf16_surrogate.data() == 0, "incomplete_utf16_surrogate 2");
  }
  {
    const uint16_t *ptr = nullptr;
    auto res = utf16_to_utf8_lenient(valid_utf16_surrogate.data(), valid_utf16_surrogate.size(), &ptr);
    assert(ptr == nullptr, "valid_utf16_surrogate 1");
  }
  {
    const uint16_t *ptr = nullptr;
    std::vector<uint16_t> in_buf;
    std::back_insert_iterator in_buf_bi = std::back_inserter(in_buf);
    std::copy(hello_bg_utf16.begin(), hello_bg_utf16.end(), in_buf_bi);
    std::copy(invalid_utf16_surrogate.begin(), invalid_utf16_surrogate.end(), in_buf_bi);
    std::copy(hello_bg_utf16.begin(), hello_bg_utf16.end(), in_buf_bi);
    auto res = utf16_to_utf8_lenient(in_buf.data(), in_buf.size(), &ptr);
    assert(ptr != nullptr, "hello invalid_utf16_surrogate 1");
    assert((ptr - in_buf.data()) == 10, "hello invalid_utf16_surrogate 2");
    assert(std::equal(hello_bg_utf8.begin(), hello_bg_utf8.end(), res.begin()), "hello invalid_utf16_surrogate 3");
    assert(std::equal(hello_bg_utf8.begin(), hello_bg_utf8.end(), res.begin() + 24), "hello invalid_utf16_surrogate 4");
    assert(res[hello_bg_utf8.size() + 0] == 0xef, "hello invalid_utf16_surrogate 5");
    assert(res[hello_bg_utf8.size() + 1] == 0xbf, "hello invalid_utf16_surrogate 6");
    assert(res[hello_bg_utf8.size() + 2] == 0xbd, "hello invalid_utf16_surrogate 7");
    assert(res[hello_bg_utf8.size() + 3] == 0xef, "hello invalid_utf16_surrogate 8");
    assert(res[hello_bg_utf8.size() + 4] == 0xbf, "hello invalid_utf16_surrogate 9");
    assert(res[hello_bg_utf8.size() + 5] == 0xbd, "hello invalid_utf16_surrogate 10");
  }
  {
    const uint16_t *ptr = nullptr;
    std::vector<uint16_t> in_buf;
    std::back_insert_iterator in_buf_bi = std::back_inserter(in_buf);
    std::copy(hello_bg_utf16.begin(), hello_bg_utf16.end(), in_buf_bi);
    std::copy(incomplete_utf16_surrogate.begin(), incomplete_utf16_surrogate.end(), in_buf_bi);
    std::copy(hello_bg_utf16.begin(), hello_bg_utf16.end(), in_buf_bi);
    auto res = utf16_to_utf8_lenient(in_buf.data(), in_buf.size(), &ptr);
    assert(ptr != nullptr, "hello incomplete_utf16_surrogate 1");
    assert((ptr - in_buf.data()) == 10, "hello incomplete_utf16_surrogate 2");
    assert(std::equal(hello_bg_utf8.begin(), hello_bg_utf8.end(), res.begin()), "hello incomplete_utf16_surrogate 3");
    assert(std::equal(hello_bg_utf8.begin(), hello_bg_utf8.end(), res.begin() + 21), "hello incomplete_utf16_surrogate 4");
    assert(res[hello_bg_utf8.size() + 0] == 0xef, "hello incomplete_utf16_surrogate 5");
    assert(res[hello_bg_utf8.size() + 1] == 0xbf, "hello incomplete_utf16_surrogate 6");
    assert(res[hello_bg_utf8.size() + 2] == 0xbd, "hello incomplete_utf16_surrogate 7");
  }
  {
    const uint16_t *ptr = nullptr;
    std::vector<uint16_t> in_buf;
    std::back_insert_iterator in_buf_bi = std::back_inserter(in_buf);
    std::copy(hello_bg_utf16.begin(), hello_bg_utf16.end(), in_buf_bi);
    std::copy(valid_utf16_surrogate.begin(), valid_utf16_surrogate.end(), in_buf_bi);
    std::copy(hello_bg_utf16.begin(), hello_bg_utf16.end(), in_buf_bi);
    auto res = utf16_to_utf8_lenient(in_buf.data(), in_buf.size(), &ptr);
    assert(ptr == nullptr, "hello valid_utf16_surrogate 1");
    assert(std::equal(hello_bg_utf8.begin(), hello_bg_utf8.end(), res.begin()), "hello valid_utf16_surrogate 3");
    assert(std::equal(hello_bg_utf8.begin(), hello_bg_utf8.end(), res.begin() + 22), "hello valid_utf16_surrogate 4");
    assert(res[hello_bg_utf8.size() + 0] == 0xf0, "hello valid_utf16_surrogate 5");
    assert(res[hello_bg_utf8.size() + 1] == 0x90, "hello valid_utf16_surrogate 6");
    assert(res[hello_bg_utf8.size() + 2] == 0x80, "hello valid_utf16_surrogate 7");
    assert(res[hello_bg_utf8.size() + 3] == 0x80, "hello valid_utf16_surrogate 8");
  }
}

int main() {
  test_utf8_to_utf16();
  test_utf16_find_invalid();
  test_utf16_replace_invalid();
  test_utf16_to_utf8();
}