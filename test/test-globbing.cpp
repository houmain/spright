
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

  CHECK(replace_suffix("test-abc.png", "-abc", "-xyz") == "test-xyz.png");
  CHECK(replace_suffix("test-abc.png", "-efg", "-xyz") == "test-abc-xyz.png");
  CHECK(replace_suffix("test.png", "", "-xyz") == "test-xyz.png");
  CHECK(replace_suffix("test-abc-efg.png", "-abc", "-xyz") == "test-xyz-efg.png");

  CHECK(replace_suffix("test-abc", "-abc", "-xyz") == "test-xyz");
  CHECK(replace_suffix("test-abc", "-efg", "-xyz") == "test-abc-xyz");
  CHECK(replace_suffix("test", "", "-xyz") == "test-xyz");
  CHECK(replace_suffix("test-abc-efg", "-abc", "-xyz") == "test-xyz-efg");

  CHECK(has_suffix("test-abc.png", "-abc"));
  CHECK(!has_suffix("test-abcd.png", "-abc"));
  CHECK(has_suffix("test-abc", "-abc"));
  CHECK(!has_suffix("test-abcd", "-abc"));
}
