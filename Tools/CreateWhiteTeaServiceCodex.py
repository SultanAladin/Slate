#!/usr/bin/env python3
"""Create the versioned White Tea Service workspace and shared pigment Codex documents."""
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]
CONTENT = ROOT / "EngineContent"
REVISION = 2

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
        while len(stream) % 8:
            stream.append(0)
        position = len(stream)
        stream += content
        indexed.append((code, position, len(content), digest(content)))
    while len(stream) % 8:
        stream.append(0)
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

def box_mesh(name, halfx, halfy, halfz):
    verts = [
        (-halfx,-halfy,-halfz),( halfx,-halfy,-halfz),( halfx,-halfy, halfz),(-halfx,-halfy, halfz),
        (-halfx, halfy,-halfz),( halfx, halfy,-halfz),( halfx, halfy, halfz),(-halfx, halfy, halfz),
    ]
    faces = [0,1,2, 0,2,3, 4,6,5, 4,7,6, 0,4,5, 0,5,1, 1,5,6, 1,6,2, 2,6,7, 2,7,3, 3,7,4, 3,4,0]
    out = bytearray(run(name) + u32(len(verts)))
    for v in verts:
        out += b"".join(f64(x) for x in v)
    out += u32(len(faces))
    for i in faces:
        out += u32(i)
    return bytes(out)

def main():
    (CONTENT / "MaterialArchives").mkdir(parents=True, exist_ok=True)
    pigment = run("White Dielectric") + b"".join(f64(x) for x in (1.,1.,1.,0.32,1.5)) + b"\x01"
    pigment_path = CONTENT / "MaterialArchives" / "WhiteDielectric.pigment"
    pigment_path.write_bytes(codex(1, 0x574449454C454354, [(0x464E4950, pigment)]))

    naming = run("White Tea Service")
    environment = b"".join(f64(x) for x in (35., 120., 4.8, 5500., 1., 1., 1.))
    material = "EngineContent/MaterialArchives/WhiteDielectric.pigment"
    entries = [
        scene_entry(0, "Sun", "", "", (0.,0.,0.)),
        scene_entry(1, "Sky", "", "", (0.,0.,0.)),
        scene_entry(2, "Atmosphere", "", "", (0.,0.,0.)),
        scene_entry(3, "Service Teapot", "Mesh/ServiceTeapot", material, (0., 0.035, 0.)),
        scene_entry(3, "Teacup", "Mesh/Teacup", material, (0.31, 0.035, 0.05)),
        scene_entry(3, "Saucer", "Mesh/Saucer", material, (0.31, 0.0, 0.05)),
        scene_entry(3, "Sugar Bowl", "Mesh/SugarBowl", material, (-0.30, 0.0, 0.08)),
        scene_entry(3, "Milk Jug", "Mesh/MilkJug", material, (-0.16, 0.0, -0.28)),
        scene_entry(3, "Floor", "Mesh/Floor", material, (0., -0.002, 0.), scale=(2.,1.,2.)),
    ]
    scene = u32(len(entries)) + b"".join(entries)
    meshes = [
        box_mesh("Mesh/ServiceTeapot", .150, .085, .105),
        box_mesh("Mesh/Teacup", .062, .048, .062),
        box_mesh("Mesh/Saucer", .088, .010, .088),
        box_mesh("Mesh/SugarBowl", .078, .058, .072),
        box_mesh("Mesh/MilkJug", .066, .072, .054),
        box_mesh("Mesh/Floor", 1.000, .001, 1.000),
    ]
    mesh_section = u32(len(meshes)) + b"".join(meshes)
    embedded = u32(0)
    (CONTENT / "WhiteTeaService.codex").write_bytes(codex(0, 0x5748544541534552,
        [(0x4D414E57, naming), (0x564E4557, environment), (0x454E4353, scene),
         (0x4853454D, mesh_section), (0x44424D45, embedded)]))

if __name__ == "__main__":
    main()
