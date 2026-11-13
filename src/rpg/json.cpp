#include "rpg/json.hpp"

#include <cstdio>

namespace chili::rpg::json {

namespace {

struct Parser {
  std::string_view text;
  std::vector<Token> &tokens;
  size_t pos = 0;
  std::string error;
  int depth = 0;

  int line_at(size_t at) const {
    int line = 1;
    for(size_t i = 0; i < at && i < text.size(); i++)
      if(text[i] == '\n') line++;
    return line;
  }

  bool fail(const char *what) {
    if(error.empty()) {
      char buf[96];
      snprintf(buf, sizeof(buf), "line %d: %s", line_at(pos), what);
      error = buf;
    }
    return false;
  }

  void skip_space() {
    while(pos < text.size() && (text[pos] == ' ' || text[pos] == '\n' || text[pos] == '\r' || text[pos] == '\t')) pos++;
  }

  bool literal(std::string_view word) {
    if(text.substr(pos, word.size()) != word) return fail("expected a value");
    pos += word.size();
    return true;
  }

  // Leaves pos after the closing quote. start/end hold the raw contents.
  bool string(uint32_t &start, uint32_t &end) {
    pos++;  // opening quote
    start = pos;
    while(pos < text.size() && text[pos] != '"') {
      if(text[pos] == '\\') pos++;
      else if((unsigned char)text[pos] < 0x20) return fail("control character in a string");
      pos++;
    }
    if(pos >= text.size()) return fail("unterminated string");
    end = pos++;
    return true;
  }

  bool value() {
    if(++depth > 64) return fail("too deeply nested");
    skip_space();
    if(pos >= text.size()) return fail("expected a value");

    uint32_t me = tokens.size();
    tokens.push_back(Token{Type::null, false, 0, uint32_t(pos), uint32_t(pos), 0, 0});
    char c = text[pos];

    if(c == '{' || c == '[') {
      bool object = c == '{';
      tokens[me].type = object ? Type::object : Type::array;
      pos++;
      skip_space();
      char close = object ? '}' : ']';
      uint16_t count = 0;
      if(pos < text.size() && text[pos] == close) {
        pos++;
      } else {
        while(true) {
          if(object) {
            skip_space();
            if(pos >= text.size() || text[pos] != '"') return fail("expected a key in quotes");
            // a key is stored as a string token right before its value
            uint32_t key = tokens.size();
            tokens.push_back(Token{Type::string, false, 0, 0, 0, 0, 0});
            if(!string(tokens[key].start, tokens[key].end)) return false;
            tokens[key].next = key + 1;
            skip_space();
            if(pos >= text.size() || text[pos] != ':') return fail("expected ':'");
            pos++;
          }
          if(!value()) return false;
          if(count == 0xffff) return fail("too many items");
          count++;
          skip_space();
          if(pos < text.size() && text[pos] == ',') {
            pos++;
            continue;
          }
          if(pos < text.size() && text[pos] == close) {
            pos++;
            break;
          }
          return fail(object ? "expected ',' or '}'" : "expected ',' or ']'");
        }
      }
      tokens[me].count = count;
    } else if(c == '"') {
      tokens[me].type = Type::string;
      if(!string(tokens[me].start, tokens[me].end)) return false;
    } else if(c == 't') {
      tokens[me].type = Type::boolean;
      tokens[me].value = true;
      if(!literal("true")) return false;
    } else if(c == 'f') {
      tokens[me].type = Type::boolean;
      if(!literal("false")) return false;
    } else if(c == 'n') {
      if(!literal("null")) return false;
    } else if(c == '-' || (c >= '0' && c <= '9')) {
      tokens[me].type = Type::integer;
      bool negative = c == '-';
      if(negative) pos++;
      if(pos >= text.size() || text[pos] < '0' || text[pos] > '9') return fail("expected a number");
      int64_t n = 0;
      while(pos < text.size() && text[pos] >= '0' && text[pos] <= '9') {
        n = n * 10 + (text[pos++] - '0');
        if(n > 2147483647) return fail("number too big");
      }
      if(pos < text.size() && (text[pos] == '.' || text[pos] == 'e' || text[pos] == 'E'))
        return fail("numbers must be whole numbers");
      tokens[me].number = int32_t(negative ? -n : n);
    } else {
      return fail("expected a value");
    }

    tokens[me].end = tokens[me].type == Type::string ? tokens[me].end : uint32_t(pos);
    tokens[me].next = tokens.size();
    depth--;
    return true;
  }
};

void append_utf8(std::string &out, uint32_t cp) {
  if(cp < 0x80) {
    out += char(cp);
  } else if(cp < 0x800) {
    out += char(0xc0 | (cp >> 6));
    out += char(0x80 | (cp & 0x3f));
  } else {
    out += char(0xe0 | (cp >> 12));
    out += char(0x80 | ((cp >> 6) & 0x3f));
    out += char(0x80 | (cp & 0x3f));
  }
}

int hex(char c) {
  if(c >= '0' && c <= '9') return c - '0';
  if(c >= 'a' && c <= 'f') return c - 'a' + 10;
  if(c >= 'A' && c <= 'F') return c - 'A' + 10;
  return 0;
}

}

std::string Doc::parse(std::string_view t) {
  text = t;
  tokens.clear();
  Parser p{text, tokens, 0, {}, 0};
  if(p.value()) {
    p.skip_space();
    if(p.pos != text.size()) p.fail("text after the end of the file");
  }
  if(!p.error.empty()) tokens.clear();
  return p.error;
}

const Token &Ref::token() const { return doc->tokens[index]; }
Type Ref::type() const { return token().type; }
bool Ref::boolean() const { return doc && token().type == Type::boolean && token().value; }
int32_t Ref::integer() const { return doc && token().type == Type::integer ? token().number : 0; }
int Ref::size() const { return doc && (type() == Type::array || type() == Type::object) ? token().count : 0; }

int Ref::line() const {
  if(!doc) return 0;
  int line = 1;
  for(uint32_t i = 0; i < token().start && i < doc->text.size(); i++)
    if(doc->text[i] == '\n') line++;
  return line;
}

std::string Ref::string() const {
  std::string out;
  if(!is_string()) return out;
  std::string_view raw = doc->text.substr(token().start, token().end - token().start);
  for(size_t i = 0; i < raw.size(); i++) {
    char c = raw[i];
    if(c != '\\' || i + 1 >= raw.size()) {
      out += c;
      continue;
    }
    c = raw[++i];
    switch(c) {
      case 'n': out += '\n'; break;
      case 't': out += '\t'; break;
      case 'r': out += '\r'; break;
      case 'b': out += '\b'; break;
      case 'f': out += '\f'; break;
      case 'u':
        if(i + 4 < raw.size()) {
          uint32_t cp = 0;
          for(int k = 1; k <= 4; k++) cp = cp * 16 + hex(raw[i + k]);
          append_utf8(out, cp);
          i += 4;
        }
        break;
      default: out += c;  // \" \\ \/
    }
  }
  return out;
}

Ref Ref::get(std::string_view key) const {
  for(auto m : members())
    if(m.key() == key) return m;
  return Ref();
}

Ref Ref::at(int n) const {
  if(!is_array() || n < 0 || n >= size()) return Ref();
  int i = 0;
  for(auto v : items())
    if(i++ == n) return v;
  return Ref();
}

Ref::Range Ref::items() const {
  return Range(doc, index + 1, is_array() ? size() : 0, false);
}

Ref::Range Ref::members() const {
  return Range(doc, index + 1, is_object() ? size() : 0, true);
}

Ref Ref::Range::It::operator*() const {
  if(!object) return Ref(doc, index);
  const Token &k = doc->tokens[index];
  Ref r(doc, index + 1);
  r.key_ = doc->text.substr(k.start, k.end - k.start);
  return r;
}

Ref::Range::It &Ref::Range::It::operator++() {
  uint32_t value = object ? index + 1 : index;
  index = doc->tokens[value].next;
  left--;
  return *this;
}

}
