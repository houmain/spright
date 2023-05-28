
#include "catch.hpp"
#include "src/common.h"
#include <sstream>

using namespace spright;

TEST_CASE("join_expressions") {
  auto arguments = std::vector<std::string_view>{ };
  split_arguments(" a  + b ", &arguments);
  CHECK(arguments.size() == 3);
  join_expressions(&arguments);
  REQUIRE(arguments.size() == 1);
  CHECK(arguments[0] == "a  + b");

  split_arguments(" a  -b ", &arguments);
  CHECK(arguments.size() == 2);
  join_expressions(&arguments);
  REQUIRE(arguments.size() == 1);
  CHECK(arguments[0] == "a  -b");

  split_arguments("c a+  b- d", &arguments);
  CHECK(arguments.size() == 4);
  join_expressions(&arguments);
  REQUIRE(arguments.size() == 2);
  CHECK(arguments[1] == "a+  b- d");

  split_arguments("+ a+ + ++b- ", &arguments);
  CHECK(arguments.size() == 4);
  join_expressions(&arguments);
  REQUIRE(arguments.size() == 1);
  CHECK(arguments[0] == "+ a+ + ++b-");
}

TEST_CASE("split_expression") {
  auto arguments = std::vector<std::string_view>{ };
  split_expression("a  + b", &arguments);
  REQUIRE(arguments.size() == 3);
  CHECK(arguments[0] == "a");
  CHECK(arguments[1] == "+");
  CHECK(arguments[2] == "b");

  split_expression("a+  b- d", &arguments);
  CHECK(arguments.size() == 5);

  split_expression("", &arguments);
  CHECK(arguments.size() == 1);
}

TEST_CASE("split_arguments") {
  auto arguments = std::vector<std::string_view>{ };
  split_arguments(" a  + b ", &arguments);
  CHECK(arguments.size() == 3);

  CHECK_THROWS(split_arguments(" \"a  + b ", &arguments));
  CHECK_THROWS(split_arguments(" 'a  + b ", &arguments));
  CHECK_THROWS(split_arguments(" a  + b \"", &arguments));
  CHECK_THROWS(split_arguments(" a  + b '", &arguments));
}

TEST_CASE("split_arguments - optional comma") {
  auto arguments = std::vector<std::string_view>{ };
  split_arguments(" def a b ", &arguments);
  REQUIRE(arguments.size() == 3);
  CHECK(arguments[0] == "def");
  CHECK(arguments[1] == "a");
  CHECK(arguments[2] == "b");

  split_arguments(" def a,b ", &arguments);
  REQUIRE(arguments.size() == 3);
  CHECK(arguments[0] == "def");
  CHECK(arguments[1] == "a");
  CHECK(arguments[2] == "b");
  
  split_arguments(" def  a , b ", &arguments);
  REQUIRE(arguments.size() == 3);
  CHECK(arguments[0] == "def");
  CHECK(arguments[1] == "a");
  CHECK(arguments[2] == "b");

  split_arguments(" def \" a,b \"  c ", &arguments);
  REQUIRE(arguments.size() == 3);
  CHECK(arguments[0] == "def");
  CHECK(arguments[1] == " a,b ");
  CHECK(arguments[2] == "c");

  split_arguments(" def \" a,b \" , c ", &arguments);
  REQUIRE(arguments.size() == 3);
  CHECK(arguments[0] == "def");
  CHECK(arguments[1] == " a,b ");
  CHECK(arguments[2] == "c");

  // do not allow after definition name
  CHECK_THROWS(split_arguments("def,a,b,c", &arguments));

  // do not allow trailing comma
  CHECK_THROWS(split_arguments("def a,b,c,", &arguments));
  CHECK_THROWS(split_arguments("def a , b , c , ", &arguments));

  // allow to mix (because of pivot expression)
  CHECK_NOTHROW(split_arguments("def a , b c ", &arguments));
  CHECK_NOTHROW(split_arguments("def a b , c ", &arguments));
  
}

TEST_CASE("Rect") {
  CHECK(combine(Rect(0, 0, 4, 4), Rect(4, 0, 4, 4)) == Rect(0, 0, 8, 4));
}

TEST_CASE("replace_variables") {

  const auto replace_function = [](std::string_view string) {
    if (string == "sprite.id")
      return "ok";
    if (string == "empty")
      return "";
    throw std::runtime_error("unknown variable");
  };
  const auto replace = [&](std::string string) {
    replace_variables(string, replace_function);
    return string;
  };
  CHECK(replace("test-{{sprite.id}}-test") == "test-ok-test");
  CHECK(replace("test-{{ sprite.id}}-test") == "test-ok-test");
  CHECK(replace("test-{{sprite.id  }}-test") == "test-ok-test");
  CHECK(replace("test-{{empty }}-test") == "test--test");
  CHECK(replace("test-{{ empty}}-test") == "test--test");
  CHECK(replace("test-{{  empty    }}-test") == "test--test");
  CHECK_THROWS(replace("test-{{}}-test"));
  CHECK_THROWS(replace("test-{{ }}-test"));
  CHECK_THROWS(replace("test-{{ sprite2.id }}-test"));
}
