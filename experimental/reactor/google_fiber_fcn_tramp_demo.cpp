#include <ucontext.h>
#include <functional>
#include <iostream>

// A simple struct to demonstrate a non-const object
struct MyData {
  int value;
};

// The C-style wrapper function for makecontext
void wrapper_function(int func_ptr_high,
                      int func_ptr_low,
                      int data_ptr_high,
                      int data_ptr_low) {
  // Reconstruct the pointers
  std::function<void(MyData&)>* func_ptr =
    reinterpret_cast<std::function<void(MyData&)>*>(
      (static_cast<long long>(func_ptr_high) << 32) | func_ptr_low);
  MyData* data_ptr =
    reinterpret_cast<MyData*>((static_cast<long long>(data_ptr_high) << 32)
                              | data_ptr_low);

  // Call the std::function with the non-const reference
  (*func_ptr)(*data_ptr);
}

int main() {
  ucontext_t uc_main, uc_child;

  // Initialize the std::function
  std::function<void(MyData&)> my_func = [](MyData& data) {
    std::cout << "Inside std::function. Original value: " << data.value
              << std::endl;
    data.value = 42; // Modify the non-const object
    std::cout << "Inside std::function. Modified value: " << data.value
              << std::endl;
  };

  // Initialize the non-const object
  MyData my_data = {10};

  // Get the address of the std::function object and the MyData object
  long long func_addr = reinterpret_cast<long long>(&my_func);
  long long data_addr = reinterpret_cast<long long>(&my_data);

  // Split addresses into high and low parts for makecontext
  int func_high = static_cast<int>(func_addr >> 32);
  int func_low = static_cast<int>(func_addr & 0xFFFFFFFF);
  int data_high = static_cast<int>(data_addr >> 32);
  int data_low = static_cast<int>(data_addr & 0xFFFFFFFF);

  // Allocate stack for the child context
  char child_stack[SIGSTKSZ];

  // Initialize and prepare the child context
  getcontext(&uc_child);
  uc_child.uc_stack.ss_sp = child_stack;
  uc_child.uc_stack.ss_size = sizeof(child_stack);
  uc_child.uc_link = &uc_main; // Link back to main context
  makecontext(&uc_child, reinterpret_cast<void (*)()>(wrapper_function), 4,
              func_high, func_low, data_high, data_low);

  std::cout << "Before swapcontext. MyData value: " << my_data.value
            << std::endl;

  // Swap to the child context
  swapcontext(&uc_main, &uc_child);

  std::cout << "After swapcontext. MyData value: " << my_data.value
            << std::endl;

  return 0;
}
