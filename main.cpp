
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

static std::vector<uint16_t> utf8_to_utf6_lenient(const uint8_t *in_buf, size_t in_buf_len, const uint8_t** first_invalid_char) {

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

static void assert(bool check, const std::string &label) {
  if (!check) {
    std::cerr << "Assertion failed: " << label << std::endl;
    std::exit(1);
  }
}

int main() {
  {
    auto res = utf8_to_utf6_lenient(hello_bg_utf8.data(), hello_bg_utf8.size(), nullptr);
    assert(res == hello_bg_utf16, "hello_bg_utf8 1");
  }
  {
    const uint8_t val = 42;
    const uint8_t *ptr = &val;
    auto res = utf8_to_utf6_lenient(hello_bg_utf8.data(), hello_bg_utf8.size(), &ptr);
    assert(res == hello_bg_utf16, "hello_bg_utf8 2 1");
    assert(ptr == nullptr, "hello_bg_utf8 2 2");
  }
  {
    const uint8_t *ptr = nullptr;
    auto res = utf8_to_utf6_lenient(invalid_utf8_continuation.data(), invalid_utf8_continuation.size(), &ptr);
    assert(ptr != nullptr, "invalid_utf8_continuation 1");
    assert(ptr - invalid_utf8_continuation.data() == 1, "invalid_utf8_continuation 2");
  }
  {
    const uint8_t *ptr = nullptr;
    auto res = utf8_to_utf6_lenient(incomplete_utf8_sequence.data(), incomplete_utf8_sequence.size(), &ptr);
    assert(ptr != nullptr, "incomplete_utf8_sequence 1");
    assert(ptr - incomplete_utf8_sequence.data() == 0, "incomplete_utf8_sequence 2");
  }
  {
    const uint8_t *ptr = nullptr;
    auto res = utf8_to_utf6_lenient(invalid_overlong_utf8.data(), invalid_overlong_utf8.size(), &ptr);
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
    auto res = utf8_to_utf6_lenient(in_buf.data(), in_buf.size(), &ptr);
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
    auto res = utf8_to_utf6_lenient(in_buf.data(), in_buf.size(), &ptr);
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
    auto res = utf8_to_utf6_lenient(in_buf.data(), in_buf.size(), &ptr);
    assert(ptr != nullptr, "hello invalid_overlong_utf8 1");
    assert((ptr - in_buf.data()) == 18, "hello invalid_overlong_utf8 2");
    assert(std::equal(hello_bg_utf16.begin(), hello_bg_utf16.end(), res.begin()), "hello invalid_overlong_utf8 3");
    assert(std::equal(hello_bg_utf16.begin(), hello_bg_utf16.end(), res.begin() + 10), "hello invalid_overlong_utf8 4");
    assert(res[hello_bg_utf16.size()] == invalid_char_replacement, "hello invalid_overlong_utf8 5");
    /*
    for (size_t start = 0; start < res.size(); start++) {
      if (std::equal(hello_bg_utf16.begin(), hello_bg_utf16.end(), res.begin() + start)) {
        std::cout << start << std::endl;
      }
    }
    */
  }
}