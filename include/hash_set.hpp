#include <cstddef>
#include <concepts>
#include <functional>

namespace my_std
{

template<typename Key>
concept is_hashable = requires(Key k)
{
    { std::hash<Key>{}(k) } -> std::convertible_to<std::size_t>;
};

template <typename Key> requires is_hashable<Key>
class hash_set
{
private:

public:

    hash_set();
    hash_set(std::size_t bucket_count);

    hash_set(const hash_set& other);
    hash_set(hash_set&& other);

    ~hash_set();


    hash_set& operator=(const hash_set& other) const;
    hash_set& operator=(hash_set& other);
    hash_set& operator=(hash_set&& other);

    //TODO use iterators
    const Key* begin() const;
    Key* begin();

    const Key* end() const;
    Key* end();

    bool empty() const;
    std::size_t size() const;
    std::size_t max_size() const;


    void clear();
    void insert(const Key& k);
    void insert(Key&&);

    void erase(const Key* pos);
    void erase(const Key& k);

    std::size_t count() const;

    const Key* find() const;
    Key* find();

    bool contains(const Key& k);

    // these are helpers for the buckets
    const Key* begin(std::size_t n) const;
    Key* begin(std::size_t n);

    const Key* end(std::size_t n) const;
    Key* end(std::size_t n);

    std::size_t bucket_count() const;
    std::size_t max_bucket_count() const;

    std::size_t bucket_size(std::size_t n) const;
    std::size_t bucket(const Key& k);

    float load_factor() const;

    float max_load_factor() const;
    void max_load_factor(float f);

    void rehash(std::size_t count);
    void reserve(std::size_t count);


};

} // namespace my_std
