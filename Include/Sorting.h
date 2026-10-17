#pragma once
// Our own sorting + searching used for the admin views:
//   mergeSort    - stable, O(n log n) always
//   quickSort    - not stable, O(n log n) average, O(n^2) worst case
//   binarySearch - O(log n), works only on a SORTED vector
#include <cstddef>
#include <utility>
#include <vector>

namespace sorting {
namespace detail {
template <typename T, typename Less>
void mergeSortRec(std::vector<T>& v, std::vector<T>& tmp, size_t lo, size_t hi, Less less) {
    if (hi - lo < 2) return;
    size_t mid = lo + (hi - lo) / 2;
    mergeSortRec(v, tmp, lo, mid, less);
    mergeSortRec(v, tmp, mid, hi, less);
    size_t i = lo, j = mid, k = lo;
    while (i < mid && j < hi) tmp[k++] = less(v[j], v[i]) ? v[j++] : v[i++];
    while (i < mid) tmp[k++] = v[i++];
    while (j < hi) tmp[k++] = v[j++];
    for (size_t x = lo; x < hi; ++x) v[x] = tmp[x];
}

// Quick sort: pick the middle element as pivot, put smaller ones left, bigger ones right.
template <typename T, typename Less>
void quickSortRec(std::vector<T>& v, int lo, int hi, Less less) {
    if (lo >= hi) return;
    T pivot = v[lo + (hi - lo) / 2];
    int i = lo, j = hi;
    while (i <= j) {
        while (less(v[i], pivot)) i++;
        while (less(pivot, v[j])) j--;
        if (i <= j) { std::swap(v[i], v[j]); i++; j--; }
    }
    quickSortRec(v, lo, j, less);
    quickSortRec(v, i, hi, less);
}
}  // namespace detail

template <typename T, typename Less>
void quickSort(std::vector<T>& v, Less less) {
    if (v.size() > 1) detail::quickSortRec(v, 0, static_cast<int>(v.size()) - 1, less);
}

// Binary search on a vector sorted by getKey(item). Returns the index, or -1 if not found.
// 'steps' (optional) counts how many comparisons were needed.
template <typename T, typename Key, typename GetKey>
int binarySearch(const std::vector<T>& v, const Key& key, GetKey getKey, int* steps = nullptr) {
    int lo = 0, hi = static_cast<int>(v.size()) - 1, count = 0;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        ++count;
        if (getKey(v[mid]) == key) { if (steps) *steps = count; return mid; }
        if (getKey(v[mid]) < key) lo = mid + 1;
        else hi = mid - 1;
    }
    if (steps) *steps = count;
    return -1;
}

template <typename T, typename Less>
void mergeSort(std::vector<T>& v, Less less) {
    std::vector<T> tmp(v.size());
    detail::mergeSortRec(v, tmp, 0, v.size(), less);
}
}  // namespace sorting