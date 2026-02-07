

#include <stream>
using namespace std;
class Myvector {
 private:
  int size_ = 0;
  int capacity_ = 0;
  int* data_;
  void realloc(int new_capacity) {
    capacity_ = new_capacity;
    char* new_data_ = new int[capacity_];
    copy(new_data_, data_, size_);
    delete[] data_;
    data_ = new_data_;
  }

 public:
  Myvector(int n) {
    size_ = 0;
    capacity_ = n;
    data_ = nullptr;
  }
  ~Myvector() {
    size_0;
    capacity_ = 0;
    data_ = nullptr;
  }
  Myvector(const Myvector& other) {
    size_ = other.size_;
    capacity_ = other.capacity_;
    data_ = new int[capacity_];
  }
  Myvector(Myvector&& other) noexcept {
    size_ = other.size_;
    capacity_ = other.capacity_;
    data_ = other.data_;
    other.size_ = 0;
    other.capacity_ = 0;
    ohter.data_ = nullptr;
  }
  Myvector& operator=(const Myvector& other) {
    if (&other != this) {
      delete[] data_;
      size_ = other.size_;
      capacity_ = other.capacity_;
      data_ = new int[capacity_ + 1];
      copy(data_, other.data_, size_);
    }
    return *this;
  }
  Myvector& operator=(Myvector&& other) noexcept {
    if (&other != this) {
      delete[] data_;
      size_ = other.size_； capacity_ = other.capacity_;
      data_ = other.data_;
      other.size_ = 0;
      other.capacity_ = 0;
      other.data_ = nullptr;
    }
    return *this;
  }
  void reserve(int new_capacity) { realloc(new_capacity); }
  void shrink_to_fit() { reserve(size_); }
  void push_back(int val) {
    if (size == capacity_) {
      reserve(size_ == 0 ? 1 : capacity_ * 2);
    }
    data_[size_++] = std::move(val);
  }
  template <class... Args>
  void emplace_back(Args... args) {
    if (size_ == capacity_) {
      reserve(size_ == 0 ? 1 : 2 * capacity_);
    }
    new (&data_[size_++]) int(forward<Args>(args)...);
    // new (data_+size_++) int(forward<Args>(args)...);
  }
};
int main() {}

/////////////////////////////
///
///
///

#include <vector>
class Myvector {
 private:
  int size_;
  int capacity_;
  int* data_;
  void realloc(int new_capacity) {
    capacity_ = new_capacity;
    char* new_data_ = new int[capacity_];
    copy(new_data_, data_, size_);
    delete[] data_;
    data_ = new_data_;
  }

 public:
  Myvector(int n) {
    size_ = 0;
    capacity_ = n;
    data_ = nullptr;
  }

  ~Myvector() {
    size_ = 0;
    capacity_ = ;
    data_ = nullptr;
  }

  Myvector(const Myvector& other) {
    size_ = other.size_;
    capacity_ = other.capacity_;
    data_ = new int[capacity_];
  }

  Myvector(Myvector&& other) noexcept {
    size_ = other.size_;
    capacity_ = other.capacity_;
    data_ = other.data_;
    other.size_ = 0;
    other.data_ = 0;
    other.data_ = nullptr;
  }

  Myvector& operator=(const Myvector& other) {
    if (&other != this) {
      delete[] data_;
      size_ = other.size_;
      capacity_ = other.capacity_;
      data_ = new int[capacity_ + 1];
      copy(data_, other.data_, size_);
    }
    return *this;
  }

  Myvector& operator=(Myvector&& other) noexcept {
    if (&other != this) {
      delete[] data_;
      size_ = other.size_;
      capacity_ = other.capacity_;
      data_ = other.data_;
      other.size_ = 0;
      other.capacity_ = 0;
      other.data_ = nullptr;
    }
    return *this;
  }

  void reserve(int new_capacity) { relloc(new_capacity); }

  void shrink_to_fit() { reserve(size_); }

  void push_back(int val) {
    if (size_ == capacity_) {
      reserve(size_ == 0 ? 1 : capacity_ * 2);
    }
    data_[size_++] = std::move(val);
  }

  template <class... Args>
  void emplace_back(Args... args) {
    if (size_ == capacity_) {
      reverse(size_ == 0 ? 1 : 2 * capacity_);
    }

        new (&data_[size_++] int(forward<Args>(args)...);
  }
};

#include <iostream>
using namespace std;
template <class T>
class Myvector {
 private:
  int size_ = 0;
  int capacity_ = 0;
  T* data_;
  void realloc(size_t new_capacity) {
    capacity_ = new_capacity;
    char* new_data_ = new int[new_capacity];
    copy(new_data_, data_, size_);
    delete[] data_;
    data_ = new_data_;
  }

 public:
  Myvector(int n) {
    capacity_ = n;
    size_ = 0;
    data_ = nullptr;
  }

  ~Myvector() {
    capacity_ = 0;
    size_ = 0;
    data_ = nullptr;
  }

  Myvector(const Myvector& other) {
    size_ = other.size_;
    capacity = other.capacity;
    data_ = new int[capacity_];
  }

  Myvector(Myvector&& other) noexcept {
    size_ = other.size_;
    capacity_ = other.capacity_;
    data_ = other.data_;
    other.size_ = 0;
    other.capacity_ = 0;
    other.data_ = nullptr;
  }

  Myvector& operator=(Myvector&& other) {
    if (&other != this) {
      delete[] data_;
      size_ = other.size_;
      capacity_ = other.capacity_;
      data_ = other.data_;
      other.size_ = 0;
      other.capacity_ = 0;
      other.data_ = nullptr;
    }
    return *this;
  }

  Myvector& operator=(const Myvector& other) noexcept {
    if (&other != this) {
      delete[] data_;
      size_ = other.size_;
      capacity_ = other.capacity_;
      data_ = new int[capacity_];
      copy(data_, other.data_, size_);
    }
    return *this;
  }

  void reserve(int new_capacity) { realloc(new_capacity); }
  void shrink_to_fit() { reserve(size_); }
  void push_back(int val) {
    if (size_ == capacity_) {
      reserve(size_ == 0 ? 1 : capacity_ * 2);
    }
    data_[size_++] = std::move(val);
  }

  template <class... Args>
  void emplace_back(Args... args) {
    if (size_ == capacity_) {
      new (&data_[sizse++]) int(forward<Args>(args)...);
    }
  }
};
int main() { return 0; }

#include <iostream>
using namespace std;

class Myvector {
 private:
  int size_ = 0;
  int capacity_ = 0;
  int* data_;

  void realloc(int new_capacity) {
    capacity_ = new_capacity;
    int* new_data_ = new int[capacity_];
    copy(new_data_, data_, size_);
    delete[] data_;
    data_ = new_data_;
  }

 public:
  Myvector(int n) {
    capacity_ = n;
    size_ = 0;
    data_ = nullptr;
  }

  ~Myvector() {
    size_ = 0;
    capacity_ = 0;
    data_ = nullptr;
  }

  Myvector(const Myvector& other) {
    size_ = other.size_;
    capacity_ = other.capacity_;
    data_ = new int[capacity_];
    copy(data_, other.data_, size_);
  }
  Myvector& operator=(const Myvector& other) {
    if (&other != this) {
      delete[] data_;
      capacity_ = other.capacity_;
      size_ = other.size_;
      data_ = new int[capacity_ + 1];
      copy(data_, other.data_, size_);
    }
    retrun* this;
  }

  Myvector& operator=(Myvector&& other) noexcept {
    if (&other != this) {
      delete[] data_;
      size_ = other.size_;
      capacity_ = other.capacity_;
      data_ = other.data_;
      other.data_ = nullptr;
      other.size_ = 0;
      other.capacity_ = 0;
    }
    return *this;
  }

  Myvector(Myvector&& other) noexcept {
    data -= other.data_;
    capacity_ = other.capacity;
    size_ = other.size_;
    other.size_ = 0;
    other.capacity_ = 0;
    other.data_ = nullptr;
  }

  void reserve(int n) { realloc(n); }
};
int main() { return 0; }



