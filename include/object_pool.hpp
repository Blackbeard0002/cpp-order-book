#pragma once

#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

template <typename T>
class ObjectPool {
public:
    static constexpr std::uint32_t kInvalid = std::numeric_limits<std::uint32_t>::max();

    explicit ObjectPool(std::size_t capacity) : free_head_(kInvalid) {
        slots_.reserve(capacity);
        for (std::uint32_t i = 0; i < capacity; ++i) {
            slots_.emplace_back();
            slots_.back().next_free = (i + 1 < capacity) ? i + 1 : kInvalid;
        }
        free_head_ = capacity > 0 ? 0 : kInvalid;
    }

    [[nodiscard]] std::uint32_t acquire() noexcept {
        if (free_head_ == kInvalid) {
            return kInvalid;
        }
        const std::uint32_t idx = free_head_;
        free_head_ = slots_[idx].next_free;
        slots_[idx].next_free = kInvalid;
        slots_[idx].in_use = true;
        return idx;
    }

    void release(std::uint32_t idx) noexcept {
        if (idx >= slots_.size()) {
            return;
        }
        slots_[idx].in_use = false;
        slots_[idx].next_free = free_head_;
        free_head_ = idx;
    }

    [[nodiscard]] T& get(std::uint32_t idx) noexcept { return slots_[idx].value; }
    [[nodiscard]] const T& get(std::uint32_t idx) const noexcept { return slots_[idx].value; }

    [[nodiscard]] bool in_use(std::uint32_t idx) const noexcept {
        return idx < slots_.size() && slots_[idx].in_use;
    }

    [[nodiscard]] std::size_t capacity() const noexcept { return slots_.size(); }

private:
    struct Slot {
        T value{};
        std::uint32_t next_free = kInvalid;
        bool in_use = false;
    };

    std::vector<Slot> slots_;
    std::uint32_t free_head_ = kInvalid;
};
