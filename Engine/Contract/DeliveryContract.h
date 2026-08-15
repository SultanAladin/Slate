//============================================================================================================================================
//                                                            DELIVERYCONTRACT.H
//============================================================================================================================================
// 🧩 Absence that carries a reason, and a convergent result that reports which criterion terminated it.

#pragma once

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                       REFUSAL
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Why a computation declined to deliver content. Absence without one of these is never reported.
/// tag   contract
enum class RefusalReason : std::uint32_t
{
    CapabilityAbsent    = 0u,   // [-] - the vendor capability the mechanism requires was not negotiated
    ExtentExhausted     = 1u,   // [-] - a reserved or committed claim could not be satisfied in full
    IdentityStale       = 2u,   // [-] - the generation held by the reference no longer occupies the slot
    ContentUnsupported  = 3u,   // [-] - the intake subset does not accept what the stream contained
    VersionUnmigratable = 4u,   // [-] - no declared migration reaches the current stream version
    HostDenied          = 5u,   // [-] - the operating system declined the request
    RelationCyclic      = 6u,   // [-] - the relation change would close a cycle; never applied
    DeviceLost          = 7u    // [-] - the device was lost; nothing it holds is valid to destroy or reuse
};

// 📝 🔴 RelationCyclic exists because `12` §9 requires a cycle-creating relation change to be rejected at
//    commit and reported as a Refusal a reader can discriminate. Spelling it HostDenied would have put a
//    document-model rejection behind the operating system's reason, and `86` presents that reason verbatim.
//    Both operands are the rejecting call's own arguments, so the caller names them without allocating.

// 📝 🔴 DeviceLost exists for the same reason at the vendor edge. `06` §7 requires device loss to be reported
//    upward before anything is destroyed, and `06` §4.2's recovery is a different response from an ordinary
//    refusal — the device is recreated and the capability set re-scored, rather than the call being retried.
//    A caller cannot tell those apart from HostDenied and prose, so the six sites that can observe the loss
//    report it as this reason and destroy nothing themselves.

/// 🧩 One reported refusal — the reason, plus static text naming the operand it applies to.
/// note  Detail points at string literal storage only. Nothing here ever owns an allocation.
/// tag   contract, nonallocating
struct Refusal
{
    RefusalReason  DeclaredReason = RefusalReason::HostDenied;   // [-] - the discriminating reason
    const char*    Detail         = "";                          // [-] - static text, never allocated
};

//------------------------------------------------------------------------------------------------------------------------
//                                                       DELIVERY
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Delivered content, or a refusal naming why it is absent. Used wherever a document reports a refusal.
/// note  ⚠️ Delivered is default-constructed when ContentPresent is false. Read it only through Resolve.
/// tag   contract, nonallocating, nonthrowing
template <typename Content>
struct ContentDelivery
{
    Content  Delivered      = Content{};   // [-] - meaningful only while ContentPresent holds
    Refusal  Declined       = {};          // [-] - meaningful only while ContentPresent is false
    bool     ContentPresent = false;       // [-] - the discriminant, and the only thing read first

    /// 🧩 Constructs a delivered result around content the computation produced.
    /// in    Produced   [-]  the content to deliver
    /// out   Deliver    [-]  ContentPresent holds
    /// cost  ✔️
    static constexpr ContentDelivery Deliver(const Content& Produced)
    {
        ContentDelivery Constructed;
        Constructed.Delivered      = Produced;
        Constructed.ContentPresent = true;
        return Constructed;
    }

    /// 🧩 Constructs a refused result carrying the reason the content is absent.
    /// in    Declining  [-]  the reason and the operand it applies to
    /// out   Deliver    [-]  ContentPresent is false
    /// cost  ✔️
    static constexpr ContentDelivery Refuse(const Refusal& Declining)
    {
        ContentDelivery Constructed;
        Constructed.Declined       = Declining;
        Constructed.ContentPresent = false;
        return Constructed;
    }

    /// 🧩 Reads the delivered content.
    /// out   Content    [-]  the produced content
    /// pre   ContentPresent holds
    /// cost  ✔️
    constexpr const Content& Resolve() const
    {
        return Delivered;
    }
};

/// 🧩 Public spelling for content delivered or refused across one fallible call.
/// note  An alias permits `Deliver<Content>::Deliver(Produced)`: a class named Deliver cannot declare a static
///       function carrying its own name because C++ reserves that spelling for its constructor.
/// tag   contract, nonallocating, nonthrowing
template <typename Content>
using Deliver = ContentDelivery<Content>;

//------------------------------------------------------------------------------------------------------------------------
//                                                  CONVERGENT RESULT
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Which of the two terminating conditions ended an iteration.
/// note  A solver that returns its last iterate at the ceiling is otherwise indistinguishable from one
///       that converged, and the ambiguity propagates upward as an unexplained artefact.
/// tag   contract
enum class TerminationCause : std::uint32_t
{
    CriterionSatisfied = 0u,   // [-] - the declared convergence criterion was met
    CeilingReached     = 1u    // [-] - the declared iteration ceiling was reached first
};

/// 🧩 The result of a Convergent computation. Never returned as a bare approximation.
/// tag   contract, nonallocating, nonthrowing
template <typename Content>
struct ConvergentResult
{
    Content           Approximation  = Content{};                       // [-]  - the last iterate produced
    double            ResidualNorm   = 0.0;                             // [-]  - ‖r‖ measured at termination
    std::uint32_t     IterationCount = 0u;                              // [-]  - iterations actually taken
    TerminationCause  Cause          = TerminationCause::CeilingReached; // [-] - which condition terminated
};

}   // namespace Slate
