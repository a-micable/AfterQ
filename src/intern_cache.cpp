#include "cairn/intern_cache.h"
#include <cstring>
#include <stdexcept>

namespace cairn {

struct InternEntry {
  char* data = nullptr;
  std::size_t size = 0;
  std::size_t refs = 0;
  bool in_cache = false;
  std::list<InternEntry*>::iterator lru_it;
};

static InternEntry* make_entry(const std::string& text) {
  InternEntry* entry = new InternEntry();
  entry->size = text.size();
  entry->data = new char[entry->size + 1u];
  std::memcpy(entry->data, text.data(), entry->size);
  entry->data[entry->size] = '\0';
  return entry;
}

static void destroy_if_unreferenced(InternEntry* entry) {
  if (entry != nullptr && entry->refs == 0u) {
    delete[] entry->data;
    delete entry;
  }
}

InternedString::InternedString(InternEntry* entry) : entry_(entry) {
  retain();
}

InternedString::InternedString(const InternedString& other) : entry_(other.entry_) {
  retain();
}

InternedString::InternedString(InternedString&& other) noexcept : entry_(other.entry_) {
  other.entry_ = nullptr;
}

InternedString& InternedString::operator=(const InternedString& other) {
  if (this != &other) {
    release();
    entry_ = other.entry_;
    retain();
  }
  return *this;
}

InternedString& InternedString::operator=(InternedString&& other) noexcept {
  if (this != &other) {
    release();
    entry_ = other.entry_;
    other.entry_ = nullptr;
  }
  return *this;
}

InternedString::~InternedString() {
  release();
}

void InternedString::retain() {
  if (entry_ != nullptr) {
    ++entry_->refs;
  }
}

void InternedString::release() {
  InternEntry* old = entry_;
  entry_ = nullptr;
  if (old != nullptr) {
    if (old->refs == 0u) {
      throw std::logic_error("intern refcount underflow");
    }
    --old->refs;
    if (!old->in_cache) {
      destroy_if_unreferenced(old);
    }
  }
}

const char* InternedString::c_str() const {
  return entry_ == nullptr ? "" : entry_->data;
}

std::string InternedString::str() const {
  return std::string(c_str());
}

bool InternedString::empty() const {
  return entry_ == nullptr || entry_->size == 0u;
}

bool InternedString::valid() const {
  return entry_ != nullptr;
}

bool InternedString::operator==(const InternedString& other) const {
  return entry_ == other.entry_;
}

InternCache::InternCache(std::size_t capacity) : capacity_(capacity == 0u ? 1u : capacity) {}

InternCache::~InternCache() {
  for (InternEntry* entry : lru_) {
    entry->in_cache = false;
    if (entry->refs > 0u) {
      --entry->refs;
    }
    destroy_if_unreferenced(entry);
  }
}

InternedString InternCache::intern(const std::string& text) {
  const auto found = map_.find(text);
  if (found != map_.end()) {
    touch(found->second);
    return InternedString(found->second);
  }
  InternEntry* entry = make_entry(text);
  entry->refs = 1u;
  entry->in_cache = true;
  lru_.push_front(entry);
  entry->lru_it = lru_.begin();
  map_.emplace(text, entry);
  evict_if_needed();
  return InternedString(entry);
}

bool InternCache::contains(const std::string& text) const {
  return map_.find(text) != map_.end();
}

InternStats InternCache::stats() const {
  InternStats s;
  s.entries = map_.size();
  s.evictions = evictions_;
  for (const auto& pair : map_) {
    s.live_bytes += pair.first.size() + 1u;
  }
  return s;
}

std::size_t InternCache::capacity() const {
  return capacity_;
}

void InternCache::touch(InternEntry* entry) {
  lru_.erase(entry->lru_it);
  lru_.push_front(entry);
  entry->lru_it = lru_.begin();
}

void InternCache::evict_if_needed() {
  while (map_.size() > capacity_) {
    InternEntry* victim = lru_.back();
    lru_.pop_back();
    map_.erase(std::string(victim->data, victim->size));
    drop_cache_ref(victim);
    ++evictions_;
  }
}

void InternCache::drop_cache_ref(InternEntry* entry) {
  entry->in_cache = false;
  if (entry->refs == 0u) {
    throw std::logic_error("intern cache refcount underflow");
  }
  --entry->refs;
  destroy_if_unreferenced(entry);
}
}
