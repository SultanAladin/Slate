// =========================================================================================================================
//                                                   TYRESOLVERPREVIEW.JS
// =========================================================================================================================
// The engine's six tyre kernels, lowered from the same .slang to WGSL, stepping a fleet of cars in the browser.
//
// What is the engine's and what is the preview's:
//   · the lattice (TyreLattice.bin) is one tyre as ConstructTyreLattice built it — prestressed, coloured; it is tiled here
//   · the six compute pipelines are Shader/*.wgsl, produced by BuildShaders.py from Engine/.../TyreSolver/Shader/*.slang
//   · the dispatch order per substep is TyreDeviceExchange::Record's: prediction, cords by colour, gather, solve, contact,
//     recovery; the step block that Vulkan pushes is here a 256-byte-strided uniform reached by dynamic offset
//   · the load crosses back as the sixteen accumulator integers per tyre, read two frames late, and the vehicle below
//     integrates against the secant stiffness they imply — the rule documented on ContactLoad in TyreSolver.h
//   · the vehicle sim, road and rendering are this file's alone and make no claim beyond "a car to hang the tyres on"

const Gravity = 9.81;

// ------------------------------------------------------------------------------------------------------------------------
//                                                     THE LATTICE STREAM
// ------------------------------------------------------------------------------------------------------------------------

async function LoadLattice(url)
{
    const bytes  = new Uint8Array(await (await fetch(url)).arrayBuffer());
    const view   = new DataView(bytes.buffer);
    const length = view.getUint32(0, true);
    const header = JSON.parse(new TextDecoder().decode(bytes.subarray(4, 4 + length)));
    let   cursor = 4 + length;

    const take = (count) => { const out = bytes.slice(cursor, cursor + count); cursor += count; return out; };
    const P = header.ParticleCount, C = header.CordCount, R = header.ShapeRowLimit;

    header.arrays =
    {
        Particles:        take(P * 16),
        Motions:          take(P * 16),
        Velocities:       take(P * 16),
        Rests:            take(P * 16),
        Gradients:        take(P * 48),
        Cords:            take(C * 16),
        CordMultipliers:  take(C * 4),
        Hubs:             take(128),
        Accumulators:     take(header.AccumulatorStride * 4),
        ShapeRows:        take(R * 16),
        ShapeSums:        take(header.ShapeSumStride * 4),
        ShapeMultipliers: take(R * 4),
        ShapeAdvances:    take(R * 4),
        ShapeMatrix:      take(header.ShapeMatrixStride * 4),
    };

    if (cursor !== bytes.length)
        throw new Error(`lattice stream length mismatch: ${cursor} of ${bytes.length}`);

    return header;
}

// Tiles the one exported tyre into T tyres: particle indices offset per tyre, colour runs become colour-major tyre-minor
// as ConstructTyreLattice lays them out, every tyre's hub record a copy of the exported one.
function TileLattice(one, tyreCount)
{
    const P = one.ParticleCount, C = one.CordCount, T = tyreCount;
    const repeat = (src) => { const out = new Uint8Array(src.length * T); for (let t = 0; t < T; ++t) out.set(src, t * src.length); return out; };

    const particles = repeat(one.arrays.Particles);
    const rests     = repeat(one.arrays.Rests);
    const restView  = new Float32Array(rests.buffer);

    for (let t = 0; t < T; ++t)
        for (let p = 0; p < P; ++p)
            restView[(t * P + p) * 4 + 3] = t;

    const cords   = new Uint8Array(C * 16 * T);
    const cordU32 = new Uint32Array(cords.buffer);
    const srcU32  = new Uint32Array(one.arrays.Cords.buffer, one.arrays.Cords.byteOffset, C * 4);
    const colours = [];
    let   written = 0;

    for (const [offset, count] of one.Colours)
    {
        colours.push([written, count * T]);

        for (let t = 0; t < T; ++t)
            for (let c = 0; c < count; ++c)
            {
                const s = (offset + c) * 4, d = written * 4;
                cordU32[d]     = srcU32[s]     + t * P;
                cordU32[d + 1] = srcU32[s + 1] + t * P;
                cordU32[d + 2] = srcU32[s + 2];
                cordU32[d + 3] = srcU32[s + 3];
                ++written;
            }
    }

    return {
        Particles: particles,
        Motions: repeat(one.arrays.Motions),
        Velocities: repeat(one.arrays.Velocities),
        Rests: rests,
        Gradients: repeat(one.arrays.Gradients),
        Cords: cords,
        CordMultipliers: repeat(one.arrays.CordMultipliers),
        Hubs: repeat(one.arrays.Hubs),
        Accumulators: repeat(one.arrays.Accumulators),
        ShapeRows: repeat(one.arrays.ShapeRows),
        ShapeSums: repeat(one.arrays.ShapeSums),
        ShapeMultipliers: repeat(one.arrays.ShapeMultipliers),
        ShapeAdvances: repeat(one.arrays.ShapeAdvances),
        ShapeMatrix: repeat(one.arrays.ShapeMatrix),
        Colours: colours,
    };
}

// Places one tyre rigidly at a pose — PlaceTyre's job, done on the host copy before upload. Particles and their
// substep-start positions move together; velocities become the rim's; multipliers are kept.
function PlaceTyre(tiled, one, t, hub)
{
    const P = one.ParticleCount;
    const particles = new Float32Array(tiled.Particles.buffer, t * P * 16, P * 4);
    const motions   = new Float32Array(tiled.Motions.buffer,   t * P * 16, P * 4);
    const velocities = new Float32Array(tiled.Velocities.buffer, t * P * 16, P * 4);
    const rests     = new Float32Array(tiled.Rests.buffer,     t * P * 16, P * 4);

    for (let p = 0; p < P; ++p)
    {
        const w = RotateByQuaternion(hub.q, rests[p * 4], rests[p * 4 + 1], rests[p * 4 + 2]);
        particles[p * 4] = hub.x + w[0]; particles[p * 4 + 1] = hub.y + w[1]; particles[p * 4 + 2] = hub.z + w[2];
        motions[p * 4] = particles[p * 4]; motions[p * 4 + 1] = particles[p * 4 + 1]; motions[p * 4 + 2] = particles[p * 4 + 2];
        motions[p * 4 + 3] = 0;
        velocities[p * 4] = hub.vx; velocities[p * 4 + 1] = hub.vy; velocities[p * 4 + 2] = hub.vz; velocities[p * 4 + 3] = 0;
    }
}

// The hub record, as TyreHubRecord spells it: begin pose, end pose, velocities, then the inflation and ground scalars
// the construction filled and this leaves alone except for the ground height and the pressure.
function WriteHub(hubs, t, begin, end, groundHeight)
{
    const f = new Float32Array(hubs.buffer, t * 128, 32);
    f[0] = begin.x; f[1] = begin.y; f[2] = begin.z; f[3] = begin.q[0]; f[4] = begin.q[1]; f[5] = begin.q[2]; f[6] = begin.q[3];
    f[7] = end.x;   f[8] = end.y;   f[9] = end.z;   f[10] = end.q[0];  f[11] = end.q[1];  f[12] = end.q[2];  f[13] = end.q[3];
    f[14] = end.vx; f[15] = end.vy; f[16] = end.vz; f[17] = end.wx;    f[18] = end.wy;    f[19] = end.wz;
    f[23] = groundHeight;
}

// DeclareInflation's rule: α = V₀ / (γ p_abs), the volume row's target V₀ + α p₀, written to the hub and the row.
function DeclareInflation(hubs, shapeRows, shapeRowLimit, t, gaugePressure)
{
    const hub = new Float32Array(hubs.buffer, t * 128, 32);
    const row = new Float32Array(shapeRows.buffer, t * shapeRowLimit * 16, 4);
    const restVolume = hub[20], gamma = hub[29], ambient = hub[30];
    const compliance = restVolume / (gamma * (gaugePressure + ambient));
    hub[21] = gaugePressure; hub[22] = compliance;
    row[0] = restVolume + compliance * gaugePressure; row[1] = compliance;
}

// ------------------------------------------------------------------------------------------------------------------------
//                                                      SMALL ALGEBRA
// ------------------------------------------------------------------------------------------------------------------------

function RotateByQuaternion(q, x, y, z)
{
    const [qx, qy, qz, qw] = q;
    const tx = 2 * (qy * z - qz * y), ty = 2 * (qz * x - qx * z), tz = 2 * (qx * y - qy * x);
    return [x + qw * tx + (qy * tz - qz * ty), y + qw * ty + (qz * tx - qx * tz), z + qw * tz + (qx * ty - qy * tx)];
}

function MultiplyQuaternion(a, b)
{
    return [a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1],
            a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0],
            a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3],
            a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2]];
}

function Perspective(fovY, aspect, near)
{
    // Reversed-infinite depth, as the engine's own projection is.
    const f = 1 / Math.tan(fovY / 2);
    return new Float32Array([f / aspect, 0, 0, 0,  0, f, 0, 0,  0, 0, 0, -1,  0, 0, near, 0]);
}

function LookAt(eye, target, up)
{
    const z = Normalise([eye[0] - target[0], eye[1] - target[1], eye[2] - target[2]]);
    const x = Normalise(Cross(up, z));
    const y = Cross(z, x);
    return new Float32Array([x[0], y[0], z[0], 0,  x[1], y[1], z[1], 0,  x[2], y[2], z[2], 0,
                             -Dot(x, eye), -Dot(y, eye), -Dot(z, eye), 1]);
}

function Multiply4(a, b)
{
    const out = new Float32Array(16);
    for (let c = 0; c < 4; ++c) for (let r = 0; r < 4; ++r)
        out[c * 4 + r] = a[r] * b[c * 4] + a[4 + r] * b[c * 4 + 1] + a[8 + r] * b[c * 4 + 2] + a[12 + r] * b[c * 4 + 3];
    return out;
}

const Dot = (a, b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
const Cross = (a, b) => [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]];
const Normalise = (a) => { const l = Math.hypot(a[0], a[1], a[2]) || 1; return [a[0] / l, a[1] / l, a[2] / l]; };

// ------------------------------------------------------------------------------------------------------------------------
//                                                         THE ROAD
// ------------------------------------------------------------------------------------------------------------------------

// A ring road of radius TrackRadius with four lanes, a speed hump across all lanes, a stretch of rumble and a long
// gentle swell. The height is a function of position so that the ground mesh and the per-tyre plane agree exactly.
const TrackRadius = 80;
const LaneRadii   = [TrackRadius - 4.5, TrackRadius - 1.5, TrackRadius + 1.5, TrackRadius + 4.5];

function RoadHeight(x, z)
{
    const r   = Math.hypot(x, z);
    const phi = Math.atan2(z, x);                       // [rad] around the ring
    const s   = ((phi + 2 * Math.PI) % (2 * Math.PI)) * TrackRadius;   // [m] arc position on the centre line
    let h = 0;

    // the hump: a raised cosine 1.2 m long and 45 mm high at s = 40
    const d = s - 40;
    if (Math.abs(d) < 0.6) h += 0.045 * 0.5 * (1 + Math.cos(Math.PI * d / 0.6));

    // the rumble strip: 12 mm ripples 0.5 m apart between s = 120 and s = 150
    if (s > 120 && s < 150) h += 0.006 * (1 - Math.cos(2 * Math.PI * (s - 120) / 0.5));

    // the swell: a 60 mm wave 25 m long between s = 250 and s = 400, banked inward
    if (s > 250 && s < 400) h += 0.06 * 0.5 * (1 - Math.cos(2 * Math.PI * (s - 250) / 25)) * (1 + 0.3 * (TrackRadius - r) / 6);

    return h;
}

// ------------------------------------------------------------------------------------------------------------------------
//                                                      THE VEHICLE SIM
// ------------------------------------------------------------------------------------------------------------------------

// A quarter-car per wheel hanging from one rigid body in heave, pitch and roll, driven around its lane at a set speed.
// The tyre is the GPU's: this only holds the hub and reads the load back.
const Car =
{
    Mass: 1400, Wheelbase: 2.7, Track: 1.6, UnsprungMass: 40,
    SpringRate: 32000, Damping: 3200, RideHeight: 0.33, StaticTravel: 0.0,
    Ixx: 500, Izz: 2400, BodyHeight: 0.55,
};

class Vehicle
{
    constructor(index, laneRadius, phase, outerRadius)
    {
        this.lane = laneRadius; this.phi = phase; this.outerRadius = outerRadius;
        this.heave = 0; this.heaveV = 0; this.pitch = 0; this.pitchV = 0; this.roll = 0; this.rollV = 0;
        this.wheels = [];
        const corners = [[+1, -1], [+1, +1], [-1, -1], [-1, +1]];   // [forward, lateral(+ = outward)] FL FR RL RR

        for (const [fwd, lat] of corners)
            this.wheels.push({ fwd, lat, y: outerRadius, vy: 0, spin: 0, load: 0, loadAtY: outerRadius, k: 0, contacts: 0, fxz: 0 });

        // the static load on each corner is a quarter of the body plus the unsprung mass; the spring is
        // preloaded so that the ride height is the rest height
        Car.StaticTravel = (Car.Mass / 4) * Gravity / Car.SpringRate;
    }

    // World frame for the car at this instant: position on the lane, forward along the tangent.
    frame(phi)
    {
        const cx = this.lane * Math.cos(phi), cz = this.lane * Math.sin(phi);
        const fwd = Normalise([-Math.sin(phi), 0, Math.cos(phi)]);   // d/dphi of (cos, sin), counter-clockwise from above
        const lat = Cross(fwd, [0, 1, 0]);                            // right-hand lateral; outward on this ring
        return { cx, cz, fwd, lat };
    }

    step(dt, speed, hubs, firstTyre, tyreStiffnessGuess)
    {
        const begin = this.frame(this.phi);
        const omega = speed / this.lane;
        this.phi += omega * dt;
        const end = this.frame(this.phi);

        const wheelAt = (f, w) => [f.cx + w.fwd * Car.Wheelbase / 2 * f.fwd[0] + w.lat * Car.Track / 2 * f.lat[0],
                                   f.cz + w.fwd * Car.Wheelbase / 2 * f.fwd[2] + w.lat * Car.Track / 2 * f.lat[2]];
        const groundMean = this.wheels.reduce((sum, w) => { const [x, z] = wheelAt(end, w); return sum + RoadHeight(x, z); }, 0) / 4;
        const bodyY = groundMean + this.outerRadius + Car.RideHeight + this.heave;
        const freeLength = Car.RideHeight + Car.StaticTravel;   // [m] - the spring's length carrying nothing

        // The body's lateral acceleration is inward; its inertial reaction at the body's height rolls it outward.
        // `lat` is fwd × up, which on this ring points inward, so outward roll is negative and the moment is too.
        const lateral = speed * speed / this.lane;
        let heaveForce = -Car.Mass * Gravity, pitchMoment = 0, rollMoment = -Car.Mass * lateral * Car.BodyHeight;

        for (let i = 0; i < 4; ++i)
        {
            const w = this.wheels[i];
            const [px, pz] = wheelAt(end, w);
            const ground = RoadHeight(px, pz);

            // the body corner this wheel hangs from: pitch positive is nose up, roll positive lowers the inner side
            const cornerY = bodyY + w.fwd * Car.Wheelbase / 2 * Math.sin(this.pitch) - w.lat * Car.Track / 2 * Math.sin(this.roll);
            const cornerV = this.heaveV + w.fwd * Car.Wheelbase / 2 * this.pitchV - w.lat * Car.Track / 2 * this.rollV;

            // The tyre, as the secant rule on ContactLoad says: the load it last reported, at the hub height it was
            // measured at, and the stiffness the two imply. Before the first readback, or when the hub has only just
            // reached the ground, a guessed stiffness keeps the car from falling through its own tyres.
            let f0 = 0, y0 = w.y, kt = 0;
            if (w.load > 0)
            {
                const measured = Math.max(0, this.outerRadius + ground - w.loadAtY);
                f0 = w.load; y0 = w.loadAtY; kt = measured > 1e-4 ? w.load / measured : tyreStiffnessGuess;
            }
            else if (w.y - ground < this.outerRadius)
            {
                f0 = tyreStiffnessGuess * (this.outerRadius + ground - w.y); kt = tyreStiffnessGuess;
            }

            // m v′ = −m g + [F₀ + kₜ(y₀ − y′)] − [kₛ(L₀ − (cornerY − y′)) + c(v′ − cornerV)],  y′ = y + v′ dt
            const m = Car.UnsprungMass;
            const solve = (f0, y0, kt) => {
                const num = m * w.vy + dt * (-m * Gravity + f0 + kt * (y0 - w.y) - Car.SpringRate * (freeLength - cornerY + w.y) + Car.Damping * cornerV);
                const den = m + dt * (Car.Damping + dt * (kt + Car.SpringRate));
                return num / den;
            };
            let vNew = solve(f0, y0, kt);
            if (f0 + kt * (y0 - w.y - vNew * dt) < 0) vNew = solve(0, w.y, 0);   // the tyre cannot pull
            const yNew = w.y + vNew * dt;

            const springForce = Car.SpringRate * (freeLength - (cornerY - yNew)) + Car.Damping * (vNew - cornerV);   // [N] - up on the body
            heaveForce  += springForce;
            pitchMoment += springForce * w.fwd * Car.Wheelbase / 2;
            rollMoment  -= springForce * w.lat * Car.Track / 2;

            // the rim: yaw along the tangent, spin about the axle; both poses, so the substeps interpolate
            const spinRate  = -speed / (this.outerRadius - 0.012);
            const spinBegin = w.spin;
            w.spin += spinRate * dt;

            const poseBegin = this.rimPose(begin, w, w.y, spinBegin, speed, omega, spinRate);
            const poseEnd   = this.rimPose(end,   w, yNew, w.spin, speed, omega, spinRate);
            poseEnd.vy = vNew;

            WriteHub(hubs, firstTyre + i, poseBegin, poseEnd, ground);

            w.y = yNew; w.vy = vNew; w.ground = ground; w.deflection = Math.max(0, this.outerRadius + ground - yNew);
        }

        // the body: symplectic Euler in heave, pitch and roll
        this.heaveV += dt * heaveForce / Car.Mass;  this.heave += dt * this.heaveV;
        this.pitchV += dt * pitchMoment / Car.Izz;  this.pitch += dt * this.pitchV;
        this.rollV  += dt * rollMoment / Car.Ixx;   this.roll  += dt * this.rollV;
        this.bodyY = bodyY; this.frameNow = end;
    }

    rimPose(frame, w, y, spin, speed, omega, spinRate)
    {
        const x = frame.cx + w.fwd * Car.Wheelbase / 2 * frame.fwd[0] + w.lat * Car.Track / 2 * frame.lat[0];
        const z = frame.cz + w.fwd * Car.Wheelbase / 2 * frame.fwd[2] + w.lat * Car.Track / 2 * frame.lat[2];
        // yaw about Y taking local X to the tangent: rotating (1,0,0) by ψ about Y gives (cos ψ, 0, −sin ψ)
        const psi = Math.atan2(-frame.fwd[2], frame.fwd[0]);
        const qYaw  = [0, Math.sin(psi / 2), 0, Math.cos(psi / 2)];
        const qSpin = [0, 0, Math.sin(spin / 2), Math.cos(spin / 2)];
        const q = MultiplyQuaternion(qYaw, qSpin);
        const axle = RotateByQuaternion(qYaw, 0, 0, 1);
        return { x, y, z, q, vx: speed * frame.fwd[0], vy: 0, vz: speed * frame.fwd[2],
                 wx: axle[0] * spinRate, wy: -omega, wz: axle[2] * spinRate };   // φ increasing is a turn about −Y
    }

    // Takes a decoded load for wheel i, remembering the hub height it was measured at (two frames ago).
    acceptLoad(i, fy, fxz, contacts, measuredY)
    {
        const w = this.wheels[i];
        w.load = fy; w.fxz = fxz; w.contacts = contacts; w.loadAtY = measuredY;
    }
}

// ------------------------------------------------------------------------------------------------------------------------
//                                                        RENDERING
// ------------------------------------------------------------------------------------------------------------------------

const RenderShader = /* wgsl */`
struct View { viewProjection : mat4x4<f32>, eye : vec4<f32>, params : vec4<u32> };
@group(0) @binding(0) var<uniform> view : View;
@group(0) @binding(1) var<storage, read> particles : array<vec4<f32>>;
@group(0) @binding(2) var<storage, read> motions : array<vec4<f32>>;
@group(0) @binding(3) var<storage, read> hubs : array<f32>;

struct Out { @builtin(position) clip : vec4<f32>, @location(0) world : vec3<f32>, @location(1) tint : vec3<f32> };

@vertex fn tyreVertex(@builtin(vertex_index) vi : u32, @builtin(instance_index) tyre : u32, @location(0) index : u32) -> Out {
    let perTyre = view.params.x;
    var p : vec3<f32>;
    var tint = vec3<f32>(0.16, 0.17, 0.19);
    if (index >= perTyre) {
        // the rim centre: the hub's end pose, offset along the axle to the bead plane this fan belongs to
        let h = tyre * 32u;
        let side = select(-1.0, 1.0, index == perTyre + 1u);
        let q = vec4<f32>(hubs[h + 10u], hubs[h + 11u], hubs[h + 12u], hubs[h + 13u]);
        let a = normalize(vec3<f32>(2.0 * (q.x * q.z + q.w * q.y), 2.0 * (q.y * q.z - q.w * q.x), 1.0 - 2.0 * (q.x * q.x + q.y * q.y)));
        p = vec3<f32>(hubs[h + 7u], hubs[h + 8u], hubs[h + 9u]) + a * side * 0.095;
        tint = vec3<f32>(0.62, 0.64, 0.66);
    } else {
        let i = tyre * perTyre + index;
        p = particles[i].xyz;
        let contact = motions[i].w;
        if (contact > 0.0) { tint = vec3<f32>(0.95, 0.45, 0.12); }
        if (particles[i].w == 0.0) { tint = vec3<f32>(0.62, 0.64, 0.66); }
    }
    var o : Out;
    o.clip = view.viewProjection * vec4<f32>(p, 1.0);
    o.world = p;
    o.tint = tint;
    return o;
}

@vertex fn meshVertex(@location(0) position : vec3<f32>, @location(1) tint : vec3<f32>) -> Out {
    var o : Out;
    o.clip = view.viewProjection * vec4<f32>(position, 1.0);
    o.world = position;
    o.tint = tint;
    return o;
}

@fragment fn shade(in : Out) -> @location(0) vec4<f32> {
    let n = normalize(cross(dpdx(in.world), dpdy(in.world)));
    let l = normalize(vec3<f32>(0.35, 0.8, 0.45));
    let v = normalize(view.eye.xyz - in.world);
    let diffuse = abs(dot(n, l));
    let fresnel = pow(1.0 - abs(dot(n, v)), 3.0) * 0.25;
    let colour = in.tint * (0.25 + 0.75 * diffuse) + vec3<f32>(fresnel);
    let fog = clamp(length(view.eye.xyz - in.world) / 260.0, 0.0, 1.0);
    return vec4<f32>(mix(colour, vec3<f32>(0.05, 0.06, 0.07), fog * fog), 1.0);
}
`;

// One tyre's triangle indices over the lattice, plus two bead fans closing the rim; index PerTyre and PerTyre+1 name
// the two rim centres.
function BuildTyreIndices(rings, segments)
{
    const per = rings * segments;
    const out = [];
    for (let r = 0; r + 1 < rings; ++r)
        for (let s = 0; s < segments; ++s)
        {
            const a = r * segments + s, b = r * segments + (s + 1) % segments, c = a + segments, d = b + segments;
            out.push(a, c, b, b, c, d);
        }
    for (let s = 0; s < segments; ++s)
    {
        out.push(per, (s + 1) % segments, s);
        out.push(per + 1, (rings - 1) * segments + s, (rings - 1) * segments + (s + 1) % segments);
    }
    return new Uint32Array(out);
}

function BuildGround()
{
    const positions = [], tints = [];
    const radii = [], angles = 1600, inner = TrackRadius - 14, outer = TrackRadius + 14;
    for (let r = inner; r <= outer + 1e-6; r += 0.5) radii.push(r);
    const vertex = (r, a) => {
        const x = r * Math.cos(a), z = r * Math.sin(a);
        const h = RoadHeight(x, z);
        const lane = Math.abs(((r - TrackRadius + 6) % 3) - 1.5) < 0.05 && r > TrackRadius - 6.1 && r < TrackRadius + 6.1;
        const onRoad = r > TrackRadius - 6 && r < TrackRadius + 6;
        positions.push(x, h, z);
        if (lane) tints.push(0.75, 0.75, 0.7);
        else if (onRoad) tints.push(0.21 + h * 1.5, 0.21 + h * 1.5, 0.22 + h * 1.5);
        else tints.push(0.12, 0.16, 0.11);
    };
    for (let i = 0; i < angles; ++i)
    {
        const a0 = i / angles * 2 * Math.PI, a1 = (i + 1) / angles * 2 * Math.PI;
        for (let j = 0; j + 1 < radii.length; ++j)
        {
            vertex(radii[j], a0); vertex(radii[j + 1], a0); vertex(radii[j], a1);
            vertex(radii[j], a1); vertex(radii[j + 1], a0); vertex(radii[j + 1], a1);
        }
    }
    return { positions: new Float32Array(positions), tints: new Float32Array(tints) };
}

function BoxTriangles(out, centre, fwd, lat, up, half, tint)
{
    const corner = (i, j, k) => [centre[0] + fwd[0] * i * half[0] + lat[0] * j * half[1] + up[0] * k * half[2],
                                 centre[1] + fwd[1] * i * half[0] + lat[1] * j * half[1] + up[1] * k * half[2],
                                 centre[2] + fwd[2] * i * half[0] + lat[2] * j * half[1] + up[2] * k * half[2]];
    const faces = [[[1,1,1],[1,-1,1],[1,-1,-1],[1,1,-1]], [[-1,1,1],[-1,1,-1],[-1,-1,-1],[-1,-1,1]],
                   [[1,1,1],[1,1,-1],[-1,1,-1],[-1,1,1]], [[1,-1,1],[-1,-1,1],[-1,-1,-1],[1,-1,-1]],
                   [[1,1,1],[-1,1,1],[-1,-1,1],[1,-1,1]], [[1,1,-1],[1,-1,-1],[-1,-1,-1],[-1,1,-1]]];
    for (const f of faces)
    {
        const c = f.map(([i, j, k]) => corner(i, j, k));
        for (const idx of [0, 1, 2, 0, 2, 3]) out.push(...c[idx], ...tint);
    }
}

// ------------------------------------------------------------------------------------------------------------------------
//                                                        THE PREVIEW
// ------------------------------------------------------------------------------------------------------------------------

const EntryPoints = ["TyrePrediction", "TyreCordProjection", "TyreShapeGather", "TyreShapeSolve", "TyreContactProjection", "TyreVelocityRecovery"];
const StepStride  = 256;   // [B] - one step block per dispatch, at the uniform offset alignment

class Preview
{
    constructor(device, canvas, one, wgsl)
    {
        this.device = device; this.canvas = canvas; this.one = one; this.wgsl = wgsl;
        this.context = canvas.getContext("webgpu");
        this.format = navigator.gpu.getPreferredCanvasFormat();
        this.context.configure({ device, format: this.format, alphaMode: "opaque" });
        this.camera = { yaw: 0.9, pitch: 0.35, distance: 9, target: [0, 0, 0], follow: true };
        this.paused = false; this.frameIndex = 0; this.watch = 0;
        this.settings = { cars: 20, speed: 15, substeps: 32, pressure: 2.2 };
        this.timing = { frame: 0, gpu: 0 };
        this.buildStatic();
        this.buildPipelines();
    }

    buildStatic()
    {
        const d = this.device;
        this.viewBuffer = d.createBuffer({ size: 112, usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST });
        const indices = BuildTyreIndices(this.one.Rings, this.one.Segments);
        this.tyreIndexCount = indices.length;
        this.tyreIndexBuffer = d.createBuffer({ size: indices.byteLength, usage: GPUBufferUsage.VERTEX | GPUBufferUsage.COPY_DST });
        d.queue.writeBuffer(this.tyreIndexBuffer, 0, indices);

        const ground = BuildGround();
        this.groundCount = ground.positions.length / 3;
        this.groundPositions = d.createBuffer({ size: ground.positions.byteLength, usage: GPUBufferUsage.VERTEX | GPUBufferUsage.COPY_DST });
        this.groundTints = d.createBuffer({ size: ground.tints.byteLength, usage: GPUBufferUsage.VERTEX | GPUBufferUsage.COPY_DST });
        d.queue.writeBuffer(this.groundPositions, 0, ground.positions);
        d.queue.writeBuffer(this.groundTints, 0, ground.tints);
        this.bodyBuffer = d.createBuffer({ size: 40 * 36 * 6 * 4, usage: GPUBufferUsage.VERTEX | GPUBufferUsage.COPY_DST });

        this.canTime = d.features.has("timestamp-query");
        if (this.canTime)
        {
            this.querySet = d.createQuerySet({ type: "timestamp", count: 2 });
            this.queryBuffer = d.createBuffer({ size: 16, usage: GPUBufferUsage.QUERY_RESOLVE | GPUBufferUsage.COPY_SRC });
            this.queryRead = [0, 1, 2].map(() => ({ buffer: d.createBuffer({ size: 16, usage: GPUBufferUsage.MAP_READ | GPUBufferUsage.COPY_DST }), busy: false }));
        }
    }

    buildPipelines()
    {
        const d = this.device;
        const readOnly = new Set([3, 5, 7, 9]);
        this.programs = EntryPoints.map((name) => {
            const source = this.wgsl[name];
            const used = [...source.matchAll(/@binding\((\d+)\) @group\(0\)/g)].map((m) => +m[1]);
            const entries = used.map((b) => ({
                binding: b, visibility: GPUShaderStage.COMPUTE,
                buffer: b === 14 ? { type: "uniform", hasDynamicOffset: true, minBindingSize: 48 }
                                 : { type: readOnly.has(b) ? "read-only-storage" : "storage" },
            }));
            const layout = d.createBindGroupLayout({ entries });
            const module = d.createShaderModule({ label: name, code: source });
            const pipeline = d.createComputePipeline({ label: name, layout: d.createPipelineLayout({ bindGroupLayouts: [layout] }),
                                                       compute: { module, entryPoint: "main" } });
            return { name, used, layout, pipeline, bindGroup: null };
        });

        const renderModule = d.createShaderModule({ code: RenderShader });
        this.renderLayout = d.createBindGroupLayout({ entries: [
            { binding: 0, visibility: GPUShaderStage.VERTEX | GPUShaderStage.FRAGMENT, buffer: { type: "uniform" } },
            { binding: 1, visibility: GPUShaderStage.VERTEX, buffer: { type: "read-only-storage" } },
            { binding: 2, visibility: GPUShaderStage.VERTEX, buffer: { type: "read-only-storage" } },
            { binding: 3, visibility: GPUShaderStage.VERTEX, buffer: { type: "read-only-storage" } },
        ] });
        const layout = d.createPipelineLayout({ bindGroupLayouts: [this.renderLayout] });
        const depth = { format: "depth32float", depthWriteEnabled: true, depthCompare: "greater" };
        this.tyrePipeline = d.createRenderPipeline({ layout, depthStencil: depth, primitive: { topology: "triangle-list", cullMode: "none" },
            vertex: { module: renderModule, entryPoint: "tyreVertex", buffers: [{ arrayStride: 4, attributes: [{ shaderLocation: 0, offset: 0, format: "uint32" }] }] },
            fragment: { module: renderModule, entryPoint: "shade", targets: [{ format: this.format }] } });
        this.meshPipeline = d.createRenderPipeline({ layout, depthStencil: depth, primitive: { topology: "triangle-list", cullMode: "none" },
            vertex: { module: renderModule, entryPoint: "meshVertex", buffers: [
                { arrayStride: 12, attributes: [{ shaderLocation: 0, offset: 0, format: "float32x3" }] },
                { arrayStride: 12, attributes: [{ shaderLocation: 1, offset: 0, format: "float32x3" }] }] },
            fragment: { module: renderModule, entryPoint: "shade", targets: [{ format: this.format }] } });
        this.bodyPipeline = d.createRenderPipeline({ layout, depthStencil: depth, primitive: { topology: "triangle-list", cullMode: "none" },
            vertex: { module: renderModule, entryPoint: "meshVertex", buffers: [
                { arrayStride: 24, attributes: [{ shaderLocation: 0, offset: 0, format: "float32x3" }, { shaderLocation: 1, offset: 12, format: "float32x3" }] }] },
            fragment: { module: renderModule, entryPoint: "shade", targets: [{ format: this.format }] } });
    }

    // Builds the fleet: tiles the lattice, places every tyre under its car, uploads, and lays out the step blocks.
    buildFleet()
    {
        const d = this.device, one = this.one;
        const cars = this.settings.cars, T = cars * 4;
        this.tyreCount = T;
        const tiled = TileLattice(one, T);

        this.vehicles = [];
        const perLane = Math.ceil(cars / LaneRadii.length);
        for (let c = 0; c < cars; ++c)
        {
            const lane = c % LaneRadii.length, slot = Math.floor(c / LaneRadii.length);
            const phase = (slot / perLane) * 2 * Math.PI + lane * 0.35;
            const v = new Vehicle(c, LaneRadii[lane], phase, one.OuterRadius);
            this.vehicles.push(v);
            const f = v.frame(v.phi);
            for (let i = 0; i < 4; ++i)
            {
                const w = v.wheels[i];
                const pose = v.rimPose(f, w, 0, 0, 0, 0, 0);
                const ground = RoadHeight(pose.x, pose.z);
                w.y = ground + one.OuterRadius + 0.004; w.ground = ground;
                pose.y = w.y;
                PlaceTyre(tiled, one, c * 4 + i, pose);
            }
        }

        if (this.spans) for (const b of Object.values(this.spans)) b.destroy();
        this.spans = {};
        const names = ["Particles", "Motions", "Velocities", "Rests", "Gradients", "Cords", "CordMultipliers", "Hubs", "Accumulators",
                       "ShapeRows", "ShapeSums", "ShapeMultipliers", "ShapeAdvances", "ShapeMatrix"];
        this.spanOrder = names;
        for (const name of names)
        {
            const bytes = tiled[name];
            const buffer = d.createBuffer({ label: name, size: bytes.byteLength, usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST | GPUBufferUsage.COPY_SRC });
            d.queue.writeBuffer(buffer, 0, bytes);
            this.spans[name] = buffer;
        }
        this.hubBytes = tiled.Hubs;
        this.shapeRowBytes = tiled.ShapeRows;
        this.colours = tiled.Colours;
        this.speedNow = 0;
        this.gaugeNow = new Float32Array(one.arrays.Hubs.buffer, one.arrays.Hubs.byteOffset, 32)[21];

        this.readback = [0, 1, 2].map(() => ({ buffer: d.createBuffer({ size: T * 64, usage: GPUBufferUsage.MAP_READ | GPUBufferUsage.COPY_DST }), busy: false, frame: -1, hubY: new Float32Array(T) }));
        this.layoutSteps();

        for (const program of this.programs)
            program.bindGroup = d.createBindGroup({ layout: program.layout, entries: program.used.map((b) => ({
                binding: b, resource: b === 14 ? { buffer: this.stepBuffer, size: 48 } : { buffer: this.spans[names[b]] } })) });

        this.renderBindGroup = d.createBindGroup({ layout: this.renderLayout, entries: [
            { binding: 0, resource: { buffer: this.viewBuffer } },
            { binding: 1, resource: { buffer: this.spans.Particles } },
            { binding: 2, resource: { buffer: this.spans.Motions } },
            { binding: 3, resource: { buffer: this.spans.Hubs } } ] });

        const watch = document.getElementById("watch");
        watch.innerHTML = "";
        for (let c = 0; c < cars; ++c) { const o = document.createElement("option"); o.value = c; o.textContent = `car ${c + 1}`; watch.appendChild(o); }
        this.watch = Math.min(this.watch, cars - 1); watch.value = this.watch;
        document.getElementById("tyres").textContent = T;
        document.getElementById("particles").textContent = (T * one.ParticleCount).toLocaleString();
    }

    // The step blocks for every dispatch of a frame, written once: they change only with the substep count.
    layoutSteps()
    {
        const one = this.one, S = this.settings.substeps, T = this.tyreCount;
        const perSubstep = 1 + this.colours.length + 3 + 1;
        this.dispatches = [];
        const blocks = new ArrayBuffer(S * perSubstep * StepStride);
        const u32 = new Uint32Array(blocks), f32 = new Float32Array(blocks);
        const P = one.ParticleCount * T, groups = (n) => Math.ceil(n / one.GroupExtent);
        let k = 0;
        const push = (program, substep, iteration, cordOffset, cordCount, groupCount) => {
            const o = k * StepStride / 4;
            u32[o] = P; u32[o + 1] = one.Rings; u32[o + 2] = one.Segments; u32[o + 3] = T;
            f32[o + 4] = (1 / 60) / S; u32[o + 5] = substep; u32[o + 6] = S; u32[o + 7] = iteration;
            u32[o + 8] = substep; u32[o + 9] = cordOffset; u32[o + 10] = cordCount;
            this.dispatches.push({ program, offset: k * StepStride, groups: groupCount });
            ++k;
        };
        for (let s = 0; s < S; ++s)
        {
            push(0, s, 0, 0, 0, groups(P));
            for (const [offset, count] of this.colours) push(1, s, 0, offset, count, groups(count));
            push(2, s, 0, 0, 0, groups(P));
            push(3, s, 0, 0, 0, T);
            push(4, s, 0, 0, 0, groups(P));
            push(5, s, 0, 0, 0, groups(P));
        }
        if (this.stepBuffer) this.stepBuffer.destroy();
        this.stepBuffer = this.device.createBuffer({ size: blocks.byteLength, usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST });
        this.device.queue.writeBuffer(this.stepBuffer, 0, blocks);
        document.getElementById("dispatches").textContent = this.dispatches.length;
        if (this.programs[0].bindGroup)
            for (const program of this.programs)
                program.bindGroup = this.device.createBindGroup({ layout: program.layout, entries: program.used.map((b) => ({
                    binding: b, resource: b === 14 ? { buffer: this.stepBuffer, size: 48 } : { buffer: this.spans[this.spanOrder[b]] } })) });
    }

    frame(now)
    {
        const d = this.device, dt = 1 / 60;
        const started = performance.now();

        if (!this.paused)
        {
            // the speed slews at 4 m/s² — a rim yanked from rest to fifteen metres a second in one frame drags
            // its beads a quarter of a metre ahead of the carcass, which is the first failure PlaceTyre's note names
            const target = this.settings.speed;
            this.speedNow += Math.max(-4 * dt, Math.min(4 * dt, target - this.speedNow));

            const gauge = this.settings.pressure * 1e5;
            if (gauge !== this.gaugeNow)
            {
                this.gaugeNow = gauge;
                for (let t = 0; t < this.tyreCount; ++t) DeclareInflation(this.hubBytes, this.shapeRowBytes, this.one.ShapeRowLimit, t, gauge);
                d.queue.writeBuffer(this.spans.ShapeRows, 0, this.shapeRowBytes);
            }

            for (let c = 0; c < this.vehicles.length; ++c)
                this.vehicles[c].step(dt, this.speedNow, this.hubBytes, c * 4, 250000);
            d.queue.writeBuffer(this.spans.Hubs, 0, this.hubBytes);
        }

        const encoder = d.createCommandEncoder();

        if (!this.paused)
        {
            const timestamps = this.canTime ? { querySet: this.querySet, beginningOfPassIndex: 0, endOfPassIndex: 1 } : undefined;
            const pass = encoder.beginComputePass({ timestampWrites: timestamps });
            let current = -1;
            for (const dispatch of this.dispatches)
            {
                const program = this.programs[dispatch.program];
                if (current !== dispatch.program) { pass.setPipeline(program.pipeline); current = dispatch.program; }
                pass.setBindGroup(0, program.bindGroup, [dispatch.offset]);
                pass.dispatchWorkgroups(dispatch.groups);
            }
            pass.end();

            const slot = this.readback[this.frameIndex % 3];
            if (!slot.busy)
            {
                encoder.copyBufferToBuffer(this.spans.Accumulators, 0, slot.buffer, 0, slot.buffer.size);
                slot.busy = true; slot.frame = this.frameIndex;
                for (let t = 0; t < this.tyreCount; ++t) slot.hubY[t] = this.vehicles[t >> 2].wheels[t & 3].y;
                slot.pending = true;
            }
            if (this.canTime)
            {
                const q = this.queryRead[this.frameIndex % 3];
                if (!q.busy) { encoder.resolveQuerySet(this.querySet, 0, 2, this.queryBuffer, 0); encoder.copyBufferToBuffer(this.queryBuffer, 0, q.buffer, 0, 16); q.busy = true; q.pending = true; }
            }
        }

        this.render(encoder);
        d.queue.submit([encoder.finish()]);

        // the readback: mapped when the device is done with it, decoded, and the hub height it was measured at kept
        for (const slot of this.readback)
            if (slot.pending)
            {
                slot.pending = false;
                slot.buffer.mapAsync(GPUMapMode.READ).then(() => {
                    const ints = new Int32Array(slot.buffer.getMappedRange());
                    for (let t = 0; t < this.tyreCount; ++t)
                    {
                        const b = t * 16;
                        const fx = ints[b] / 4096, fy = ints[b + 1] / 4096, fz = ints[b + 2] / 4096;
                        this.vehicles[t >> 2].acceptLoad(t & 3, fy, Math.hypot(fx, fz), ints[b + 6], slot.hubY[t]);
                    }
                    slot.buffer.unmap(); slot.busy = false;
                }).catch(() => { slot.busy = false; });
            }
        if (this.canTime)
            for (const q of this.queryRead)
                if (q.pending)
                {
                    q.pending = false;
                    q.buffer.mapAsync(GPUMapMode.READ).then(() => {
                        const t = new BigInt64Array(q.buffer.getMappedRange());
                        this.timing.gpu = Number(t[1] - t[0]) / 1e6;
                        q.buffer.unmap(); q.busy = false;
                    }).catch(() => { q.busy = false; });
                }

        if (!this.paused) ++this.frameIndex;
        this.timing.frame = this.timing.frame * 0.9 + (performance.now() - started) * 0.1;
        this.hud();
    }

    render(encoder)
    {
        const d = this.device, canvas = this.canvas;
        const width = canvas.clientWidth * devicePixelRatio | 0, height = canvas.clientHeight * devicePixelRatio | 0;
        if (canvas.width !== width || canvas.height !== height)
        {
            canvas.width = width; canvas.height = height;
            if (this.depth) this.depth.destroy();
            this.depth = d.createTexture({ size: [width, height], format: "depth32float", usage: GPUTextureUsage.RENDER_ATTACHMENT });
        }

        const cam = this.camera, v = this.vehicles[this.watch];
        if (cam.follow && v && v.frameNow)
        {
            const f = v.frameNow;
            cam.target = [f.cx, v.bodyY - 0.2, f.cz];
        }
        const eye = [cam.target[0] + cam.distance * Math.cos(cam.pitch) * Math.sin(cam.yaw),
                     cam.target[1] + cam.distance * Math.sin(cam.pitch),
                     cam.target[2] + cam.distance * Math.cos(cam.pitch) * Math.cos(cam.yaw)];
        const viewProjection = Multiply4(Perspective(0.9, width / height, 0.05), LookAt(eye, cam.target, [0, 1, 0]));
        const block = new ArrayBuffer(112);
        new Float32Array(block, 0, 16).set(viewProjection);
        new Float32Array(block, 64, 4).set([eye[0], eye[1], eye[2], 0]);
        new Uint32Array(block, 80, 4).set([this.one.ParticleCount, 0, 0, 0]);
        d.queue.writeBuffer(this.viewBuffer, 0, block);

        // the car bodies, rebuilt each frame from the vehicle states
        const body = [];
        for (const car of this.vehicles)
        {
            if (!car.frameNow) continue;
            const f = car.frameNow, up = [0, 1, 0];
            const centre = [f.cx, car.bodyY + 0.35, f.cz];
            const fwd = [f.fwd[0] * Math.cos(car.pitch), Math.sin(car.pitch), f.fwd[2] * Math.cos(car.pitch)];
            const lat = [f.lat[0] * Math.cos(car.roll), -Math.sin(car.roll), f.lat[2] * Math.cos(car.roll)];
            const tint = car === v ? [0.85, 0.2, 0.15] : [0.22, 0.32, 0.5];
            BoxTriangles(body, centre, fwd, lat, up, [2.1, 0.85, 0.28], tint);
            BoxTriangles(body, [centre[0] - fwd[0] * 0.3 + up[0] * 0.5, centre[1] + 0.5, centre[2] - fwd[2] * 0.3], fwd, lat, up, [1.1, 0.75, 0.25], tint);
        }
        const bodyData = new Float32Array(body);
        this.bodyCount = bodyData.length / 6;
        d.queue.writeBuffer(this.bodyBuffer, 0, bodyData);

        const pass = encoder.beginRenderPass({
            colorAttachments: [{ view: this.context.getCurrentTexture().createView(), clearValue: { r: 0.05, g: 0.06, b: 0.07, a: 1 }, loadOp: "clear", storeOp: "store" }],
            depthStencilAttachment: { view: this.depth.createView(), depthClearValue: 0, depthLoadOp: "clear", depthStoreOp: "discard" } });
        pass.setBindGroup(0, this.renderBindGroup);
        pass.setPipeline(this.meshPipeline);
        pass.setVertexBuffer(0, this.groundPositions); pass.setVertexBuffer(1, this.groundTints);
        pass.draw(this.groundCount);
        pass.setPipeline(this.bodyPipeline);
        pass.setVertexBuffer(0, this.bodyBuffer);
        pass.draw(this.bodyCount);
        pass.setPipeline(this.tyrePipeline);
        pass.setVertexBuffer(0, this.tyreIndexBuffer);
        pass.draw(this.tyreIndexCount, this.tyreCount);
        pass.end();
    }

    hud()
    {
        const v = this.vehicles[this.watch];
        document.getElementById("frameMs").textContent = this.timing.frame.toFixed(2);
        document.getElementById("gpuMs").textContent = this.canTime ? this.timing.gpu.toFixed(2) : "n/a";
        if (!v) return;
        const names = ["fl", "fr", "rl", "rr"];
        let sum = 0;
        for (let i = 0; i < 4; ++i)
        {
            const w = v.wheels[i];
            sum += w.load;
            document.getElementById(names[i] + "0").textContent = w.load.toFixed(0);
            document.getElementById(names[i] + "1").textContent = w.fxz.toFixed(0);
            document.getElementById(names[i] + "2").textContent = ((w.deflection || 0) * 1000).toFixed(1);
            document.getElementById(names[i] + "3").textContent = w.contacts;
        }
        const weight = (Car.Mass + 4 * Car.UnsprungMass) * Gravity;
        document.getElementById("sum").textContent = (sum / weight).toFixed(3);
        document.getElementById("sumBar").style.width = Math.min(100, sum / weight * 50) + "%";
    }
}

// ------------------------------------------------------------------------------------------------------------------------
//                                                          MAIN
// ------------------------------------------------------------------------------------------------------------------------

async function main()
{
    const status = document.getElementById("status");
    const say = (text) => { status.textContent = text; };

    if (!navigator.gpu) { say("WebGPU is not available in this browser. Chrome 113+, Edge 113+, Safari 18+ or Firefox 141+ with WebGPU enabled."); return; }
    const adapter = await navigator.gpu.requestAdapter({ powerPreference: "high-performance" });
    if (!adapter) { say("No WebGPU adapter was offered."); return; }
    const features = [];
    if (adapter.features.has("timestamp-query")) features.push("timestamp-query");
    const device = await adapter.requestDevice({ requiredFeatures: features,
        requiredLimits: { maxStorageBufferBindingSize: Math.min(adapter.limits.maxStorageBufferBindingSize, 1 << 30),
                          maxBufferSize: Math.min(adapter.limits.maxBufferSize, 1 << 30) } });
    device.lost.then((info) => say("device lost: " + info.message));
    device.addEventListener("uncapturederror", (e) => say("WebGPU error: " + e.error.message));

    say("loading lattice and kernels…");
    const base = new URL(".", import.meta.url);
    const one = await LoadLattice(new URL("TyreLattice.bin", base));
    const wgsl = {};
    for (const name of EntryPoints) wgsl[name] = await (await fetch(new URL(`Shader/${name}.wgsl`, base))).text();

    const canvas = document.getElementById("view");
    const preview = new Preview(device, canvas, one, wgsl);
    preview.buildFleet();
    say("");

    const bind = (id, key, scale, rebuild) => {
        const input = document.getElementById(id), label = document.getElementById(id + "Value");
        input.addEventListener("input", () => { label.textContent = input.value; });
        input.addEventListener("change", () => {
            preview.settings[key] = +input.value * scale;
            if (rebuild === "fleet") preview.buildFleet(); else if (rebuild === "steps") preview.layoutSteps();
        });
    };
    bind("cars", "cars", 1, "fleet");
    bind("speed", "speed", 1);
    bind("substeps", "substeps", 1, "steps");
    bind("pressure", "pressure", 1);
    document.getElementById("watch").addEventListener("change", (e) => { preview.watch = +e.target.value; });
    const followButton = document.getElementById("follow");
    const toggleFollow = () => { preview.camera.follow = !preview.camera.follow; followButton.textContent = "follow: " + (preview.camera.follow ? "on" : "off"); };
    followButton.addEventListener("click", toggleFollow);
    const pauseButton = document.getElementById("pause");
    const togglePause = () => { preview.paused = !preview.paused; pauseButton.textContent = preview.paused ? "resume" : "pause"; };
    pauseButton.addEventListener("click", togglePause);
    window.addEventListener("keydown", (e) => { if (e.key === "f" || e.key === "F") toggleFollow(); if (e.key === " ") { e.preventDefault(); togglePause(); } });

    let dragging = false, last = [0, 0];
    canvas.addEventListener("pointerdown", (e) => { dragging = true; last = [e.clientX, e.clientY]; canvas.setPointerCapture(e.pointerId); });
    canvas.addEventListener("pointerup", () => { dragging = false; });
    canvas.addEventListener("pointermove", (e) => {
        if (!dragging) return;
        preview.camera.yaw -= (e.clientX - last[0]) * 0.005;
        preview.camera.pitch = Math.max(0.05, Math.min(1.4, preview.camera.pitch + (e.clientY - last[1]) * 0.005));
        last = [e.clientX, e.clientY];
    });
    canvas.addEventListener("wheel", (e) => { preview.camera.distance = Math.max(1.5, Math.min(120, preview.camera.distance * Math.exp(e.deltaY * 0.001))); e.preventDefault(); }, { passive: false });

    const loop = (now) => { preview.frame(now); requestAnimationFrame(loop); };
    requestAnimationFrame(loop);
}

export { LoadLattice, TileLattice, PlaceTyre, WriteHub, DeclareInflation, RoadHeight, Vehicle, Car, LaneRadii, BuildTyreIndices };

if (typeof document !== "undefined")
    main().catch((e) => { document.getElementById("status").textContent = String(e.stack || e); console.error(e); });
