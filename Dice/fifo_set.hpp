/*
 * fifo_set
 * Copyright (C) 2023 String.Empty
 */
#pragma once
#include <unordered_set>
#include <unordered_map>
#include <map>

template<typename _Kty>
class fifo_set_base {
protected:
    size_t inc_end{ 0 };
    std::unordered_map<_Kty, size_t> index;
    std::map<size_t, _Kty> orders;
public:
    using value_type = _Kty;
    fifo_set_base() = default;
    bool operator[](const _Kty& key) const{
        return index.count(key);
    }
    size_t size()const { return index.size(); }
    bool empty()const { return index.empty(); }
    const value_type& in(size_t idx)const {
        return orders.at(idx);
    }
    size_t next(size_t idx)const {
        auto fwd{ orders.upper_bound(idx) };
        return fwd == orders.end() ? inc_end : fwd->first;
    }
    size_t next(size_t idx, size_t n)const {
        auto fwd{ orders.lower_bound(idx) };
        std::advance(fwd, n);
        return fwd == orders.end() ? inc_end : fwd->first;
    }
};
template<typename _Kty>
class fifo_const_iter {
    using value_type = _Kty;
    using const_iterator = fifo_const_iter<_Kty>;
    using _Base = fifo_set_base<_Kty>;
    const _Base* const con;
public:
    size_t idx;
    explicit fifo_const_iter(const _Base* ref, size_t i = 0) :con(ref), idx(i) {}
     const value_type& operator*()const {
        return con->in(idx);
    }
     const value_type* operator->() {
        return &con->in(idx);
    }
    bool operator!=(const const_iterator& other)const {
        return con != other.con ||
            (idx != other.idx && !con->empty());
    }
    const_iterator& operator++() {
        idx = con->next(idx);
        return *this;
    }
};
template<typename _Kty>
class fifo_set_iter {
    using _Base = fifo_set_base< _Kty>;
    _Base* con;
public:
    size_t idx;
    using iterator = fifo_set_iter<_Kty>;
    using iterator_category = std::random_access_iterator_tag;
    using value_type = _Kty;
    using difference_type = ptrdiff_t;
    using pointer = value_type*;
    using reference = const value_type&;
    explicit fifo_set_iter(_Base* ref = nullptr, size_t i = 0) :con(ref), idx(i) {}
    reference operator*() {
        return con->in(idx);
    }
    pointer operator->()const {
        return &(const_cast<reference>(con->in(idx)));
    }
    bool operator==(const iterator& other)const {
        return con == other.con &&
            (idx == other.idx || con->empty());
    }
    bool operator!=(const iterator& other)const {
        return con != other.con ||
            (idx != other.idx && !con->empty());
    }
    iterator& operator=(const iterator& other) {
        con = other.con;
        idx = other.idx;
        return *this;
    }
    iterator& operator++() {
        idx = con->next(idx);
        return *this;
    }
    iterator& operator+=(size_t n) {
        idx = con->next(idx, n);
        return *this;
    }
};
template<typename _Kty, typename Compare = void,
    typename _Alloc = std::allocator<_Kty>>
class fifo_set :public fifo_set_base<_Kty> {
    //std::mutex ex_key;
    using _Base = fifo_set_base<_Kty>;
    using const_iterator = fifo_const_iter<_Kty>;
public:
    using key_type = _Kty;
    using value_type = _Kty;
    using iterator = fifo_set_iter<_Kty>;
    fifo_set() {};
    fifo_set(std::initializer_list<_Kty> list) {
        for (auto& x : list) {
            insert(x);
        }
    }
    bool operator==(const fifo_set<_Kty>& other)const { return _Base::values == other.values; }
    size_t max_size()const noexcept { return _Base::index.max_size(); }
    bool count(const _Kty& key)const { return _Base::index.count(key); }
    iterator begin() {
        return iterator(this, _Base::orders.empty() ? 0 : _Base::orders.begin()->first);
    }
    const_iterator begin()const {
        return const_iterator(this, _Base::orders.empty() ? 0 : _Base::orders.begin()->first);
    }
    const_iterator cbegin()const { return begin(); }
    iterator end() {
        return iterator(this, _Base::inc_end);
    }
    const_iterator end()const {
        return const_iterator(this, _Base::inc_end);
    }
    const_iterator cend()const { return end(); }
    iterator find(const _Kty& key) {
        if (_Base::index.count(key)) {
            return iterator(this, _Base::index.at(key));
        }
        return end();
    }
    const_iterator find(const _Kty& key)const {
        if (_Base::index.count(key)) {
            return const_iterator(this, _Base::index.at(key));
        }
        return end();
    }
    std::pair<iterator, bool> insert(const value_type& p) {
        if (auto it{ _Base::index.find(p) }; it != _Base::index.end()) {
            return { iterator(this, it->second), false };
        }
        _Base::orders.emplace(_Base::inc_end, p);
        _Base::index.emplace(p, _Base::inc_end);
        return { iterator(this, ++_Base::inc_end), true };
    }
    iterator emplace(const _Kty& key) {
        if (_Base::index.count(key)) {
            return end();
        }
        _Base::orders.emplace(_Base::inc_end, key);
        _Base::index[key] = _Base::inc_end;
        return iterator(this, ++_Base::inc_end);
    }
    iterator erase(const _Kty& key) {
        //exlock guard{ ex_key };
        if (auto it(_Base::index.find(key)); it != _Base::index.end()) {
            auto idx{ _Base::index.at(key) };
            auto next{ ++_Base::orders.find(idx) };
            _Base::index.erase(it);
            _Base::orders.erase(idx);
            if (next != _Base::orders.end())return iterator(this, next->first);
        }
        return end();
    }
    iterator erase(const iterator& iter) {
        //exlock guard{ ex_key };
        if (auto it(_Base::orders.find(iter.idx)); it != _Base::orders.end()) {
            _Base::index.erase(it->second);
            _Base::orders.erase(it);
            return iterator(this, _Base::next(iter.idx));
        }
        return end();
    }
    void clear() noexcept {
        if (_Base::inc_end) {
            _Base::inc_end = 0;
            _Base::index.clear();
            _Base::orders.clear();
        }
    }
};