#pragma once

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include "shared_queue.h"

namespace fifo {
// Function to read from FIFO
void ReadFromFIFO(std::string_view fifo_path,
                  std::shared_ptr<util::SharedQueue<std::string>> data_queue) {
  // Create the FIFO, remove if it already exists.
  if (std::filesystem::exists(fifo_path)) {
    std::filesystem::remove(fifo_path);
  }
  if (mkfifo(fifo_path.data(), 0666) == -1) {
    perror("mkfifo");
  }

  int fd = open(fifo_path.data(), O_RDONLY | O_NONBLOCK);
  if (fd == -1) {
    perror("open");
    return;
  }

  char buffer[1024];
  while (true) {
    ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
    if (bytesRead > 0) {
      buffer[bytesRead] = '\0';
      std::string data(buffer);

      // Add data to the queue
      data_queue->Enqueue(data);
    } else if (bytesRead == 0) {
      // End of file
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } else {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        perror("read");
        break;
      }
    }
  }

  close(fd);
}

// Class handling the write to FIFO.
class FIFOWriter {
 public:
  using WriteFn = std::function<void(const std::string&)>;

  FIFOWriter(std::string_view fifo_path) : fifo_path_(fifo_path) {
    if (std::filesystem::exists(fifo_path.data())) {
      std::filesystem::remove(fifo_path.data());
    }
    if (mkfifo(fifo_path.data(), 0666) == -1) {
      perror("mkfifo");
    }
  }

  // Opens and writes data to the FIFO.
  // TODO: Return status if the write failed.
  void Write(const std::string& data) {
    std::cout << "Opening " << fifo_path_.data() << std::endl;
    fd_ = open(fifo_path_.data(), O_WRONLY);
    if (fd_ == -1) {
      perror("open");
    }
    ssize_t bytesWritten = write(fd_, data.data(), data.size());
    if (bytesWritten == -1) {
      perror("write");
    }
  }

  // Retrieves the write function as a callable.
  WriteFn GetWriteFn() {
    return [this](const std::string& data) { Write(data); };
  }

 private:
  std::string_view fifo_path_;
  int fd_;
};
}  // namespace fifo
