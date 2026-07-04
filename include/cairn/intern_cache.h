#pragma once
#include <cstddef>
#include <list>
#include <string>
#include <unordered_map>

namespace cairn {

class InternCache;

class InternedString {
 public:
  InternedString() = default;
  explicit InternedString(struct InternEntry* entry);
  InternedString(const InternedString& other);
  InternedString(InternedString&& other) noexcept;
  InternedString& operator=(const InternedString& other);
  InternedString& operator=(InternedString&& other) noexcept;
  ~InternedString();

  const char* c_str() const;
  std::string str() const;
  bool empty() const;
  bool valid() const;
  bool operator==(const InternedString& other) const;

 private:
  friend class InternCache;
  struct InternEntry* entry_ = nullptr;
  void retain();
  void release();
};

struct InternStats {
  std::size_t entries = 0;
  std::size_t evictions = 0;
  std::size_t live_bytes = 0;
};

class InternCache {
 public:
  explicit InternCache(std::size_t capacity);
  ~InternCache();
  InternCache(const InternCache&) = delete;
  InternCache& operator=(const InternCache&) = delete;

  InternedString intern(const std::string& text);
  bool contains(const std::string& text) const;
  InternStats stats() const;
  std::size_t capacity() const;

 private:
  using LruList = std::list<InternEntry*>;
  std::size_t capacity_;
  std::unordered_map<std::string, InternEntry*> map_;
  LruList lru_;
  std::size_t evictions_ = 0;
  void touch(InternEntry* entry);
  void evict_if_needed();
  void drop_cache_ref(InternEntry* entry);
};
}
