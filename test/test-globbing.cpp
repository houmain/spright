
#include "catch.hpp"
#include "src/globbing.h"

using namespace spright;

TEST_CASE("globbing - Match") {
  CHECK(match("", ""));
  CHECK(!match("a", "b"));
  CHECK(match("?", "b"));
  CHECK(match("?a", "ba"));
  CHECK(!match("?a", "bb"));
  CHECK(!match("?", ""));
  CHECK(match("*", ""));
  CHECK(match("*", "a"));
  CHECK(match("a*", "a"));
  CHECK(match("*a", "a"));
  CHECK(match("a*b", "ab"));
  CHECK(match("a*b", "acb"));
  CHECK(match("a*b", "acccb"));
  CHECK(!match("*", "/"));
  CHECK(!match("*", "a/"));
  CHECK(match("a/*", "a/"));
  CHECK(match("a/*", "a/b"));
  CHECK(match("**/*", ""));
  CHECK(match("**/*", "a"));
  CHECK(match("**/*", "a/b"));
  CHECK(match("**/*b", "a/b"));
  CHECK(match("a/**/b", "a/b"));
  CHECK(match("a/**/b", "a/c/b"));
  CHECK(match("a/**/b", "a/ccc/ddd/b"));
  CHECK(!match("a/**/b", "a/ccc/ddd/bbb"));
  CHECK(match("a/**/*b", "a/ccc/ddd/bbb"));
  CHECK(!match("a/**/b", "ab"));
}
