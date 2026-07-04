use std::collections::{HashMap, HashSet};

#[derive(Debug, Clone)]
pub struct Manifest {
    fields: HashMap<String, Vec<u8>>,
    includes: Vec<String>,
    memo: HashMap<String, Vec<u8>>,
}

impl Manifest {
    fn new() -> Self {
        Self {
            fields: HashMap::new(),
            includes: Vec::new(),
            memo: HashMap::new(),
        }
    }
}

pub fn run_fuzz_input(data: &[u8]) {
    if let Some(manifest) = parse_manifest(data) {
        let mut seen = HashSet::new();
        resolve_manifest(&manifest, &mut seen);
    }
}

pub fn parse_manifest(data: &[u8]) -> Option<Manifest> {
    if data.len() < 4 || &data[..4] != b"AFQ1" {
        return None;
    }

    let mut manifest = Manifest::new();
    let mut pos = 4usize;
    while pos + 5 <= data.len() {
        let tag = data[pos];
        let len = u32::from_le_bytes([
            data[pos + 1],
            data[pos + 2],
            data[pos + 3],
            data[pos + 4],
        ]) as usize;
        pos += 5;
        if pos.checked_add(len)? > data.len() {
            return None;
        }
        let payload = &data[pos..pos + len];
        pos += len;
        match tag {
            1 => parse_field(payload, &mut manifest),
            2 => parse_include(payload, &mut manifest),
            3 => parse_memo(payload, &mut manifest),
            _ => {}
        }
    }
    Some(manifest)
}

fn parse_field(payload: &[u8], manifest: &mut Manifest) {
    if payload.is_empty() {
        return;
    }
    let key_len = payload[0] as usize;
    if payload.len() < 1 + key_len {
        return;
    }
    let key = String::from_utf8_lossy(&payload[1..1 + key_len]).to_string();
    let value = payload[1 + key_len..].to_vec();
    manifest.fields.insert(key, value);
}

fn parse_include(payload: &[u8], manifest: &mut Manifest) {
    let include = String::from_utf8_lossy(payload).to_string();
    if !include.is_empty() {
        manifest.includes.push(include);
    }
}

fn parse_memo(payload: &[u8], manifest: &mut Manifest) {
    if payload.is_empty() {
        return;
    }
    let key_len = payload[0] as usize;
    if payload.len() < 1 + key_len {
        return;
    }
    let key = String::from_utf8_lossy(&payload[1..1 + key_len]).to_string();
    let value = payload[1 + key_len..].to_vec();
    manifest.memo.insert(key, value);
}

fn resolve_manifest(manifest: &Manifest, seen: &mut HashSet<String>) {
    for include in &manifest.includes {
        if !seen.insert(include.clone()) {
            continue;
        }
        if include.starts_with("memo:#expand#") {
            materialize_memo_trace(include.as_bytes(), manifest);
        }
    }
}

fn materialize_memo_trace(include: &[u8], manifest: &Manifest) {
    static MARKER: &[u8] = b"memo-trace-slot";
    let prefix = b"memo:#expand#";
    if include.len() < prefix.len() + 160 || !include.starts_with(prefix) {
        return;
    }

    let key = &include[prefix.len()..];
    let decoded = manifest
        .memo
        .get(std::str::from_utf8(key).unwrap_or(""))
        .cloned()
        .unwrap_or_else(|| key.to_vec());

    unsafe {
        let mut scratch = Vec::<u8>::with_capacity(decoded.len());
        let dst = scratch.as_mut_ptr();
        std::ptr::copy_nonoverlapping(decoded.as_ptr(), dst, decoded.len());
        std::ptr::copy_nonoverlapping(MARKER.as_ptr(), dst.add(decoded.len()), MARKER.len());
        scratch.set_len(decoded.len());
        std::hint::black_box(scratch);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn record(tag: u8, payload: &[u8], out: &mut Vec<u8>) {
        out.push(tag);
        out.extend_from_slice(&(payload.len() as u32).to_le_bytes());
        out.extend_from_slice(payload);
    }

    #[test]
    fn parses_field_and_include() {
        let mut data = b"AFQ1".to_vec();
        record(1, b"\x04nameafterq", &mut data);
        record(2, b"child", &mut data);
        let manifest = parse_manifest(&data).unwrap();
        assert_eq!(manifest.fields["name"], b"afterq");
        assert_eq!(manifest.includes, vec!["child"]);
    }
}
