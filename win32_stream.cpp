#include "input_stream.h"
#include <assert.h>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

struct Payload {
  std::vector<char> Buffer;
};

struct InputStreamImpl {
  std::unordered_map<void *, std::shared_ptr<Payload>> m_map;
  std::mutex m_mutex;

  void Enqueue(void *handle, std::span<const char> data) {
    std::scoped_lock<std::mutex> lock(m_mutex);
    auto found = m_map.find(handle);
    if (found == m_map.end()) {
      auto inserted = m_map.insert({handle, std::make_shared<Payload>()});
      assert(inserted.second);
      found = inserted.first;
    }
    auto begin = found->second->Buffer.size();
    found->second->Buffer.resize(begin + data.size());
    std::copy(data.begin(), data.end(), found->second->Buffer.data() + begin);
  }

  size_t Read(void *handle, std::vector<char> &buf) {
    std::scoped_lock<std::mutex> lock(m_mutex);
    auto found = m_map.find(handle);
    if (found == m_map.end()) {
      return 0;
    }
    auto size = found->second->Buffer.size();
    if (!size) {
      return 0;
    }
    buf.assign(found->second->Buffer.begin(), found->second->Buffer.end());
    found->second->Buffer.clear();
    return size;
  }
};

InputStream::InputStream() : m_impl(new InputStreamImpl) {}
InputStream::~InputStream() { delete m_impl; }

void InputStream::Register(void *handle) {}
void InputStream::Unregister(void *handle) {}
void InputStream::Poll() {}
void InputStream::Enqueue(void *handle, std::span<const char> data) {
  m_impl->Enqueue(handle, data);
}
size_t InputStream::Read(void *handle, std::vector<char> &buf) {
  return m_impl->Read(handle, buf);
}
