
#include "catch.hpp"
#include "src/globbing.h"
#include "src/FilenameSequence.h"

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
  CHECK(replace_suffix("test.png", "", "") == "test.png");
  CHECK(replace_suffix("test-abc.png", "-abc", "") == "test.png");

  CHECK(replace_suffix("test-abc", "-abc", "-xyz") == "test-xyz");
  CHECK(replace_suffix("test-abc", "-efg", "-xyz") == "test-abc-xyz");
  CHECK(replace_suffix("test", "", "-xyz") == "test-xyz");
  CHECK(replace_suffix("test-abc-efg", "-abc", "-xyz") == "test-xyz-efg");
  CHECK(replace_suffix("test", "", "") == "test");
  CHECK(replace_suffix("test-abc", "-abc", "") == "test");

  CHECK(has_suffix("test-abc.png", "-abc"));
  CHECK(!has_suffix("test-abcd.png", "-abc"));
  CHECK(has_suffix("test-abc", "-abc"));
  CHECK(!has_suffix("test-abcd", "-abc"));
}

TEST_CASE("FilenameSequence") {
  CHECK(!FilenameSequence("test.txt").is_sequence());
  CHECK(!FilenameSequence("test{.txt").is_sequence());
  CHECK(!FilenameSequence("test}.txt").is_sequence());
  CHECK(!FilenameSequence("test{0.txt").is_sequence());
  CHECK(!FilenameSequence("test{0-.txt").is_sequence());
  CHECK(!FilenameSequence("test{02-a}.txt").is_sequence());
  CHECK(!FilenameSequence("test{a-}.txt").is_sequence());
  CHECK(!FilenameSequence("test{a-02}.txt").is_sequence());
  CHECK(FilenameSequence("test{0-10}.txt").is_sequence());
  CHECK(FilenameSequence("test{0-10}.txt").count() == 11);
  CHECK(FilenameSequence("test{0}.txt").is_sequence());
  CHECK(FilenameSequence("test{0}.txt").count() == 1);
  CHECK(FilenameSequence("test{0}.txt").first() == 0);
  CHECK(FilenameSequence("test{1}.txt").is_sequence());
  CHECK(FilenameSequence("test{1}.txt").count() == 1);
  CHECK(FilenameSequence("test{1}.txt").first() == 1);
  CHECK(FilenameSequence("test{01-0}.txt").is_sequence());
  CHECK(FilenameSequence("test{01-0}.txt").count() == 0);
  CHECK(FilenameSequence("test{01-01}.txt").count() == 1);
  CHECK(FilenameSequence("test{02-01}.txt").count() == 0);
  CHECK(FilenameSequence("test{0-}.txt").is_sequence());
  CHECK(FilenameSequence("test{08-}.txt").is_infinite_sequence());
  CHECK(FilenameSequence("test{00-01}.txt").is_sequence());
  CHECK(FilenameSequence("test{00-01}.txt").count() == 2);
  CHECK(FilenameSequence("test{08-10}.txt").first() == 8);
  CHECK(FilenameSequence("test{08-1.txt").get_nth_filename(1) == "test{08-1.txt");
  CHECK(FilenameSequence("test{08-10}.txt").get_nth_filename(1) == "test09.txt");
  CHECK(FilenameSequence("test{08-10}.txt").get_nth_filename(3) == "test11.txt");
  CHECK(FilenameSequence("test{08-}.txt").get_nth_filename(1) == "test09.txt");
  CHECK(FilenameSequence("test{0008-}.txt").get_nth_filename(101) == "test0109.txt");

  auto filename = FilenameSequence("test{08-10}.txt");
  filename.set_count(3);
  CHECK(filename == "test{08-10}.txt");

  filename.set_infinite();
  CHECK(filename == "test{08-}.txt");

  filename.set_count(1000);
  CHECK(filename == "test{08-1007}.txt");

  CHECK(try_make_sequence("test01.txt", "test02.txt").is_sequence());
  CHECK(try_make_sequence("test01.txt", "test02.txt").count() == 2);
  CHECK(try_make_sequence("test01.txt", "test08.txt").is_sequence());
  CHECK(try_make_sequence("test01.txt", "test08.txt").count() == 8);
  CHECK(!try_make_sequence("test01.txt", "tes02.txt").is_sequence());
}
