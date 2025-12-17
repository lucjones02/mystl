#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>


namespace my_std
{

template <typename T>
struct vector 
{
    T* _data_begin {nullptr};
    T* _data_end {nullptr};
    T* _alloc_end {nullptr};

    vector() = default;

    explicit vector(std::size_t count)
    {
        std::size_t alloc_size = std::min(count, MAX_SIZE);
        allocate(alloc_size);

        while (_data_end != _alloc_end)
        {
            // *(_data_end) = std::move(T()); // TODO: I think this requires T to be default constructable
            (_data_end) = new (_data_end) T();
            ++_data_end;
        }
    }

    explicit vector(std::size_t count, const T& value)
    {
        std::size_t alloc_size = std::min(count, MAX_SIZE);
        allocate(alloc_size);

        while (_data_end != _alloc_end)
        {
            *(_data_end) = value;
            ++_data_end;
        }
    }

    vector(const vector& other)
    {
        allocate(other.capacity());

        for (std::size_t index {0}; index < other.size(); ++index)
        {
            new (_data_end) T{other[index]};
            ++_data_end;
        }
    }

    vector(vector&& other)
    {
        swap(other);
    }

    vector& operator=(const vector& other)
    {
        vector tmp {other};
        swap(tmp);
        return *this;
    }

    vector& operator=(vector&& other)
    {
        vector tmp {std::move(other)};
        swap(tmp);
        return *this;
    }
    
    ~vector()
    {
        deallocate();
    }

private:
    // only allocates count * sizeof(T) raw bytes
    void allocate(std::size_t alloc_size)
    {
        if (alloc_size == 0)
        {
            _data_begin = nullptr;
            _data_end   = nullptr;
            _alloc_end  = nullptr;
            return;
        }

        // using operator new here because it allows me to only allocate raw bytes
        // so constructor calls are kept for later
        _data_begin = static_cast<T*>(::operator new(alloc_size * sizeof(T)));
        _data_end   = _data_begin;
        _alloc_end  = _data_begin + alloc_size; 
    }

    // TODO: alignment delete thing in the specs
    void deallocate(T* data_begin, T* data_end)
    {
        if (data_begin != nullptr)
        {
            // reverse order from construction
            T* it = data_end;
            while (it != data_begin)
            {
                (--it)->~T();
            }

            // operator delete does not call destructor as used with operator new
            // so all good here no double free
            ::operator delete(data_begin);
        }
    }

    void deallocate()
    {
        deallocate(_data_begin, _data_end);

        _data_begin = nullptr;
        _data_end   = nullptr;
        _alloc_end  = nullptr;
    }

    std::size_t next_cap(const std::size_t cap) const
    {
        return 2 * cap;
    }

    static constexpr std::size_t MAX_SIZE = 1 << 12;


public:
    const T& at(std::size_t pos) const
    {
        if (pos > _data_end - _data_begin) throw std::out_of_range("my_std::vector::at - ");
        return *(_data_begin + pos);
    }

    T& at(std::size_t pos)
    {
        return const_cast<T&>(std::as_const(*this).at(pos));
    }


    const T& operator[](std::size_t pos) const
    {
        return *(_data_begin + pos);
    }

    T& operator[](std::size_t pos)
    {
        return const_cast<T&>(std::as_const(*this)[pos]);
    }
    

    const T& front() const
    {
        return *(_data_begin);
    }

    T& front()
    {
        return const_cast<T&>(std::as_const(*this).front());
    }
     

    const T& back() const
    {
        return *(_data_end - 1);
    }

    T& back()
    {
        return const_cast<T&>(std::as_const(*this).back());
    }


    const T* begin() const
    {
        return _data_begin;
    }

    T* begin()
    {
        return const_cast<T*>(std::as_const(*this).begin());
    }


    const T* end() const
    {
        return _data_end;
    }

    T* end()
    {
        return const_cast<T*>(std::as_const(*this).end());
    }


    std::size_t size() const
    {
        return (_data_end - _data_begin);
    }

    std::size_t max_size() const
    {
        return MAX_SIZE * sizeof(T);
    }

    bool empty() const
    {
        return this->size() == 0;
    }


    void reserve(std::size_t new_cap)
    {
        if (new_cap <= this->capacity()) return;

        std::size_t old_size = this->size();
        T* previous_start = _data_begin;

        std::size_t alloc_size = std::min(new_cap, MAX_SIZE);
        allocate(alloc_size);

        if (previous_start == nullptr) return;

        std::size_t pos = 0;
        while (pos < old_size)
        {
            _data_end = new (_data_end) T(std::move(*(previous_start + pos)));
            // *(_data_end) = std::move(*(previous_start + pos));
            _data_end++;
            pos++;
        }

        deallocate(previous_start, previous_start + old_size);
    }

    std::size_t capacity() const
    {
        return _alloc_end - _data_begin;
    }

    void shrink_to_fit()
    {
        std::size_t old_size = this->size();
        T* previous_start = _data_begin;

        allocate(this->size());
        
        if (previous_start == nullptr) return;

        std::size_t pos = 0;
        while (pos < old_size)
        {
            *(_data_end++) = std::move(*(previous_start + pos++));
        }

        delete previous_start;
    }

    void clear()
    {
        std::size_t cap = this->capacity();

        deallocate();
        allocate(cap);
    }

private:

    template<typename Arg>
    T* __insert_impl(const T* pos, std::size_t count, Arg&& value)
    {
        //TODO: this will leak memory if a new statement throws an error
        if (count == 0) return const_cast<T*>(pos);

        if (pos > _data_end || pos < _data_begin)
        {
            throw std::out_of_range("insert");
        }

        // resize if necessary
        // we treat both of these cases seperately instead of calling resize if needed and then inserting
        // because I can avoid moving over all the elements just to have to move more right after
        if (this->size() + count > this->capacity())
        {
            T* _prev_begin = this->begin();
            T* _prev_end   = this->end();

            std::size_t alloc_size = std::min(next_cap(this->capacity()), MAX_SIZE);
            allocate(alloc_size);

            T* it  = _prev_begin;
            while (it != pos)
            {
                new (_data_end++) T(std::move(*(it++)));
            }

            T* insertion_pos = _data_end;

            while (count)
            {
                // we use forward here because considering how we call this function, we are guaranteed an lvalue ref here
                // if count is > 1 so we will never move something twice
                new (_data_end++) T(std::forward<Arg>(value));
                count--;
            }

            while (it != _prev_end)
            {
                new (_data_end++) T(std::move(*(it++)));
            }

            deallocate(_prev_begin, _prev_end);

            return insertion_pos;
        }
        else
        {
            T* new_it = _data_end + count;
            T* old_it = _data_end;
            while (old_it != pos)
            {
                new(--new_it) T(std::move(*(--old_it)));
            }

            T* it = const_cast<T*>(pos);
            while (it < const_cast<T*>(pos) + count)
            {
                // we use forward here to avoid compilation errors but considering how we
                // call this function, we are guaranteed an lvalue ref here
                new ((it++)) T(std::forward<Arg>(value));
                _data_end++;
            }

            return const_cast<T*>(pos);
        }
    }

public:

    [[maybe_unused]] T* insert(const T* pos, const T& value)
    {
        return __insert_impl(pos, 1, value);
    }

    [[maybe_unused]] T* insert(const T* pos, T&& value)
    {
        return __insert_impl(pos, 1, std::move(value));
    }

    [[maybe_unused]] T* insert(const T* pos, std::size_t count, const T& value)
    {
        return __insert_impl(pos, count, value);
    }

private:
    T* __erase_impl(const T* first, const T* last)
    {
        T* it = const_cast<T*>(last);

        T* new_it = const_cast<T*>(first);
        while (it < _data_end)
        {
            *(new_it++) = std::move(*(it++));
        }

        T* new_end = new_it;

        while (new_it < _data_end)
        {
            (new_it++)->~T();
        }

        _data_end = new_end;

        return const_cast<T*>(last);
    }

public:
    [[maybe_unused]] T* erase(T* pos)
    {
        return __erase_impl(pos, pos + 1);
    }

    [[maybe_unused]] T* erase(const T* pos)
    {
        return __erase_impl(pos, pos + 1);
    }

    [[maybe_unused]] T* erase(T* first, T* last)
    {
        return __erase_impl(first, last);
    }

    [[maybe_unused]] T* erase(const T* first, const T* last)
    {
        return __erase_impl(first, last);
    }

public:

    void push_back(const T& value)
    {
        __insert_impl(_data_end, 1, value);
    }

    void push_back(T&& value)
    {
        __insert_impl(_data_end, 1, std::move(value));
    }

    template <class... Args>
    T& emplace_back(Args&&... args);

    void pop_back()
    {
        __erase_impl(_data_end - 1, _data_end);
    }

    void resize(std::size_t count)
    {
        this->resize(count, T());
    }

    void resize(std::size_t count, const T& value)
    {
        if (count == this->size()) return;

        if (count < this->size())
        {
            __erase_impl(this->begin() + count, _data_end);
        }
        else
        {
            __insert_impl(_data_end, count - this->size(), value);
        }
    }

    void swap(vector& other) noexcept
    {
        std::swap(_data_begin, other._data_begin);
        std::swap(_data_end, other._data_end);
        std::swap(_alloc_end, other._alloc_end);
    }
};

} // namespace my_std
