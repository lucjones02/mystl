#pragma once

#include <cstddef>
#include <cassert>
#include <utility>
#include <array>

template <typename T, size_t N>
struct my_array {
    // no constructors to make this aggregate
    // also no private/protected data members to make this aggregate
    T _data[N];

    size_t size()
    {
        return N;
    }

    const T& at(size_t n) const
    {
        assert(n < N);
        return _data[n];
    }

    T& at(size_t n)
    {
        return const_cast<T&>(std::as_const(*this).at(n));
    }

    const T& operator[](size_t n) const
    {
        return _data[n];
    }

    T& operator[](size_t n)
    {
        return const_cast<T&>(std::as_const(*this)[n]);
    }

    const T& front() const
    {
        return _data[0];
    }

    T& front()
    {
        return const_cast<T&>(std::as_const(*this).front());
    }

    const T& back() const
    {
        return _data[N-1];
    }

    T& back()
    {
        return const_cast<T&>(std::as_const(*this).back());
    }

    T* begin()
    {
        return &_data[0];
    }
    T* end()
    {
        return &_data[N];
    }

    bool empty()
    {
        return N==0;
    }

    void fill(const T& value)
    {
        for (size_t i = 0; i < N; i++)
        {
            _data[i] = value;
        }
    }

    void swap(my_array& other) noexcept
    {
        std::swap_ranges(begin(), end(), other.begin());
    }

};
