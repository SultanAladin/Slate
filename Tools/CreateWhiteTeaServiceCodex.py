#!/usr/bin/env python3
"""Create the versioned White Tea Service workspace and shared pigment Codex documents."""
from pathlib import Path
import hashlib, struct

ROOT = Path(__file__).resolve().parents[1]
CONTENT = ROOT / "EngineContent"
REVISION = 1

def u16(v): return struct.pack("<H", v)
def u32(v): return struct.pack("<I", v)
def u64(v): return struct.pack("<Q", v)
def f64(v): return struct.pack("<d", v)
def run(v):
    b = v.encode("utf-8")
    return u32(len(b)) + b

def digest(data):
    value = 14695981039346656037
    for byte in data:
        value = ((value ^ byte) * 1099511628211) & 0xffffffffffffffff
    return value

def codex(profile, identity, sections):
    stream = bytearray(64)
    indexed = []
    for code, content in sections:
        while len(stream) % 8: stream.append(0)
        position = len(stream)
        stream += content
        indexed.append((code, position, len(content), digest(content)))
    while len(stream) % 8: stream.append(0)
    index_at = len(stream)
    index_data = bytearray(u32(0x58444953) + u32(len(indexed)))
    for code, position, size, checksum in indexed:
        index_data += u32(code) + u16(1) + u16(0) + u64(REVISION) + u64(position) + u64(size) + u64(checksum)
    stream += index_data
    index_digest = digest(index_data)
    stream += u32(0x54464353) + u32(0) + u64(index_at) + u64(len(index_data)) + u64(index_digest)
    preamble = (u32(0x44434C53) + u16(1) + u16(0) + u32(profile) + u32(64) +
                u64(index_at) + u64(len(index_data)) + u64(identity) + u64(REVISION) + u64(index_digest) + u64(0))
    stream[:64] = preamble
    return bytes(stream)

def scene_entry(subject, name, geometry, material, position, rotation=(0.,0.,0.), scale=(1.,1.,1.)):
    return u32(subject) + run(name) + run(geometry) + run(material) + b"".join(f64(x) for x in position + rotation + scale)

def main():
    pigment = run("White Dielectric") + b"".join(f64(x) for x in (1.,1.,1.,0.32,1.5)) + b"\x01"
    pigment_path = CONTENT / "MaterialArchives" / "WhiteDielectric.pigment"
    pigment_path.write_bytes(codex(1, 0x574449454C454354, [(0x464E4950, pigment)]))

    naming = run("White Tea Service")
    environment = b"".join(f64(x) for x in (35., 120., 4.8, 5500., 1., 1., 1.))
    material = "EngineContent/MaterialArchives/WhiteDielectric.pigment"
    root = "EngineContent/GeometryArchives/WhiteTeaService/"
    entries = [
        scene_entry(0, "Sun", "", "", (0.,0.,0.)),
        scene_entry(1, "Sky", "", "", (0.,0.,0.)),
        scene_entry(2, "Atmosphere", "", "", (0.,0.,0.)),
        scene_entry(3, "Service Teapot", root + "ServiceTeapot.obj", material, (0., 0.035, 0.)),
        scene_entry(3, "Teacup", root + "Teacup.obj", material, (0.31, 0.035, 0.05)),
        scene_entry(3, "Saucer", root + "Saucer.obj", material, (0.31, 0.0, 0.05)),
        scene_entry(3, "Sugar Bowl", root + "SugarBowl.obj", material, (-0.30, 0.0, 0.08)),
        scene_entry(3, "Milk Jug", root + "MilkJug.obj", material, (-0.16, 0.0, -0.28)),
        scene_entry(3, "Floor", "procedural://plane", material, (0., -0.002, 0.), scale=(2.,1.,2.)),
    ]
    scene = u32(len(entries)) + b"".join(entries)
    embedded = u32(0)
    (CONTENT / "WhiteTeaService.codex").write_bytes(codex(0, 0x5748544541534552,
        [(0x4D414E57, naming), (0x564E4557, environment), (0x454E4353, scene), (0x44424D45, embedded)]))

if __name__ == "__main__": main()
