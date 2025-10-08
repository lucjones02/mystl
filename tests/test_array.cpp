#include <catch2/catch_test_macros.hpp>
#include <array.hpp>
#include <iostream>
#include <array.hpp>

TEST_CASE("array bring-up", "[ARRAY]")
{
    my_array<uint32_t, 4> arr = {0, 1, 2, 3};

    for (uint32_t i = 0; i < arr.size(); i++)
    {
        REQUIRE(arr[i] == i);
    }

    for (uint32_t i = 0; auto it : arr)
    {
        REQUIRE(it == i++);
    }

    arr.fill(0);
    for (uint32_t i = 0; i < arr.size(); i++)
    {
        REQUIRE(arr[i] == 0);
    }

    for (uint32_t i = 0; i < arr.size(); i++)
    {
        arr[i] = 2 * i + 1;
        REQUIRE(arr[i] == 2 * i + 1);
    }

    arr.fill(1);
    for (auto it = arr.begin(); it != arr.end(); it++)
    {
        REQUIRE(*it == 1);
    }
}

TEST_CASE("array swap", "[ARRAY]")
{
    my_array<uint32_t, 4> arr0 {0, 0, 0, 0};
    my_array<uint32_t, 4> arr1 {1, 1, 1, 1};

    arr0.swap(arr1);

    for (auto e : arr0)
    {
        REQUIRE(e == 1);
    }

    for (auto e : arr1)
    {
        REQUIRE(e == 0);
    }

}
