module;

#include <cstddef>

export module AgisPointersModule;

import <vector>;

namespace Agis
{

export template<typename T>
class StridedPointer {
public:
    // Constructor
    StridedPointer(T* ptr, std::size_t size, std::size_t stride)
        : data(ptr), elementCount(size), strideSize(stride) {}
    StridedPointer(std::vector<T>& v, size_t stride)
        : data(v.data()), elementCount(v.size()), strideSize(stride) {}

    ~StridedPointer() = default;

    // Subscript operator overload
    T& operator[](std::size_t index) {
        return data[index * strideSize];
    }

    // Const version of the subscript operator overload
    const T& operator[](std::size_t index) const {
        return data[index * strideSize];
    }

    T const* get() const { return this->data; }

    // Custom iterator
    class iterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        // Constructor
        iterator(T* ptr, std::size_t stride) : ptr(ptr), stride(stride) {}

        // Dereference operator
        reference operator*() const {
            return *ptr;
        }

        // Pointer arithmetic operators
        iterator& operator++() {
            ptr += stride;
            return *this;
        }

        iterator operator++(int) {
            iterator temp = *this;
            ++(*this);
            return temp;
        }

        iterator& operator--() {
            ptr -= stride;
            return *this;
        }

        iterator operator--(int) {
            iterator temp = *this;
            --(*this);
            return temp;
        }

        iterator& operator+=(difference_type n) {
            ptr += n * stride;
            return *this;
        }

        iterator& operator-=(difference_type n) {
            ptr -= n * stride;
            return *this;
        }

        iterator operator+(difference_type n) const {
            iterator temp = *this;
            return temp += n;
        }

        iterator operator-(difference_type n) const {
            iterator temp = *this;
            return temp -= n;
        }

        difference_type operator-(const iterator& other) const {
            return (ptr - other.ptr) / stride;
        }

        // Comparison operators
        bool operator==(const iterator& other) const {
            return ptr == other.ptr;
        }

        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }

        bool operator<(const iterator& other) const {
            return ptr < other.ptr;
        }

        bool operator>(const iterator& other) const {
            return other < *this;
        }

        bool operator<=(const iterator& other) const {
            return !(other < *this);
        }

        bool operator>=(const iterator& other) const {
            return !(*this < other);
        }

    private:
        T* ptr;              // Pointer to the data
        std::size_t stride;  // Size of the stride
    };

    size_t size() const { return this->elementCount; }

    // Begin iterator
    iterator begin() const {
        return iterator(data, strideSize);
    }

    // End iterator
    iterator end() const {
        return iterator(data + (elementCount * strideSize), strideSize);
    }

private:
    T* data;              // Pointer to the data
    std::size_t elementCount;  // Number of elements
    std::size_t strideSize;    // Size of the stride
};
}