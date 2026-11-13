#pragma once

// A small JSON reader for game files. It keeps the text and a flat list of
// tokens (about 16 bytes each), so a whole game file fits in a few KB of RAM
// while it loads. Numbers must be whole numbers.

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace chili::rpg::json {

enum class Type : uint8_t { null, boolean, integer, string, array, object };

struct Token {
  Type type;
  bool value;        // boolean
  uint16_t count;    // items, or members of an object
  uint32_t start;    // in the text: string contents, or the first character
  uint32_t end;
  uint32_t next;     // the token after this value and everything inside it
  int32_t number;    // integer
};

class Doc;

// A value inside a Doc. A missing value is false: `if(auto v = obj.get("x"))`.
class Ref {
public:
  Ref() = default;
  Ref(const Doc *doc, uint32_t index) : doc(doc), index(index) {}

  explicit operator bool() const { return doc != nullptr; }
  Type type() const;
  bool is_null() const { return doc && type() == Type::null; }
  bool is_bool() const { return doc && type() == Type::boolean; }
  bool is_int() const { return doc && type() == Type::integer; }
  bool is_string() const { return doc && type() == Type::string; }
  bool is_array() const { return doc && type() == Type::array; }
  bool is_object() const { return doc && type() == Type::object; }

  bool boolean() const;
  int32_t integer() const;
  // Unescaped text of a string.
  std::string string() const;
  // Items of an array, or members of an object.
  int size() const;
  // The line the value starts on, for error messages.
  int line() const;

  // A member of an object, or an empty Ref.
  Ref get(std::string_view key) const;
  // An item of an array, or an empty Ref.
  Ref at(int n) const;
  // The key, when this value came from members().
  std::string_view key() const { return key_; }

  // Iterates items() or members(): `for(auto v : ref.items())`.
  class Range {
  public:
    struct It {
      const Doc *doc;
      uint32_t index;
      int left;
      bool object;
      Ref operator*() const;
      It &operator++();
      bool operator!=(const It &o) const { return left != o.left; }
    };
    Range(const Doc *doc, uint32_t first, int count, bool object) : doc(doc), first(first), count(count), object(object) {}
    It begin() const { return {doc, first, count, object}; }
    It end() const { return {doc, 0, 0, object}; }

  private:
    const Doc *doc;
    uint32_t first;
    int count;
    bool object;
  };
  Range items() const;
  Range members() const;

private:
  friend class Doc;
  const Token &token() const;

  const Doc *doc = nullptr;
  uint32_t index = 0;
  std::string_view key_;
};

class Doc {
public:
  // Reads `text`, which must stay alive while the Doc is used. Returns "" or
  // an error such as "line 3: expected a value".
  std::string parse(std::string_view text);
  Ref root() const { return tokens.empty() ? Ref() : Ref(this, 0); }

private:
  friend class Ref;
  std::string_view text;
  std::vector<Token> tokens;
};

}
