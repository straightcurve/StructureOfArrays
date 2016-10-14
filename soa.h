#pragma once

#include <cassert>
#include <type_traits>
#include <vector>

// This class makes heavy use of template parameter pack.
// http://en.cppreference.com/w/cpp/language/parameter_pack

#define FOR_EACH_ARRAY(expression)                                          \
  size_t array_index = 0;                                                   \
  int dummy[] = {                                                           \
      (get_array<Elements>(array_index)->expression, ++array_index, 0)...}; \
  (void)dummy

// Structure of Arrays.
template <typename... Elements>
class SoA {
 public:
  SoA() {
    size_t array_index = 0;
    int dummy_init[] = {(new (get_array<Elements>(array_index))
                             std::vector<Elements>(),
                         ++array_index, 0)...};
    (void)dummy_init;  // avoids unused variable compiler warnings.
  }

  virtual ~SoA() { FOR_EACH_ARRAY(~vector<Elements>()); }

  // Returns the number of elements in the arrays.
  size_t size() const { return size_; }

  // Returns |true| if there are no elements in the arrays, |false| otherwise.
  bool empty() const { return (size_ == 0); }

  // Adds an element to the end of the arrays via rvalue.
  void push_back(Elements &&... elements) {
    FOR_EACH_ARRAY(push_back(std::forward<Elements>(elements)));

    // Increment number of elements in the arrays.
    ++size_;
  }

  // Adds an element to the end of the arrays, as a copy of the const ref value.
  void push_back(const Elements &... elements) {
    FOR_EACH_ARRAY(push_back(elements));

    // Increment number of elements in the arrays.
    ++size_;
  }

  // Erases the last element in the arrays.
  void pop_back() {
    if (empty()) {
      return;
    }

    FOR_EACH_ARRAY(pop_back());

    // Decrement number of elements in the arrays.
    --size_;
  }

  // Erases elements of a given index from the arrays.
  void erase(size_t element_index) {
    if (element_index >= size()) {
      assert(false);
      return;
    }

    size_t array_index = 0;
    int dummy[] = {(erase_impl<Elements>(element_index, array_index++), 0)...};
    (void)dummy;

    // Decrement number of elements in the arrays.
    --size_;
  }

  // Erases |num_elements| elements from |start_index| from the arrays.
  void erase(size_t start_index, size_t num_elements) {
    if (start_index >= size() || (start_index + num_elements) > size()) {
      assert(false);

      return;
    }

    size_t array_index = 0;
    int dummy[] = {
        (erase_impl<Elements>(start_index, num_elements, array_index++), 0)...};
    (void)dummy;

    // Decrement number of elements in the arrays.
    size_ -= num_elements;
  }

  // Swaps two elements inside the array.
  void swap(size_t index0, size_t index1) {
    if (index0 >= size() || index1 > size()) {
      assert(false);

      return;
    }

    size_t array_index = 0;
    int dummy[] = {(swap_impl<Elements>(index0, index1, array_index++), 0)...};
    (void)dummy;
  }

  // Reserves enough memories in all arrays to contain |reserve_size| elements.
  void reserve(size_t reserve_size) { FOR_EACH_ARRAY(reserve(reserve_size)); }

  // Resizes the arrays to contain |size| elements;
  void resize(size_t size) {
    FOR_EACH_ARRAY(resize(size));

    size_ = size;
  }

  // Returns a pointer to the |ArrayIndex|th array.
  template <typename ElementType, std::size_t ArrayIndex>
  ElementType *array() {
    static_assert(ArrayIndex < kNumArrays, "Requested invalid array index.");

    std::vector<ElementType> *array = get_array<ElementType>(ArrayIndex);
    return array->data();
  }

  // Returns a const pointer to the |ArrayIndex|th array.
  template <typename ElementType, std::size_t ArrayIndex>
  const ElementType *array() const {
    static_assert(ArrayIndex < kNumArrays, "Requested invalid array index.");

    const std::vector<ElementType> *array = get_array<ElementType>(ArrayIndex);
    return array->data();
  }

  // Returns a reference to the |index|th element from the |ArrayIndex|th array
  // as type |ElementType|.
  template <typename ElementType, std::size_t ArrayIndex>
  ElementType &get(size_t index) {
    static_assert(ArrayIndex < kNumArrays,
                  "Requested invalid array index in get().");

    std::vector<ElementType> *array = get_array<ElementType>(ArrayIndex);
    return (*array)[index];
  }

  // Returns a const reference to the |index|th element from the |ArrayIndex|th
  // array as type |ElementType|.
  template <typename ElementType, std::size_t ArrayIndex>
  const ElementType &get(size_t index) const {
    static_assert(ArrayIndex < kNumArrays,
                  "Requested invalid array index in get().");

    const std::vector<ElementType> *array = get_array<ElementType>(ArrayIndex);
    return (*array)[index];
  }

  // Returns the number of arrays.
  size_t num_arrays() const { return kNumArrays; }

 private:
  template <class Type>
  std::vector<Type> *get_array(size_t array_index) {
    static_assert(sizeof(std::vector<Type>) == sizeof(std::vector<void *>),
                  "Structure of Array assumes of vector<Type> is same size as "
                  "vector<void*> however the assumption failed.");
    static_assert(alignof(std::vector<Type>) == alignof(std::vector<void *>),
                  "Structure of Array assumes of vector<Type> is same "
                  "alignment as vector<void*> however the assumption failed.");

    return reinterpret_cast<std::vector<Type> *>(&arrays_[array_index]);
  }

  template <class Type>
  const std::vector<Type> *get_array(size_t array_index) const {
    static_assert(sizeof(std::vector<Type>) == sizeof(std::vector<void *>),
                  "Structure of Array assumes of vector<Type> is same size as "
                  "vector<void*> however the assumption failed.");
    static_assert(alignof(std::vector<Type>) == alignof(std::vector<void *>),
                  "Structure of Array assumes of vector<Type> is same "
                  "alignment as vector<void*> however the assumption failed.");

    return reinterpret_cast<const std::vector<Type> *>(&arrays_[array_index]);
  }

  template <class Type>
  void erase_impl(size_t element_index, size_t array_index) {
    std::vector<Type> *array = get_array<Type>(array_index);

    array->erase(array->begin() + element_index);
  }

  template <class Type>
  void erase_impl(size_t element_index, size_t num_elements,
                  size_t array_index) {
    std::vector<Type> *array = get_array<Type>(array_index);

    array->erase(array->begin() + element_index,
                 array->begin() + element_index + num_elements);
  }

  template <class Type>
  void swap_impl(size_t index0, size_t index1, size_t array_index) {
    std::vector<Type> *array = get_array<Type>(array_index);

    using std::swap;
    swap((*array)[index0], (*array)[index1]);
  }

  SoA(const SoA &) = delete;
  SoA &operator=(const SoA &) = delete;

  using ArrayType = std::aligned_storage<sizeof(std::vector<void *>),
                                         alignof(std::vector<void *>)>::type;
  static const size_t kNumArrays = sizeof...(Elements);
  size_t size_ = 0;
  ArrayType arrays_[kNumArrays];
};
