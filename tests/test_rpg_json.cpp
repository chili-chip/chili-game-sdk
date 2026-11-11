#include "check.hpp"

#include "rpg/json.hpp"

#include <string>

using namespace chili::rpg;

TEST(reads_nested_values) {
  json::Doc doc;
  CHECK(doc.parse(R"({"a": 1, "b": [true, false, null], "c": {"d": "hi", "e": -42}})").empty());
  auto root = doc.root();
  CHECK(root.is_object());
  CHECK_EQ(root.get("a").integer(), 1);
  CHECK_EQ(root.get("b").size(), 3);
  CHECK(root.get("b").at(0).boolean());
  CHECK(!root.get("b").at(1).boolean());
  CHECK(root.get("b").at(2).is_null());
  CHECK(root.get("c").get("d").string() == "hi");
  CHECK_EQ(root.get("c").get("e").integer(), -42);
  CHECK(!root.get("missing"));
}

TEST(walks_members_in_file_order) {
  json::Doc doc;
  CHECK(doc.parse(R"({"z": 1, "y": [1, [2, 3]], "x": 3})").empty());
  std::string keys;
  for(auto m : doc.root().members()) keys += m.key();
  CHECK(keys == "zyx");
  int sum = 0;
  for(auto v : doc.root().get("y").at(1).items()) sum += v.integer();
  CHECK_EQ(sum, 5);
}

TEST(unescapes_strings) {
  json::Doc doc;
  CHECK(doc.parse(R"(["a\"b\\c\nd", "A"])").empty());
  CHECK(doc.root().at(0).string() == "a\"b\\c\nd");
  CHECK(doc.root().at(1).string() == "A");
}

TEST(reports_errors_with_a_line) {
  json::Doc doc;
  std::string err = doc.parse("{\n  \"a\": 1,\n  \"b\": }");
  CHECK(err.find("line 3") != std::string::npos);
  CHECK(!doc.parse("[1, 2").empty());
  CHECK(!doc.parse("{\"a\" 1}").empty());
  CHECK(!doc.parse("1.5").empty());
  CHECK(!doc.parse("[1] 2").empty());
}
