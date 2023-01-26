
#include "catch.hpp"
#include "src/common.h"
#include <sstream>

using namespace spright;

TEST_CASE("join_expressions") {
  auto arguments = std::vector<std::string_view>{ };
  split_arguments(" a  + b ", &arguments);
  CHECK(arguments.size() == 3);
  join_expressions(&arguments);
  CHECK(arguments.size() == 1);
  CHECK(arguments[0] == "a  + b");

  split_arguments(" a  -b ", &arguments);
  CHECK(arguments.size() == 2);
  join_expressions(&arguments);
  CHECK(arguments.size() == 1);
  CHECK(arguments[0] == "a  -b");

  split_arguments("c a+  b- d", &arguments);
  CHECK(arguments.size() == 4);
  join_expressions(&arguments);
  CHECK(arguments.size() == 2);
  CHECK(arguments[1] == "a+  b- d");

  split_arguments("+ a+ + ++b- ", &arguments);
  CHECK(arguments.size() == 4);
  join_expressions(&arguments);
  CHECK(arguments.size() == 1);
  CHECK(arguments[0] == "+ a+ + ++b-");
}

TEST_CASE("split_expression") {
  auto arguments = std::vector<std::string_view>{ };
  split_expression("a  + b", &arguments);
  CHECK(arguments.size() == 3);
  CHECK(arguments[0] == "a");
  CHECK(arguments[1] == "+");
  CHECK(arguments[2] == "b");

  split_expression("a+  b- d", &arguments);
  CHECK(arguments.size() == 5);

  split_expression("", &arguments);
  CHECK(arguments.size() == 1);
}

TEST_CASE("Rect") {
  CHECK(combine(Rect(0, 0, 4, 4), Rect(4, 0, 4, 4)) == Rect(0, 0, 8, 4));
}
