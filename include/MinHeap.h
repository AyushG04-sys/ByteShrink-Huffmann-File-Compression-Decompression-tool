#ifndef MIN_HEAP_H
#define MIN_HEAP_H

#include <vector>
#include <stdexcept>
#include <utility>
#include <cstddef>

/**
 * @brief Custom MinHeap template data structure.
 *
 * Implements a complete binary min-heap stored in a dynamic array (std::vector).
 *
 * DSA Array Index Properties (0-indexed):
 * - Parent of node at index i:      floor((i - 1) / 2)
 * - Left child of node at index i:  2 * i + 1
 * - Right child of node at index i: 2 * i + 2
 *
 * Complexity:
 * - Insert (Push):      O(log k) using siftUp
 * - ExtractMin (Pop):   O(log k) using siftDown
 * - Peek (Get Min):     O(1)
 * - Space Complexity:   O(k) where k is the number of elements
 *
 * @tparam T Element type (e.g., HuffmanNode*)
 * @tparam Compare Comparator functor where Compare()(a, b) returns true if a < b (has higher priority)
 */
template <typename T, typename Compare = std::less<T>>
class MinHeap {
private:
    std::vector<T> heap;
    Compare comp;

    inline size_t parent(size_t i) const { return (i - 1) / 2; }
    inline size_t leftChild(size_t i) const { return 2 * i + 1; }
    inline size_t rightChild(size_t i) const { return 2 * i + 2; }

    /**
     * @brief Sifts an element up until heap property is restored.
     * Used after inserting a new element at the end of the array.
     */
    void siftUp(size_t i) {
        while (i > 0 && comp(heap[i], heap[parent(i)])) {
            std::swap(heap[i], heap[parent(i)]);
            i = parent(i);
        }
    }

    /**
     * @brief Sifts an element down until heap property is restored.
     * Used after replacing root with the last element during extractMin.
     */
    void siftDown(size_t i) {
        size_t smallest = i;
        size_t n = heap.size();

        while (true) {
            size_t left = leftChild(i);
            size_t right = rightChild(i);

            if (left < n && comp(heap[left], heap[smallest])) {
                smallest = left;
            }
            if (right < n && comp(heap[right], heap[smallest])) {
                smallest = right;
            }

            if (smallest != i) {
                std::swap(heap[i], heap[smallest]);
                i = smallest;
            } else {
                break;
            }
        }
    }

public:
    MinHeap() = default;
    explicit MinHeap(const Compare& comparator) : comp(comparator) {}

    /**
     * @brief Inserts an element into the min-heap.
     * @param val Element to insert.
     */
    void insert(const T& val) {
        heap.push_back(val);
        siftUp(heap.size() - 1);
    }

    /**
     * @brief Removes and returns the minimum element (root).
     * @return T The minimum element.
     * @throws std::runtime_error if heap is empty.
     */
    T extractMin() {
        if (heap.empty()) {
            throw std::runtime_error("Cannot extract from an empty MinHeap");
        }
        T minVal = heap[0];
        heap[0] = heap.back();
        heap.pop_back();
        if (!heap.empty()) {
            siftDown(0);
        }
        return minVal;
    }

    /**
     * @brief Returns a reference to the minimum element without removing it.
     */
    const T& peek() const {
        if (heap.empty()) {
            throw std::runtime_error("Cannot peek into an empty MinHeap");
        }
        return heap[0];
    }

    size_t size() const { return heap.size(); }
    bool empty() const { return heap.empty(); }

    void clear() {
        heap.clear();
    }

    const std::vector<T>& rawData() const {
        return heap;
    }
};

#endif // MIN_HEAP_H
