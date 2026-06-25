#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <utility>
#include <vector>

template <typename K, typename V, typename Hash = std::hash<K>>
class FlatHashMap {
public:
    static constexpr std::size_t kEmpty = std::numeric_limits<std::size_t>::max();

    explicit FlatHashMap(std::size_t capacity = 0) { reserve(capacity); }

    void reserve(std::size_t min_capacity) {
        std::size_t cap = 16;
        while (cap < min_capacity * 2) {
            cap <<= 1;
        }
        keys_.assign(cap, K{});
        values_.assign(cap, V{});
        occupied_.assign(cap, false);
        tombstone_.assign(cap, false);
        size_ = 0;
        tombstones_ = 0;
    }

    [[nodiscard]] bool contains(const K& key) const noexcept {
        return find_slot(key) != kEmpty;
    }

    V* find(const K& key) noexcept {
        const std::size_t slot = find_slot(key);
        return slot == kEmpty ? nullptr : &values_[slot];
    }

    const V* find(const K& key) const noexcept {
        const std::size_t slot = find_slot(key);
        return slot == kEmpty ? nullptr : &values_[slot];
    }

    V& operator[](const K& key) {
        std::size_t slot = probe(key);
        if (!occupied_[slot]) {
            keys_[slot] = key;
            occupied_[slot] = true;
            tombstone_[slot] = false;
            ++size_;
        }
        return values_[slot];
    }

    bool erase(const K& key) noexcept {
        const std::size_t slot = find_slot(key);
        if (slot == kEmpty) {
            return false;
        }
        occupied_[slot] = false;
        tombstone_[slot] = true;
        --size_;
        ++tombstones_;
        return true;
    }

    [[nodiscard]] std::size_t size() const noexcept { return size_; }

private:
    [[nodiscard]] std::size_t mask() const noexcept { return keys_.size() - 1; }

    [[nodiscard]] std::size_t hash_index(const K& key) const noexcept {
        return Hash{}(key) & mask();
    }

    [[nodiscard]] std::size_t find_slot(const K& key) const noexcept {
        if (keys_.empty()) {
            return kEmpty;
        }
        std::size_t idx = hash_index(key);
        while (occupied_[idx] || tombstone_[idx]) {
            if (occupied_[idx] && keys_[idx] == key) {
                return idx;
            }
            idx = (idx + 1) & mask();
        }
        return kEmpty;
    }

    [[nodiscard]] std::size_t probe(const K& key) noexcept {
        std::size_t idx = hash_index(key);
        while (occupied_[idx]) {
            if (keys_[idx] == key) {
                return idx;
            }
            idx = (idx + 1) & mask();
        }
        if (tombstone_[idx]) {
            tombstone_[idx] = false;
            --tombstones_;
        }
        return idx;
    }

    std::vector<K> keys_;
    std::vector<V> values_;
    std::vector<bool> occupied_;
    std::vector<bool> tombstone_;
    std::size_t size_ = 0;
    std::size_t tombstones_ = 0;
};
