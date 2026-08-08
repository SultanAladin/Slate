//============================================================================================================================================
//                                                            IDENTITYCONTRACT.H
//============================================================================================================================================
// 🧩 Generational slot identity, tagged per subject so one subject's identity never passes for another's.

#pragma once

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                     SUBJECT TAGS
//------------------------------------------------------------------------------------------------------------------------

// 📝 Each tag exists only to make Identity<Subject> a distinct type. A PartitionIdentity passed where an
//    OccupantIdentity is expected is a compile error, which is the whole reason the tags are declared.
struct OccupantSubject  {};
struct PartitionSubject {};
struct SurfaceSubject   {};
struct RecordingSubject {};

//------------------------------------------------------------------------------------------------------------------------
//                                                       IDENTITY
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 A slot ordinal paired with the generation the slot held when the reference was taken.
/// note  Identity is Exact. It is an unsigned integer pair, never a real number and never narrowed.
/// note  A generation of zero declares the reference absent; issued generations begin at one.
/// tag   contract, nonallocating, nonthrowing
template <typename Subject>
struct Identity
{
    std::uint32_t  SlotOrdinal    = 0u;   // [-] - index into the slot ledger that issued it
    std::uint32_t  SlotGeneration = 0u;   // [-] - zero declares the reference absent

    /// 🧩 Whether this reference names a slot at all.
    /// out   Declared   [-]  false for a default-constructed identity
    /// cost  ✔️
    constexpr bool IdentityDeclared() const
    {
        return SlotGeneration != 0u;
    }
};

/// 🧩 Two identities of the same subject match when both halves match.
/// cost  ✔️
template <typename Subject>
constexpr bool operator==(Identity<Subject> LeftIdentity, Identity<Subject> RightIdentity)
{
    return LeftIdentity.SlotOrdinal    == RightIdentity.SlotOrdinal
        && LeftIdentity.SlotGeneration == RightIdentity.SlotGeneration;
}

template <typename Subject>
constexpr bool operator!=(Identity<Subject> LeftIdentity, Identity<Subject> RightIdentity)
{
    return !(LeftIdentity == RightIdentity);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                THE DECLARED SUBJECTS
//------------------------------------------------------------------------------------------------------------------------

using OccupantIdentity  = Identity<OccupantSubject>;    // [-] - one occupant of the document population
using PartitionIdentity = Identity<PartitionSubject>;   // [-] - one partition of one occupant's topology
using SurfaceIdentity   = Identity<SurfaceSubject>;     // [-] - one paintable surface domain
using RecordingIdentity = Identity<RecordingSubject>;   // [-] - one slot of the recording rotation

}   // namespace Slate
