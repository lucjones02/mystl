#include <algorithm>
#include <concepts>
#include <cstddef>
#include <functional>
#include <iterator>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace my_std
{

/* DESIGN CHOICES:
 *
 * There are 2 main things that are tricky here: how we handle buckets and how
 * we handle iterators
 *
 * 1. buckets
 *  - we could simply have the buckets be a vector and store all the buckets in
 * a vector but the issue is that this does not make use of cache loacality in
 * general (and why make life easy when we can make it hard?) so instead we will
 * have one large contiguous chunk of memory that will represent all of the
 * buckets and we will have to manage the buckets manually - the easiest way to
 * do this would be using open addressing, but the standard specifies an
 * interface for buckets which makes this way harder than it needs to be, so we
 * need to use chaining
 *
 *  - so we are basically forced to use one big vector, but then that means that
 * the buckets are no longer resizable arrays, which is not great. We could make
 * each bucket big enough to store the max number of elements per bucket, but
 * that would waste a tremendous amount of space. In theory all elements could
 * be inserted into 1 bucket and so each bucket would need a size of around
 * max_load_factor * num_buckets, which is crazy. In practice however, we will
 * basically never have that situation so we can cut corners.
 *
 * 2. iterators
 *  - now iterators add a bit of a headeache, because there are 2 kind of
 * iterators, the element iterators and the bucket iterators. Now luckily, the
 * bucket iterators are something called a local_iterator and their only
 * restriction is that they are the same type as element iterator, which is a
 * forward iterator. If the local_iterator had to be a contiguous iterator it
 * would have been game over - the fact that both iterators are forward
 * iterators allows us to seperate the big vector into 2 conceptual parts: the
 * "first contact area" and the "backup colision area"
 *
 *  - the first contact area is essentially going to be the area in which we
 * store a key whose hash we haven't seen yet (or have seen only x times, with x
 * small). So it's the area directly associated with the hash of a key
 *
 *  - the backup colision area will be the area in which the key is stored if
 * the first contact area is full and the elements stored in that area can have
 * any hash. The last element in the first contact area will point to the first
 * element in the backup colision area.
 *
 *  - so we are creating n number of linked lists (where n is the number of
 * buckets) and "squashing" them into one large dynamic array so that we can use
 * spatial locality. We are aiming here for a contiguous-ish layout, i.e.
 * traversing the first contact area with occasional jumps to the backup
 * collision area
 *
 *  - the trick for the iterators is to keep one long linked list for all of the
 * elements. That way you can kill 2 birds with 1 stone and have one pointer for
 * both the bucket iterators and the element iterators. The bucket iterators
 * will start at the index of the hash and end at the next index, so the buckets
 * are chained together, which means the element iterators will be that long
 * chain of all the buckets
 *
 *  - i haven't completely thought this through yet, so I'm not sure how that
 * behaviour is going to change when set gets large, but I imagine the locality
 * will get worse, unless we increase the size of the first contact area, but
 * then that will require a lot of memory if the set is large. I'm also not too
 * sure how many jumps to the backup area there will be.
 *
 *  - now it is a few months later and i have grown wiser, but more importantly
 * i have figured out that there is no obligation to keep the order of the
 * buckets, i.e. bucket 0 does not need to have index 0 in our first contact
 * area. i have gone back on the (not the previous one but the one before that)
 * point, i will not be keeping a long linked list for all of the elements,
 * instead by placing the buckets in the order in which they get a first elem
 * we get to bunch up all of the elements into the beginning of the first
 * contact area, which we can go through sequentially. the only pain in the
 * ass that remains is going to be deletion, as that will create "holes" in our
 * first contact area.
 *
 *  - there 2 options i can think of to deal with deletion, the first is to
 * move all of the elements after the key up by one, which is terrible
 * because deletion would no longer be O(1). the second option is to not move
 * all elements and simply allow holes in our first contact area. to do this
 * we would need to keep track of the emptiness of each bucket, which should
 * be do-able at the same time as we keep track of it's order within the
 * first contact area. we would then need to check if a bucket is empty
 * when iterating, which is a little bit of a downside but much less than
 * changing deletion away from constant complexity. if the bucket has more
 * than one element, we can add that element into the newly created hole
 *
 *  - to make all this happen, we are going to have a look up table that
 * will give us the position of the first element in the bucket, if nullptr
 * then the bucket is currently empty
 *
 *  - so for deletion, because we are allowed for it to be worst case linear
 *    what we will do is if the deleted key is in the first contact area and
 *    if that bucket has elements in the backup collision area, we will move
 *    the first backup collision element into the first contact area so we can
 *    guarantee that any pointer in the lut is in the first contact area
 */

template<typename Key>
concept is_hashable = requires(Key k) {
    { std::hash<Key> {}(k) } -> std::convertible_to<std::size_t>;
};

template<typename Key>
struct hash_set_node
{
    hash_set_node* next {nullptr};
    Key            key; // construct on insertion
}; // TODO: think of padding

template<typename Key>
requires is_hashable<Key>
class hash_set
{
public:
    using value_type = Key;

private:
    using node = hash_set_node<value_type>;

    class hash_set_iterator
    {
        friend class hash_set;
        using container_t = hash_set<value_type>;

        hash_set_iterator(const node* current, const container_t* container) :
            _current(current),
            _container(container)
        {
        }

        const node*        _current {nullptr};
        const container_t* _container {nullptr};

    public:
        using value_type        = Key;
        using difference_type   = std::ptrdiff_t;
        using reference         = const value_type&;
        using pointer           = const value_type*;
        using iterator_category = std::forward_iterator_tag;
        using iterator_concept  = std::forward_iterator_tag;

        hash_set_iterator() = default;

        reference operator*() const
        {
            return _current->key;
        }

        pointer operator->() const
        {
            return &_current->key;
        }

        hash_set_iterator& operator++()
        {
            if (_current->next != nullptr) {
                _current = _current->next;
                return *(this);
            }

            // we've hit the end of a bucket so get the start of the next one
            std::size_t next_bucket_index =
                _container->bucket(_current->key) + 1;

            if (next_bucket_index == _container->bucket_count()) {
                _current = &_container->_sentinal_node;
                return *(this);
            }

            while (_container->_is_bucket_empty(next_bucket_index)) {
                ++next_bucket_index;
                if (next_bucket_index == _container->bucket_count()) {
                    _current = &_container->_sentinal_node;
                    return *(this);
                }
            }

            _current = _container->_bucket_lut[next_bucket_index];
            return *(this);
        }

        hash_set_iterator operator++(int)
        {
            hash_set_iterator tmp {*this};
            ++(*this);
            return tmp;
        }

        bool operator==(const hash_set_iterator& other) const
        {
            return _current == other._current;
        }

        bool operator!=(const hash_set_iterator& other) const
        {
            return _current != other._current;
        }
    };

public:
    using iterator       = hash_set_iterator;
    using const_iterator = iterator;

private:
    using iterator_t = node*; // TODO: make this a proper iterator

    node     _sentinal_node;
    iterator _sentinal {&_sentinal_node, this};

    node* _first_contact_begin {nullptr};
    node* _first_contact_end {nullptr};
    node* _backup_collision_begin {nullptr};
    node* _backup_collision_end {nullptr};
    node* _alloc_end {nullptr};

    std::size_t _bucket_count {1};
    std::size_t _element_count {0};

    node** _bucket_lut {nullptr};

    std::vector<node*> _available_slots_first_contact {};
    std::vector<node*> _available_slots_backup_collision {};

    static constexpr std::size_t ELEMENTS_PER_FIRST_CONTACT_SLOT = 1;

private:
    bool _is_bucket_empty(const std::size_t n) const
    {
        return _bucket_lut[n] == nullptr;
    }

    enum class area_tag
    {
        first_contact_area,
        backup_collision_area,
    };

    template<area_tag tag>
    node* _get_first_available_slot()
    {
        if constexpr (tag == area_tag::first_contact_area) {
            if (!_available_slots_first_contact.empty()) {
                node* slot = _available_slots_first_contact.back();
                _available_slots_first_contact.pop_back();
                return slot;
            }
        }
        else if constexpr (tag == area_tag::backup_collision_area) {
            if (!_available_slots_backup_collision.empty()) {
                node* slot = _available_slots_backup_collision.back();
                _available_slots_backup_collision.pop_back();
                return slot;
            }
        }
        return nullptr;
    }

private:
    std::size_t _get_first_contact_slots(const std::size_t n) const
    {
        return n * ELEMENTS_PER_FIRST_CONTACT_SLOT;
    }

    std::size_t _get_backup_collision_slots(const std::size_t n) const
    {
        return n;
    }

    std::size_t _get_total_alloc_size_bytes(const std::size_t n) const
    {
        const std::size_t first_contact_size =
            sizeof(node) * _get_first_contact_slots(n);
        const std::size_t backup_collision_size =
            sizeof(node) * _get_backup_collision_slots(n);
        return first_contact_size + backup_collision_size;
    }

    void _allocate_buckets()
    {
        _first_contact_begin = static_cast<node*>(
            ::operator new(_get_total_alloc_size_bytes(_bucket_count)));

        _backup_collision_begin =
            _first_contact_begin + _get_first_contact_slots(_bucket_count);
        _backup_collision_end = _backup_collision_begin;
        _alloc_end            = _backup_collision_begin +
                                _get_backup_collision_slots(_bucket_count);
    }

    void _init_lut()
    {
        _bucket_lut =
            static_cast<node**>(::operator new(sizeof(node*) * _bucket_count));

        for (std::size_t i {0}; i < _bucket_count; ++i) {
            _bucket_lut[i] = nullptr;
        }
    }

    std::size_t _rehash_policy(const std::size_t n) const
    {
        return n * 2;
    }

public:
    hash_set()
    {
        _allocate_buckets();
        _init_lut();
    }

    hash_set(std::size_t bucket_count) :
        _bucket_count(bucket_count)
    {
        _allocate_buckets();
        _init_lut();
    }

    hash_set(const hash_set& other) :
        _bucket_count(other._bucket_count)
    {
    }

    hash_set(hash_set&& other) {}

    ~hash_set() {}

    hash_set& operator=(const hash_set& other)
    {
        hash_set tmp {other};
        swap(tmp);
        return *this;
    }

    hash_set& operator=(hash_set&& other)
    {
        hash_set tmp {std::move(other)};
        swap(tmp);
        return *this;
    }

    const_iterator begin() const
    {
        for (std::size_t bucket_index {0}; bucket_index < _bucket_count;
             ++bucket_index)
        {
            if (!_is_bucket_empty(bucket_index)) {
                return iterator {_bucket_lut[bucket_index], this};
            }
        }

        return _sentinal;
    }

    iterator begin()
    {
        return std::as_const(*this).begin();
    }

    const_iterator end() const
    {
        return _sentinal;
    }

    iterator end()
    {
        return _sentinal;
    }

    const_iterator cbegin() const
    {
        return begin();
    }

    const_iterator cend() const
    {
        return end();
    }

    bool        empty() const;
    std::size_t size() const;
    std::size_t max_size() const;

    void clear();

private:
    template<typename key_t>
    requires std::is_same_v<std::remove_cvref_t<key_t>, value_type>
    auto _insert_impl(key_t&& k) -> std::pair<iterator, bool>
    {
        const std::size_t bucket_index = bucket(k);
        const auto        hash_func = std::hash<std::remove_cvref_t<key_t>> {};

        if ((_element_count + 1) > max_load_factor() * bucket_count()) {
            const std::size_t new_bucket_count =
                std::min(_rehash_policy(_bucket_count), max_bucket_count());

            // rehash
        }

        if (_is_bucket_empty(bucket_index)) {
            node* replacement_slot =
                _get_first_available_slot<area_tag::first_contact_area>();
            if (replacement_slot == nullptr) {
                _bucket_lut[bucket_index] = _first_contact_end;
                new (_first_contact_end) node {nullptr, std::forward<key_t>(k)};
                ++_first_contact_end;
            }
            else {
                _bucket_lut[bucket_index] = replacement_slot;
                new (replacement_slot) node {nullptr, std::forward<key_t>(k)};
            }

            return std::make_pair(iterator {_bucket_lut[bucket_index], this},
                                  true);
        }

        node* it = _bucket_lut[bucket_index];
        if constexpr (ELEMENTS_PER_FIRST_CONTACT_SLOT == 1) {

            do // while (it->next != nullptr)
            {
                if (hash_func(it->key) == hash_func(k)) {
                    return std::make_pair(iterator {it, this}, false);
                }
                it = it->next == nullptr ? it : it->next;
            } while (it->next != nullptr);

            node* replacement_slot =
                _get_first_available_slot<area_tag::first_contact_area>();
            if (replacement_slot == nullptr) {
                new (_backup_collision_end)
                    node {nullptr, std::forward<key_t>(k)};
                it->next = _backup_collision_end;
                ++_backup_collision_end;
            }
            else {
                _bucket_lut[bucket_index] = replacement_slot;
                new (replacement_slot) node {nullptr, std::forward<key_t>(k)};
            }

            return std::make_pair(iterator {it->next, this}, true);
        }

        // TODO: decide later about making this permanent
        throw std::runtime_error(
            "only ELEMENTS_PER_FIRST_CONTACT_SLOT == 1 is implemented");
    }

public:
    auto insert(const value_type& k) -> std::pair<iterator, bool>
    {
        return _insert_impl(k);
    }

    auto insert(value_type&& k) -> std::pair<iterator, bool>
    {
        return _insert_impl(std::move(k));
    }

    void erase(const Key* pos);
    void erase(const Key& k);

    std::size_t count() const;

    const_iterator find() const;
    iterator       find();

    bool contains(const Key& k);

    // these are helpers for the buckets
    const Key* begin(std::size_t n) const;
    Key*       begin(std::size_t n);

    const Key* end(std::size_t n) const;
    Key*       end(std::size_t n);

    std::size_t bucket_count() const
    {
        return _bucket_count;
    }

    std::size_t max_bucket_count() const
    {
        // FIXME: temp
        return 256;
    }

    std::size_t bucket_size(std::size_t n) const;

    std::size_t bucket(const Key& k) const
    {
        return std::hash<Key> {}(k) % _bucket_count;
    }

    float load_factor() const;

    float max_load_factor() const
    {
        // FIXME: temp
        return 0.85;
    }

    void max_load_factor(float f);

    void rehash(std::size_t count);
    void reserve(std::size_t count);

    void swap(hash_set& other)
    {
        std::swap(_bucket_count, other._bucket_count);
        std::swap(_element_count, other._element_count);
    }
};

} // namespace my_std
