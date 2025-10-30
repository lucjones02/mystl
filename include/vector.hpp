#include <cstddef>

template <typename T>
struct my_vector 
{
    T* _data_begin;
    T* _data_end;
    T* _alloc_end;


    const T& at() const;
    T& at();

    const T& operator[]() const;
    T& operator[]();

    const T& front() const;
    T& front();

    const T& back() const;
    T& back();

    const T* begin() const;
    T* begin();

    const T* end() const;
    T* end();

    bool empty() const;
    std::size_t size() const;
    std::size_t max_size() const;

    void reserve(std::size_t new_cap);
    std::size_t capacity() const;
    void shrink_to_fit();

    void clear();

    void insert(const T* pos, const T& value);
    void insert(const T* pos, T&& value);
    void insert(const T* pos, std::size_t count, const T& value);

    T* erase(T* pos);
    T* erase(const T* pos);
    T* erase(T* first, T* last);
    T* erase(const T* first, const T* last);

    void push_back(const T& value);
    void push_back(T&& value);

    void pop_back();

    void resize(std::size_t count);
    void resize(std::size_t count, const T& value);

    void swap(my_vector& other) noexcept;

};
