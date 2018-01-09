#pragma once

#include <assert.h>
#include <errno.h>
#include <exception>
#include <sstream>
#include <string>
#include <string.h>
#include "sysconfig.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>

class MSR {
private:
  std::vector<int> fds;

public:
  MSR(bool write = true) {
    int numCores = getNumCores();
    assert(numCores > 0);

    fds.clear();
    for (int i = 0; i < numCores; i++) {
      std::stringstream ss;
      ss << "/dev/cpu/" << i << "/msr";
      int flags = write ? O_RDWR : O_RDONLY;
      int fd = open(ss.str().c_str(), flags);
      if (fd == -1)
        throw FileIOException("Error opening " + ss.str());
      fds.push_back(fd);
    }
  }

  ~MSR() {
    for (int fd : fds)
      close(fd);
    fds.clear();
  }

  uint64_t read(int core, ssize_t msr) const {
    int fd = fds[core];
    uint64_t val;
    ssize_t nbytes = pread(fd, &val, sizeof(val), msr);
    if (nbytes == -1) {
      std::stringstream msg;
      msg << "Error reading from msr 0x" << std::hex << msr;
      msg << " | core " << core << std::endl;
      msg << "errno : " << strerror(errno);
      throw FileIOException(msg.str());
    }
    return val;
  }

  void write(int core, ssize_t msr, uint64_t val) const {
    int fd = fds[core];
    uint64_t prevVal = read(core, msr);
    ssize_t nbytes = pwrite(fd, &val, sizeof(val), msr);
    if (nbytes == -1) {
      std::stringstream msg;
      msg << "Error writing to msr 0x" << std::hex << msr;
      msg << " | Core " << core;
      msg << " | original = 0x" << std::hex << prevVal;
      msg << " | new = 0x" << std::hex << val << std::endl;
      msg << "errno : " << strerror(errno);
      throw FileIOException(msg.str());
    }
  }

  class FileIOException : public std::exception {
  private:
    std::string msg;

  public:
    FileIOException(std::string msg) : msg(msg) {}
    ;

    ~FileIOException() throw() {}
    ;

    virtual const char *what() const throw() {
      std::string err = "msr file io error\n" + msg;
      return err.c_str();
    }
  };
};
