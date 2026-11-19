#pragma once

#include <stddef.h>
#include <stdint.h>

namespace nice_gfx { namespace pinyin {

// The caller owns the four arrays. Legacy dictionaries store NUL-terminated
// ASCII spellings and UTF-8 candidates. Packed dictionaries keep the same
// descriptor layout and store five-bit spellings and 16-bit candidate words.
struct Dictionary {
  static constexpr uint16_t Packed = 0x8000;
  const uint8_t *spellings, *values;
  const uint16_t *spellingOffsets;
  const uint32_t *valueOffsets;
  uint16_t count;

  bool packed() const { return (count & Packed) != 0; }
  uint16_t size() const { return count & (Packed - 1); }
};

inline bool available(const Dictionary *dictionary) {
  return dictionary && dictionary->size() && dictionary->spellings && dictionary->values &&
         dictionary->spellingOffsets && dictionary->valueOffsets;
}

class Composition {
  char text_[25] = {};
  uint8_t length_ = 0;

public:
  static constexpr size_t capacity = sizeof(text_) - 1;

  static bool accepts(char letter) {
    return (letter >= 'a' && letter <= 'z') || (letter >= 'A' && letter <= 'Z');
  }

  bool push(char letter) {
    if (!accepts(letter) || length_ == capacity) return false;
    if (letter >= 'A' && letter <= 'Z') letter += 'a' - 'A';
    text_[length_++] = letter;
    text_[length_] = '\0';
    return true;
  }

  bool backspace() {
    if (empty()) return false;
    text_[--length_] = '\0';
    return true;
  }

  void clear() {
    length_ = 0;
    text_[0] = '\0';
  }

  void consume(size_t count) {
    if (count >= length_) {
      clear();
      return;
    }
    for (size_t i = 0; i <= length_ - count; ++i) text_[i] = text_[i + count];
    length_ -= count;
  }

  const char *c_str() const { return text_; }
  size_t size() const { return length_; }
  bool empty() const { return !length_; }
};

struct DirectReader {
  static uint8_t read(const uint8_t *p) { return *p; }
  static uint16_t read(const uint16_t *p) { return *p; }
  static uint32_t read(const uint32_t *p) { return *p; }
};

struct Candidate {
  const uint8_t *data;
  const uint8_t *end; // Non-null for packed dictionaries.

  explicit operator bool() const { return data != nullptr; }
};

struct Match {
  const uint8_t *values;
  size_t consumed;
  const uint8_t *end;

  Candidate candidates() const { return {values, end}; }
};

template<typename Reader>
uint8_t spellingLetter(const uint8_t *spellings, size_t offset, bool packed) {
  if (!packed) return Reader::read(spellings + offset);
  const size_t bit = offset * 5;
  const uint8_t shift = bit & 7;
  const uint8_t *byte = spellings + (bit >> 3);
  uint16_t word = Reader::read(byte);
  if (shift > 3) word |= uint16_t(Reader::read(byte + 1)) << 8;
  const auto code = uint8_t((word >> shift) & 31);
  return code ? uint8_t('a' - 1 + code) : 0;
}

template<typename Reader>
uint16_t packedValueOffset(const uint32_t *offsets, size_t index) {
  const uint32_t pair = Reader::read(offsets + index / 2);
  return uint16_t(pair >> ((index & 1) * 16));
}

// Prefer the full spelling. An available prefix lets the user convert a
// syllable or phrase and continue with the rest of the composition.
template<typename Reader = DirectReader>
Match find(const Dictionary &dictionary, const Composition &composition) {
  if (!available(&dictionary)) return {};
  const char *text = composition.c_str();
  const bool packed = dictionary.packed();
  for (size_t length = composition.size(); length;) {
    size_t longest = 0;
    size_t first = 0, last = dictionary.size();
    while (first < last) {
      const size_t mid = first + (last - first) / 2;
      size_t offset = Reader::read(dictionary.spellingOffsets + mid);
      size_t i = 0;
      uint8_t letter = spellingLetter<Reader>(dictionary.spellings, offset++, packed);
      while (i < length && letter == uint8_t(text[i])) {
        ++i;
        letter = spellingLetter<Reader>(dictionary.spellings, offset++, packed);
      }
      if (i > longest) longest = i;
      const int order = int(letter) - (i < length ? uint8_t(text[i]) : 0);
      if (order < 0) first = mid + 1;
      else if (order > 0) last = mid;
      else {
        if (!packed) return {dictionary.values + Reader::read(dictionary.valueOffsets + mid), length, nullptr};
        const auto *base = reinterpret_cast<const uint16_t *>(dictionary.values);
        const auto *begin = base + packedValueOffset<Reader>(dictionary.valueOffsets, mid);
        const auto *end = base + packedValueOffset<Reader>(dictionary.valueOffsets, mid + 1);
        return {reinterpret_cast<const uint8_t *>(begin), length, reinterpret_cast<const uint8_t *>(end)};
      }
    }
    // At a miss the binary search compared the two insertion neighbours.
    // No key outside that pair can share a longer prefix with this spelling.
    length = longest < length ? longest : length - 1;
  }
  return {};
}

// Legacy UTF-8 callers may continue to use the byte-pointer overload.
template<typename Reader = DirectReader>
const uint8_t *candidate(const uint8_t *first, size_t index) {
  if (!first) return nullptr;
  while (index && Reader::read(first)) {
    while (Reader::read(first++)) {}
    --index;
  }
  return Reader::read(first) ? first : nullptr;
}

// Packed candidate words hold an offset from U+4E00. The top bit terminates
// a candidate, while two consecutive spelling offsets bound the list.
template<typename Reader = DirectReader>
Candidate candidate(Candidate first, size_t index) {
  if (!first) return first;
  if (!first.end) return {candidate<Reader>(first.data, index), nullptr};
  auto *word = reinterpret_cast<const uint16_t *>(first.data);
  const auto *end = reinterpret_cast<const uint16_t *>(first.end);
  for (; word < end && index; ++word) {
    if (Reader::read(word) & 0x8000) --index;
  }
  return {word < end ? reinterpret_cast<const uint8_t *>(word) : nullptr, first.end};
}

template<typename Reader = DirectReader>
size_t candidateCount(Candidate first) {
  if (!first) return 0;
  size_t count = 0;
  if (!first.end) {
    for (; first; first = candidate<Reader>(first, 1)) ++count;
    return count;
  }
  auto *word = reinterpret_cast<const uint16_t *>(first.data);
  const auto *end = reinterpret_cast<const uint16_t *>(first.end);
  for (; word < end; ++word) count += Reader::read(word) >> 15;
  return count;
}

template<typename Reader = DirectReader, typename WriteByte>
void writeCandidate(Candidate value, WriteByte writeByte) {
  if (!value) return;
  if (!value.end) {
    for (const auto *byte = value.data; const auto c = Reader::read(byte); ++byte) writeByte(char(c));
    return;
  }
  auto *word = reinterpret_cast<const uint16_t *>(value.data);
  const auto *end = reinterpret_cast<const uint16_t *>(value.end);
  for (; word < end; ++word) {
    const uint16_t entry = Reader::read(word);
    const uint16_t code = uint16_t(0x4e00 + (entry & 0x7fff));
    writeByte(char(0xe0 | (code >> 12)));
    writeByte(char(0x80 | ((code >> 6) & 63)));
    writeByte(char(0x80 | (code & 63)));
    if (entry & 0x8000) return;
  }
}

}} // namespace nice_gfx::pinyin
