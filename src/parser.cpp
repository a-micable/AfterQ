#include "cairn/parser.h"
#include "cairn/crc32.h"
#include <cstring>
#include <stdexcept>

namespace cairn {
namespace {
constexpr std::size_t kMaxNestedManifestDepth = 64u;

class Reader {
 public:
  Reader(const uint8_t* data, std::size_t size) : data_(data), size_(size) {}
  bool eof() const { return pos_ == size_; }
  std::size_t remaining() const { return size_ - pos_; }
  uint8_t u8() {
    need(1);
    return data_[pos_++];
  }
  uint16_t u16() {
    need(2);
    uint16_t v = static_cast<uint16_t>(data_[pos_]) | (static_cast<uint16_t>(data_[pos_ + 1u]) << 8u);
    pos_ += 2u;
    return v;
  }
  uint32_t u32() {
    need(4);
    uint32_t v = static_cast<uint32_t>(data_[pos_]) |
                 (static_cast<uint32_t>(data_[pos_ + 1u]) << 8u) |
                 (static_cast<uint32_t>(data_[pos_ + 2u]) << 16u) |
                 (static_cast<uint32_t>(data_[pos_ + 3u]) << 24u);
    pos_ += 4u;
    return v;
  }
  int64_t i64() {
    need(8);
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) {
      v |= static_cast<uint64_t>(data_[pos_ + static_cast<std::size_t>(i)]) << (8u * static_cast<unsigned>(i));
    }
    pos_ += 8u;
    return static_cast<int64_t>(v);
  }
  double f64() {
    uint64_t bits = static_cast<uint64_t>(i64());
    double d = 0.0;
    std::memcpy(&d, &bits, sizeof(double));
    return d;
  }
  std::string str() {
    const uint32_t len = u32();
    need(len);
    std::string out(reinterpret_cast<const char*>(data_ + pos_), len);
    pos_ += len;
    return out;
  }
  std::vector<uint8_t> bytes(uint32_t len) {
    need(len);
    std::vector<uint8_t> out(data_ + pos_, data_ + pos_ + len);
    pos_ += len;
    return out;
  }

 private:
  const uint8_t* data_;
  std::size_t size_;
  std::size_t pos_ = 0;
  void need(std::size_t n) {
    if (n > remaining()) {
      throw std::runtime_error("truncated manifest");
    }
  }
};

Value read_value(Reader& r, InternCache& cache) {
  Value v;
  v.type = static_cast<ValueType>(r.u8());
  switch (v.type) {
    case ValueType::String:
      v.data = cache.intern(r.str());
      break;
    case ValueType::Integer:
      v.data = r.i64();
      break;
    case ValueType::Boolean:
      v.data = r.u8() != 0u;
      break;
    case ValueType::FloatArray: {
      const uint32_t count = r.u32();
      std::vector<double> values;
      values.reserve(count);
      for (uint32_t i = 0; i < count; ++i) {
        values.push_back(r.f64());
      }
      v.data = values;
      break;
    }
    default:
      throw std::runtime_error("invalid value type");
  }
  return v;
}

void require_payload_done(Reader& payload) {
  if (!payload.eof()) {
    throw std::runtime_error("record payload has trailing bytes");
  }
}
}

Parser::Parser(InternCache& cache) : cache_(cache) {}

ParseResult Parser::parse(const std::vector<uint8_t>& bytes, bool verify_crc) {
  return parse(bytes, verify_crc, 0u);
}

ParseResult Parser::parse(const std::vector<uint8_t>& bytes, bool verify_crc, std::size_t depth) {
  if (depth > kMaxNestedManifestDepth) {
    throw std::runtime_error("nested manifest depth exceeded");
  }
  Reader top(bytes.data(), bytes.size());
  ParseResult result;
  auto current = std::make_shared<ManifestNode>();
  current->section = cache_.intern("root");
  result.root = current;
  result.sections.emplace("root", current);

  while (!top.eof()) {
    if (top.remaining() < 10u) {
      throw std::runtime_error("truncated record header");
    }
    const RecordType type = static_cast<RecordType>(top.u16());
    const uint32_t len = top.u32();
    const std::vector<uint8_t> payload_bytes = top.bytes(len);
    const uint32_t expected = top.u32();
    if (verify_crc && crc32(payload_bytes) != expected) {
      throw std::runtime_error("crc32 mismatch");
    }
    Reader payload(payload_bytes.data(), payload_bytes.size());
    switch (type) {
      case RecordType::Section: {
        const std::string name = payload.str();
        require_payload_done(payload);
        current = std::make_shared<ManifestNode>();
        current->section = cache_.intern(name);
        result.sections[name] = current;
        if (name == "root") {
          result.root = current;
        }
        break;
      }
      case RecordType::Field: {
        Field field;
        field.key = cache_.intern(payload.str());
        field.value = read_value(payload, cache_);
        require_payload_done(payload);
        current->fields.push_back(field);
        break;
      }
      case RecordType::Include: {
        const std::string target = payload.str();
        require_payload_done(payload);
        current->includes.push_back(cache_.intern(target));
        break;
      }
      case RecordType::Conditional: {
        ConditionalBlock block;
        block.expression = cache_.intern(payload.str());
        const uint32_t nested_len = payload.u32();
        const std::vector<uint8_t> nested = payload.bytes(nested_len);
        require_payload_done(payload);
        Parser nested_parser(cache_);
        block.body = nested_parser.parse(nested, verify_crc, depth + 1u).root;
        current->conditionals.push_back(block);
        break;
      }
      case RecordType::Schema: {
        SchemaDecl schema;
        schema.section = cache_.intern(payload.str());
        const uint32_t count = payload.u32();
        for (uint32_t i = 0; i < count; ++i) {
          SchemaField sf;
          sf.key = cache_.intern(payload.str());
          sf.type = static_cast<ValueType>(payload.u8());
          sf.required = payload.u8() != 0u;
          schema.fields.push_back(sf);
        }
        require_payload_done(payload);
        current->schemas.push_back(schema);
        break;
      }
      case RecordType::Dependency: {
        DependencyEdge edge;
        edge.before = cache_.intern(payload.str());
        edge.after = cache_.intern(payload.str());
        require_payload_done(payload);
        current->dependencies.push_back(edge);
        break;
      }
      default:
        throw std::runtime_error("unknown record type");
    }
  }
  return result;
}
}
