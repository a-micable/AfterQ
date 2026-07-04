#include "cairn/manifest_writer.h"
#include "cairn/crc32.h"
#include <cstring>

namespace cairn {
namespace {
void put_u8(std::vector<uint8_t>& out, uint8_t v) { out.push_back(v); }
void put_u16(std::vector<uint8_t>& out, uint16_t v) {
  out.push_back(static_cast<uint8_t>(v & 0xffu));
  out.push_back(static_cast<uint8_t>((v >> 8u) & 0xffu));
}
void put_u32(std::vector<uint8_t>& out, uint32_t v) {
  out.push_back(static_cast<uint8_t>(v & 0xffu));
  out.push_back(static_cast<uint8_t>((v >> 8u) & 0xffu));
  out.push_back(static_cast<uint8_t>((v >> 16u) & 0xffu));
  out.push_back(static_cast<uint8_t>((v >> 24u) & 0xffu));
}
void put_i64(std::vector<uint8_t>& out, int64_t value) {
  const uint64_t v = static_cast<uint64_t>(value);
  for (int i = 0; i < 8; ++i) {
    out.push_back(static_cast<uint8_t>((v >> (8u * static_cast<unsigned>(i))) & 0xffu));
  }
}
void put_f64(std::vector<uint8_t>& out, double value) {
  uint64_t bits = 0;
  std::memcpy(&bits, &value, sizeof(double));
  put_i64(out, static_cast<int64_t>(bits));
}
void put_string(std::vector<uint8_t>& out, const std::string& text) {
  put_u32(out, static_cast<uint32_t>(text.size()));
  out.insert(out.end(), text.begin(), text.end());
}
}

void ManifestWriter::record(RecordType type, const std::vector<uint8_t>& payload) {
  put_u16(out_, static_cast<uint16_t>(type));
  put_u32(out_, static_cast<uint32_t>(payload.size()));
  out_.insert(out_.end(), payload.begin(), payload.end());
  put_u32(out_, crc32(payload));
}

void ManifestWriter::section(const std::string& name) {
  std::vector<uint8_t> p;
  put_string(p, name);
  record(RecordType::Section, p);
}

void ManifestWriter::field_string(const std::string& key, const std::string& value) {
  std::vector<uint8_t> p;
  put_string(p, key);
  put_u8(p, static_cast<uint8_t>(ValueType::String));
  put_string(p, value);
  record(RecordType::Field, p);
}

void ManifestWriter::field_integer(const std::string& key, int64_t value) {
  std::vector<uint8_t> p;
  put_string(p, key);
  put_u8(p, static_cast<uint8_t>(ValueType::Integer));
  put_i64(p, value);
  record(RecordType::Field, p);
}

void ManifestWriter::field_boolean(const std::string& key, bool value) {
  std::vector<uint8_t> p;
  put_string(p, key);
  put_u8(p, static_cast<uint8_t>(ValueType::Boolean));
  put_u8(p, value ? 1u : 0u);
  record(RecordType::Field, p);
}

void ManifestWriter::field_float_array(const std::string& key, const std::vector<double>& values) {
  std::vector<uint8_t> p;
  put_string(p, key);
  put_u8(p, static_cast<uint8_t>(ValueType::FloatArray));
  put_u32(p, static_cast<uint32_t>(values.size()));
  for (double value : values) {
    put_f64(p, value);
  }
  record(RecordType::Field, p);
}

void ManifestWriter::include(const std::string& target) {
  std::vector<uint8_t> p;
  put_string(p, target);
  record(RecordType::Include, p);
}

void ManifestWriter::conditional(const std::string& expression, const std::vector<uint8_t>& nested_manifest) {
  std::vector<uint8_t> p;
  put_string(p, expression);
  put_u32(p, static_cast<uint32_t>(nested_manifest.size()));
  p.insert(p.end(), nested_manifest.begin(), nested_manifest.end());
  record(RecordType::Conditional, p);
}

void ManifestWriter::schema(const std::string& section_name, const std::vector<SchemaField>& fields) {
  std::vector<uint8_t> p;
  put_string(p, section_name);
  put_u32(p, static_cast<uint32_t>(fields.size()));
  for (const SchemaField& field : fields) {
    put_string(p, field.key.str());
    put_u8(p, static_cast<uint8_t>(field.type));
    put_u8(p, field.required ? 1u : 0u);
  }
  record(RecordType::Schema, p);
}

void ManifestWriter::dependency(const std::string& before, const std::string& after) {
  std::vector<uint8_t> p;
  put_string(p, before);
  put_string(p, after);
  record(RecordType::Dependency, p);
}

const std::vector<uint8_t>& ManifestWriter::bytes() const {
  return out_;
}

std::vector<uint8_t> make_minimal_manifest(const std::string& section_name) {
  ManifestWriter w;
  w.section(section_name);
  return w.bytes();
}
}
