//============================================================================================================================================
//                                                             INPUTEXCHANGE.H
//============================================================================================================================================
// 🧩 Timestamped device samples crossing in, with absent axes distinguishable from zero-valued ones.

#pragma once

#include "SlateMath/Platform/TickSequence/Api/TickSequence.h"

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    AXIS PRESENCE
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Which optional axes the reporting device supplied on one sample.
/// note  🔴 Absent is distinct from zero. A tablet reporting no tilt and a stylus held perfectly upright
///       are different facts, and `22` treats them differently.
/// tag   nonallocating, nonthrowing
struct AxisPresence
{
    bool  PressureReported = false;   // [-] - the device supplied a pressure reading
    bool  TiltReported     = false;   // [-] - the device supplied both tilt angles
    bool  RotationReported = false;   // [-] - the device supplied a barrel rotation
};

//------------------------------------------------------------------------------------------------------------------------
//                                                      ONE SAMPLE
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 One pointer sample, stamped at arrival by `TickSequence`.
/// note  Stamped at arrival, never at consumption. A stroke sampled at device rate and consumed at display
///       rate reconstructs only if the arrival stamps survive.
/// tag   nonallocating, nonthrowing
struct PointerSample
{
    TickPoint     Arrival     = {};       // [ns]  - stamped by TickSequence when the sample crossed in
    double        PositionX   = 0.0;      // [px]  - in the window's drawable extent
    double        PositionY   = 0.0;      // [px]  - in the window's drawable extent
    double        Pressure    = 0.0;      // [-]   - normalised; meaningful only when reported
    double        TiltAlong   = 0.0;      // [deg] - meaningful only when reported
    double        TiltAcross  = 0.0;      // [deg] - meaningful only when reported
    double        Rotation    = 0.0;      // [deg] - barrel rotation; meaningful only when reported
    AxisPresence  Supplied    = {};       // [-]   - read this before reading any optional axis above
    std::uint32_t ContactMask = 0u;       // [-]   - bit per pointer contact currently down
};

//------------------------------------------------------------------------------------------------------------------------
//                                                 THE ARRIVAL SEQUENCE
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The bounded arrival ordering of pointer samples, drained once per tick by the consumer.
/// note  ⏱️ Bounded and non-allocating: the oldest sample is discarded when the extent is full. A stroke
///       that outruns the drain loses its oldest samples, which is visible, rather than allocating during
///       an interaction, which is not.
/// tag   owning
class InputExchange
{
public:

    static constexpr std::uint32_t ArrivalCapacity = 4096u;   // [-] - samples held between two drains

    /// 🧩 Records one arriving sample against the supplied timeline.
    /// in    Arriving   [-]  the sample as the device reported it
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    void Record(const PointerSample& Arriving);

    /// 🧩 Reads one held sample in arrival order.
    /// in    ArrivalOrdinal [-]  zero is the oldest sample still held
    /// pre   ArrivalOrdinal is below HeldCount
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    const PointerSample& Sample(std::uint32_t ArrivalOrdinal) const;

    /// 🧩 How many samples are held.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    std::uint32_t HeldCount() const;

    /// 🧩 Discards every held sample. Called by the consumer once it has read them.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    void Reclaim();

private:

    PointerSample  ArrivalOrder[ArrivalCapacity] = {};   // [-] - cyclic; oldest discarded when full
    std::uint32_t  OldestOrdinal                 = 0u;   // [-] - where the oldest held sample sits
    std::uint32_t  OccupiedCount                 = 0u;   // [-] - how many are held
};

}   // namespace Slate
