#pragma once
#include <vector>
#include <string>

#include <algorithm>

/**
 * Fixed size priority queue.
 * 
 * implemented as a simple sorted vector.
 * 
 * Insertion is 0(log(n) + n) ~ O(n)
 * Access to min is 0(1)
 * 
 * Note: this could also be implemented as an interval heap, but it's more complex to implement.
 * Access would be O(log(n)) and access to min would also be 0(1)
 **/
struct FixedSizePriorityQueue {
    struct Entry {
        std::string url;
        size_t count;
        Entry(const std::pair<std::string, size_t>& p): url(p.first), count(p.second) {}
        bool operator<(const Entry& other) const {
            return this->count > other.count;
        }
    };
    FixedSizePriorityQueue() = delete;
    FixedSizePriorityQueue(size_t size) {
        array.reserve(size);
    }

    void add(const std::pair<std::string, size_t>& p) {
        const auto val = Entry{p};
        if (array.size() < array.capacity()) {
            // there is still some room in the array, we just insert the element at the right position
            array.insert(std::lower_bound(std::begin(array), std::end(array), val), val);
            return;
        }

        const auto current_min = array.back().count;
        if (current_min >= val.count) {
            // we can skip this element, it's worst than the worst in the list
            return;
        }

        // we insert the new element in its right place, and move all the others down
        const auto insert_place = std::lower_bound(std::begin(array), std::end(array), val);
        auto next_val = val;
        for (auto it = insert_place; it != std::end(array); ++it) {
            std::swap(*it, next_val);
        }
    }

    // Note: in a real project, I would have implemented iterators for this struct
    // but doing it without boost::iterator_facade is a pain
    void print() const {
        for (const auto& s: array) {
            std::cout << s.url << " " << s.count << std::endl;
        }
    }
private:
    std::vector<Entry> array;
};