#include <sys/eventfd.h>
#include <unistd.h>
#include <atomic> // For std::atomic
#include <functional>
#include <iostream>
#include <memory> // For std::unique_ptr
#include <thread>

// Worker thread function (remains mostly the same)
void worker_thread(int efd) {
  uint64_t ptr_as_uint;
  while (true) {
    ssize_t s = read(efd, &ptr_as_uint, sizeof(uint64_t));
    if (s == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) { continue; }
      else {
        perror("read");
        break;
      }
    }
    else if (s != sizeof(uint64_t)) {
      std::cerr << "Incomplete read from eventfd" << std::endl;
      break;
    }

    if (ptr_as_uint == 0) {
      std::cout << "Worker thread: Received termination signal." << std::endl;
      break;
    }

    // Reinterpret the uint64_t back to a std::function<void()>*
    // and create a unique_ptr to manage its lifetime.
    std::unique_ptr<std::function<void()>> func_ptr(
      reinterpret_cast<std::function<void()>*>(ptr_as_uint));

    if (func_ptr) {
      std::cout << "Worker thread: Executing function from heap." << std::endl;
      (*func_ptr)(); // Execute the lambda
      // The unique_ptr will automatically call delete on the underlying
      // function* when it goes out of scope, releasing the memory.
    }
    else {
      std::cerr << "Worker thread: Received null function pointer."
                << std::endl;
    }
  }
}

// Function to take a lambda by rvalue reference, move it into a unique_ptr, and
// return it
std::unique_ptr<std::function<void()>> create_task_unique_ptr(
  std::function<void()>&& task) {
  return std::make_unique<std::function<void()>>(std::move(task));
}

int main() {
  int efd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
  if (efd == -1) {
    perror("eventfd");
    return EXIT_FAILURE;
  }

  std::thread worker(worker_thread, efd);

  // Create a capturing lambda
  int captured_val = 123;
  auto my_capturing_lambda = [captured_val]() {
    std::cout << "Capturing lambda executed! Captured value: " << captured_val
              << std::endl;
  };

  // Pass the lambda as an rvalue reference to create_task_unique_ptr
  // The lambda (which is a temporary or moved) is implicitly converted to
  // std::function&& and then moved into the std::unique_ptr
  std::unique_ptr<std::function<void()>> task_unique_ptr =
    create_task_unique_ptr(std::move(my_capturing_lambda));

  // Release ownership to pass the raw pointer via eventfd
  uint64_t ptr_as_uint = reinterpret_cast<uint64_t>(task_unique_ptr.release());

  std::cout << "Main thread: Sending function pointer to worker." << std::endl;
  if (write(efd, &ptr_as_uint, sizeof(uint64_t)) != sizeof(uint64_t)) {
    perror("write");
  }

  std::this_thread::sleep_for(
    std::chrono::milliseconds(100)); // Give time for worker to process

  // Another example, directly creating and passing
  int another_val = 456;
  uint64_t another_ptr_as_uint = reinterpret_cast<uint64_t>(
    std::make_unique<std::function<void()>>([another_val]() {
      std::cout << "Another capturing lambda executed! Captured value: "
                << another_val << std::endl;
    }).release() // Release ownership for passing
  );

  std::cout << "Main thread: Sending another function pointer to worker."
            << std::endl;
  if (write(efd, &another_ptr_as_uint, sizeof(uint64_t)) != sizeof(uint64_t)) {
    perror("write another_ptr");
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Signal the worker thread to terminate
  uint64_t terminate_signal = 0;
  std::cout << "Main thread: Sending termination signal." << std::endl;
  if (write(efd, &terminate_signal, sizeof(uint64_t)) != sizeof(uint64_t)) {
    perror("write terminate_signal");
  }

  worker.join();
  close(efd);

  std::cout << "Main thread: Exiting." << std::endl;

  return 0;
}
