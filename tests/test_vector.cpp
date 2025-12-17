#include <catch2/catch_all.hpp>
#include <iostream>

#include <vector.hpp>

TEST_CASE("vector bring-up", "[VECTOR]")
{

    SECTION("size only constructor")
    {
        constexpr std::size_t vec_size = 1;
        my_std::vector<uint32_t> vec(vec_size);

        REQUIRE(vec._data_begin != 0);
        REQUIRE(vec[0] == uint32_t());
        REQUIRE(vec.size() == vec_size);
    }

    SECTION("value constructor")
    {
        constexpr uint32_t init_value = 2;
        auto vec_size = GENERATE(range(1, 5));

        my_std::vector<uint32_t> vec(vec_size, init_value);

        REQUIRE(vec.size() == vec_size);
        for (std::size_t i = 0; i < vec_size; i++)
        {
            REQUIRE(vec.at(i) == init_value);
        }
    }

    SECTION("empty vector")
    {
        constexpr std::size_t vec_size = 0;
        my_std::vector<uint32_t> vec(vec_size);

        REQUIRE(vec._data_begin == nullptr);
        REQUIRE(vec.empty());
    }
    
    SECTION("basic reserve")
    {
        constexpr std::size_t vec_size = 16;
        my_std::vector<uint32_t> vec;
        vec.reserve(vec_size);

        SUCCEED("no double free / segfaults :)");
    }

    SECTION("reserve with existing vector")
    {
        constexpr uint32_t init_value = 2;
        constexpr std::size_t vec_size = 16;
        constexpr std::size_t new_size = 32;

        my_std::vector<uint32_t> vec(vec_size, init_value);
        vec.reserve(new_size);

        SUCCEED("no double free / segfaults :)");
    }

    SECTION("shrink_to_fit")
    {
        constexpr uint32_t init_value = 0xdeadbeef;
        constexpr std::size_t vec_size = 16;
        constexpr std::size_t new_size = 32;

        my_std::vector<uint32_t> vec(vec_size, init_value);
        vec.reserve(new_size);

        vec.shrink_to_fit();

        REQUIRE(vec.size() == vec_size);
        REQUIRE(vec.capacity() == vec_size);
    }

    SECTION("clear") {
        constexpr uint32_t init_value = 0xdeadbeef;
        constexpr std::size_t vec_size = 16;

        my_std::vector<uint32_t> vec(vec_size, init_value);
        vec.clear();

        REQUIRE(vec.size() == 0);
        REQUIRE(vec.capacity() == vec_size);
    }
}

TEST_CASE("assignments bring-up", "[VECTOR]")
{
    constexpr uint32_t init_value = 0xdeadbeef;
    constexpr std::size_t vec_size = 16;

    my_std::vector<uint32_t> vec(vec_size, init_value);

    SECTION("copy constructor")
    {
        my_std::vector<uint32_t> copied_vec {vec};

        REQUIRE(copied_vec.size() == vec_size);
        REQUIRE(copied_vec.capacity() == vec_size);
        REQUIRE(copied_vec[0] == init_value);

        REQUIRE(vec.size() == vec_size);
        REQUIRE(vec.capacity() == vec_size);
        REQUIRE(vec[0] == init_value);
    }

    SECTION("copy assignment")
    {
        my_std::vector<uint32_t> copied_vec {1};
        copied_vec = vec;

        REQUIRE(copied_vec.size() == vec_size);
        REQUIRE(copied_vec.capacity() == vec_size);
        REQUIRE(copied_vec[0] == init_value);

        REQUIRE(vec.size() == vec_size);
        REQUIRE(vec.capacity() == vec_size);
        REQUIRE(vec[0] == init_value);
    }

    SECTION("move constructor")
    {
        my_std::vector<uint32_t> copied_vec {std::move(vec)};

        REQUIRE(copied_vec.size() == vec_size);
        REQUIRE(copied_vec.capacity() == vec_size);
        REQUIRE(copied_vec[0] == init_value);

        REQUIRE(vec.size() == 0);
        REQUIRE(vec.capacity() == 0);
        REQUIRE(vec._data_begin == nullptr);
    }

    SECTION("move assignment")
    {
        my_std::vector<uint32_t> copied_vec {1};
        copied_vec = std::move(vec);

        REQUIRE(copied_vec.size() == vec_size);
        REQUIRE(copied_vec.capacity() == vec_size);
        REQUIRE(copied_vec[0] == init_value);

        REQUIRE(vec.size() == 0);
        REQUIRE(vec.capacity() == 0);
        REQUIRE(vec._data_begin == nullptr);
    }
}

TEST_CASE("single copy insertion bring-up", "[VECTOR]")
{
    constexpr std::size_t vec_size   = 8;
    my_std::vector<uint32_t> vec(vec_size);
    constexpr uint32_t insert_val = 0xff00ff00;

    SECTION("copy insert without resize")
    {
        constexpr std::size_t alloc_size = 16;
        vec.reserve(alloc_size);

        SECTION("copy insert single element at front")
        {
            uint32_t* insert_pos = vec.begin();
            uint32_t* new_pos = vec.insert(insert_pos, insert_val);

            REQUIRE(vec.front() == insert_val);
            REQUIRE(insert_pos == new_pos);
            REQUIRE(vec.begin() == insert_pos); // check begin hasn't changed
            REQUIRE(*(new_pos) == insert_val);
        }
        SECTION("copy insert single element in middle")
        {
            uint32_t* insert_pos = vec.begin() + (vec_size / 2);
            uint32_t* new_pos = vec.insert(insert_pos, insert_val);

            REQUIRE(vec[vec_size / 2] == insert_val);
            REQUIRE(insert_pos == new_pos);
            REQUIRE(*(new_pos) == insert_val);
        }
        SECTION("copy insert single element at back")
        {
            uint32_t* insert_pos = vec.end();
            uint32_t* new_pos = vec.insert(insert_pos, insert_val);

            REQUIRE(vec.back() == insert_val);
            REQUIRE(insert_pos == new_pos);
            REQUIRE(*(new_pos) == insert_val);
        }
    }

    SECTION("copy insert while resize")
    {
        SECTION("copy insert single element at front")
        {
            uint32_t* insert_pos = vec.begin();
            uint32_t* new_pos = vec.insert(insert_pos, insert_val);

            REQUIRE(vec.front() == insert_val);
            REQUIRE(vec.begin() != insert_pos); // this is a valid check because when we allocate we haven't deallocated the old mem yet so the insert_pos is not available
            REQUIRE(vec.begin() == new_pos);
        }

        SECTION("copy insert single element in middle")
        {
            uint32_t* insert_pos = vec.begin() + vec_size / 2;
            uint32_t* new_pos = vec.insert(insert_pos, insert_val);

            REQUIRE(vec[vec_size / 2] == insert_val);
            REQUIRE(*(new_pos) == insert_val);
        }

        SECTION("copy insert single element at back")
        {
            uint32_t* insert_pos = vec.end();
            uint32_t* new_pos = vec.insert(insert_pos, insert_val);

            REQUIRE(vec.back() == insert_val);
            REQUIRE(*(new_pos) == insert_val);
        }
    }
}

TEST_CASE("single move insertion bring-up", "[VECTOR]")
{
    struct move_only_t
    {
        uint32_t* ptr = nullptr;
        move_only_t() { ptr = nullptr; }
        move_only_t(uint32_t val)
        {
            ptr = new uint32_t;
            *ptr = val;
        }
        ~move_only_t()
        {
            if (ptr != nullptr)
            {
                delete ptr;
            }
        }

        // delete copy constructor;
        move_only_t(const move_only_t&) = delete;

        move_only_t(move_only_t&& rhs)
        {
            if (ptr != nullptr)
            {
                delete ptr;
            }
            ptr = rhs.ptr;
            rhs.ptr = nullptr;
        }

        move_only_t& operator=(move_only_t&& rhs)
        {
            if (ptr != nullptr)
            {
                delete ptr;
            }
            ptr = rhs.ptr;
            rhs.ptr = nullptr;
            return *this;
        }
    };

    constexpr std::size_t vec_size = 8;
    my_std::vector<move_only_t> vec(vec_size);
    constexpr uint32_t setup_val = 1;

    for (auto& e : vec)
    {
        e.ptr = new uint32_t(setup_val);
    }

    SECTION("move insert without resize")
    {
        constexpr std::size_t alloc_size = 16;
        vec.reserve(alloc_size);

        SECTION("move insert at front")
        {
            move_only_t insert_obj(2);
            uint32_t* insert_ptr_val = insert_obj.ptr;
            move_only_t* begin = vec.begin();
            vec.insert(vec.begin(), std::move(insert_obj));

            REQUIRE(vec.begin() == begin);
            REQUIRE(vec.front().ptr == insert_ptr_val);
        }
    }

    SECTION("move insert with resize")
    {
        SECTION("move insert at front")
        {
            move_only_t insert_obj(2);
            uint32_t* insert_ptr_val = insert_obj.ptr;
            vec.insert(vec.begin(), std::move(insert_obj));

            REQUIRE(vec.front().ptr == insert_ptr_val);
        }
    }
}

TEST_CASE("multi insertion bring-up", "[VECTOR]")
{
    constexpr std::size_t vec_size   = 8;
    my_std::vector<uint32_t> vec(vec_size);
    constexpr uint32_t insert_val = 0xffeeddcc;

    SECTION("multi insertion without resize")
    {
        constexpr std::size_t alloc_size = 16;
        vec.reserve(alloc_size);

        SECTION("insert 2 at front")
        {
            constexpr std::size_t insert_size = 2;
            vec.insert(vec.begin(), insert_size, insert_val);

            REQUIRE(vec.front() == insert_val);
            REQUIRE(vec[1] == insert_val);
        }

        SECTION("insert 2 at back")
        {
            constexpr std::size_t insert_size = 2;
            uint32_t* return_pos = vec.insert(vec.end(), insert_size, insert_val);

            REQUIRE(vec.back() == insert_val);
            REQUIRE(*return_pos == insert_val);
            REQUIRE(*(return_pos+1) == insert_val);
        }
    }

    SECTION("multi insertion with resize")
    {
        SECTION("insert 2 at front")
        {
            constexpr std::size_t insert_size = 2;
            uint32_t* insert_pos = vec.begin();
            uint32_t* return_pos = vec.insert(insert_pos, insert_size, insert_val);

            REQUIRE(vec.front() == insert_val);
            REQUIRE(vec.begin() != insert_pos);
            REQUIRE(return_pos == vec.begin());
            REQUIRE(vec[1] == insert_val);
        }

        SECTION("insert 2 at back")
        {
            constexpr std::size_t insert_size = 2;
            uint32_t* insert_pos = vec.end();
            uint32_t* return_pos = vec.insert(insert_pos, insert_size, insert_val);

            REQUIRE(vec.back() == insert_val);
            REQUIRE(insert_pos != vec.end());
            REQUIRE(*return_pos == insert_val);
            REQUIRE(*(return_pos+1) == insert_val);
        }
    }
}

TEST_CASE("erase bring-up", "VECTOR")
{
    constexpr std::size_t vec_size = 16;
    my_std::vector<uint32_t> vec(vec_size);
    std::vector<uint32_t> expected_final_vector = { 1, 2, 3, 4, 6, 7, 8, 9, 12, 13, 14, 15};

    for (int i = 0; i < vec.size(); i++)
    {
        vec[i] = i;
    }

    vec.erase(vec.begin());
    REQUIRE(vec.size() == vec_size - 1);
    REQUIRE(vec.front() == 1);
    
    vec.erase(vec.begin() + 4);
    REQUIRE(vec.size() == vec_size - 2);
    REQUIRE(vec[3] == 4);
    REQUIRE(vec[4] == 6);

    vec.erase(vec.begin()+8, vec.begin()+10);
    REQUIRE(vec.size() == vec_size - 4);
    REQUIRE(vec[7] == 9);
    REQUIRE(vec[8] == 12);
    REQUIRE(vec[9] == 13);


    REQUIRE(vec.size() == expected_final_vector.size());
    for (int i = 0; i < vec.size(); i++)
    {
        REQUIRE(vec[i] == expected_final_vector[i]);
    }
}

TEST_CASE("push_back bring-up", "[VECTOR]")
{
    constexpr std::size_t vec_size = 8;
    my_std::vector<uint32_t> vec(vec_size);

    SECTION("no resize")
    {
        constexpr std::size_t alloc_size = 16;
        vec.reserve(alloc_size);

        vec.push_back(1);

        REQUIRE(vec.size() == vec_size+1);
        REQUIRE(vec.back() == 1);
    }

    SECTION("with resizing")
    {
        vec.push_back(1);
        REQUIRE(vec.size() == vec_size+1);
        REQUIRE(vec.capacity() > vec_size);
        REQUIRE(vec.back() == 1);
    }
}

TEST_CASE("pop_back bring-up", "[VECTOR]")
{
    constexpr std::size_t vec_size = 8;
    my_std::vector<uint32_t> vec(vec_size);

    for (int i = 0; i < vec.size(); i++)
    {
        vec[i] = i;
    }

    REQUIRE(vec.back() == vec.size()-1);

    vec.pop_back();

    REQUIRE(vec.size() == vec_size - 1);
    REQUIRE(vec.back() == vec.size() - 1);
}

TEST_CASE("resize bring-up", "[VECTOR]")
{
    constexpr std::size_t vec_size = 8;
    using test_t = uint32_t;
    my_std::vector<test_t> vec(vec_size);

    SECTION("resize to same size")
    {
        vec.resize(vec_size);
        REQUIRE(vec.size() == vec_size);
    }

    SECTION("resize to bigger")
    {
        constexpr std::size_t resize_factor = 2;
        SECTION("without value")
        {
            vec.resize(vec_size * resize_factor);

            REQUIRE(vec.size() == vec_size * resize_factor);
            REQUIRE(vec.back() == test_t());
        }

        SECTION("with value")
        {
            constexpr test_t val = 0x11223344;
            vec.resize(vec_size * resize_factor, val);

            REQUIRE(vec.size() == vec_size * resize_factor);
            REQUIRE(vec.back() == val);
        }
    }

    SECTION("resize to smaller")
    {
        constexpr std::size_t resize_divisor = 2;
        vec.resize(vec_size / resize_divisor);

        REQUIRE(vec.size() == vec_size / resize_divisor);
    }
}


TEST_CASE("swap bring-up", "[VECTOR]")
{
    using test_t = uint32_t;
    constexpr std::size_t vec_size_1 = 2;
    constexpr test_t init_value_1 = 0x11111111;

    constexpr std::size_t vec_size_2 = 4;
    constexpr test_t init_value_2 = 0x22222222;


    my_std::vector<test_t> vec1(vec_size_1, init_value_1);
    my_std::vector<test_t> vec2(vec_size_2, init_value_2);

    vec1.swap(vec2);

    REQUIRE(vec1.size() == vec_size_2);
    REQUIRE(vec2.size() == vec_size_1);

    REQUIRE(vec1.front() == init_value_2);
    REQUIRE(vec2.front() == init_value_1);

    REQUIRE(vec1[1] == init_value_2);
    REQUIRE(vec2[1] == init_value_1);
}
