#pragma once
#include <cstdint>
#include <functional>
#include <vector>
#include <algorithm>

// ═════════════════════════════════════════════════════════════════════════════
//  Signal<Args...>  — lightweight signal/slot (like Qt, but std::function)
//
//  Usage:
//      Signal<int> onValueChanged;
//      auto id = onValueChanged.connect([](int v) { printf("%d\n", v); });
//      onValueChanged.emit(42);
//      onValueChanged.disconnect(id);
// ═════════════════════════════════════════════════════════════════════════════

template <typename... Args>
class Signal
{
public:
    using SlotFunc = std::function<void(Args...)>;
    using SlotId   = uint32_t;

    SlotId connect(SlotFunc fn)
    {
        SlotId id = nextId_++;
        slots_.push_back({id, std::move(fn)});
        return id;
    }

    void disconnect(SlotId id)
    {
        slots_.erase(
            std::remove_if(slots_.begin(), slots_.end(),
                           [id](const Slot& s) { return s.id == id; }),
            slots_.end());
    }

    void disconnectAll() { slots_.clear(); }

    void emit(Args... args) const
    {
        for (const auto& s : slots_)
            s.fn(args...);
    }

    bool empty() const { return slots_.empty(); }

private:
    struct Slot
    {
        SlotId   id;
        SlotFunc fn;
    };

    std::vector<Slot> slots_;
    SlotId nextId_ = 1;
};
