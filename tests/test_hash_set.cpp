#include <catch2/catch_test_macros.hpp>
#include <hash_set.hpp>

TEST_CASE("hash set bring-up", "[HASH_SET]")
{
    STATIC_REQUIRE(my_std::is_hashable<int>);

    my_std::hash_set<int> set;

    SUCCEED();
}
