//============================================================================================================================================
//                                                          INTERACTIONINDEX.CPP
//============================================================================================================================================
// 🧩 The one grab, the one open popup, and the two fades every registered control carries.

#include "SlateUI/Interface/InteractionIndex/Api/InteractionIndex.h"

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                        CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> InteractionIndex::Construct(MotionIntegrator& Incoming)
{
    if (Motion != nullptr)
    {
        return Outcome<bool>::Refuse(Refusal{ RefusalReason::ContentUnsupported,
                                                   "InteractionIndex is already constructed" });
    }

    Motion = &Incoming;

    return Outcome<bool>::Result(true);
}

Outcome<ControlIdentity> InteractionIndex::Register()
{
    if (Motion == nullptr)
    {
        return Outcome<ControlIdentity>::Refuse(Refusal{ RefusalReason::CapabilityAbsent,
                                                          "InteractionIndex was not constructed" });
    }

    if (RegisteredSlots >= ControlCapacity)
    {
        return Outcome<ControlIdentity>::Refuse(Refusal{ RefusalReason::ExtentExhausted,
                                                          "InteractionIndex holds no further control slot" });
    }

    // 📝 🔴 Both fades are registered before the slot is claimed. Target first and refusing second would leave
    //    a slot registered against an interpolant that does not exist, and every later read of it would return
    //    the ordinal zero — which is another control's fade.
    const Outcome<std::uint32_t> HoverRegistered = Motion->RegisterEased(0.0);

    if (!HoverRegistered.Resolved)
    {
        return Outcome<ControlIdentity>::Refuse(Refusal{ RefusalReason::ExtentExhausted,
                                                          "the integrator rejected a hover fade" });
    }

    const Outcome<std::uint32_t> TakeRegistered = Motion->RegisterEased(0.0);

    if (!TakeRegistered.Resolved)
    {
        return Outcome<ControlIdentity>::Refuse(Refusal{ RefusalReason::ExtentExhausted,
                                                          "the integrator rejected a take fade" });
    }

    const std::uint32_t Target = RegisteredSlots;
    ++RegisteredSlots;

    // 📝 The generation rises with every Reset and never falls, so an identity registered before one resolves
    //    false afterwards rather than naming whatever now occupies its ordinal.
    Generations[Target] = RegisteredGeneration + 1u;

    Poses[Target].HoverOrdinal = HoverRegistered.Resolve();
    Poses[Target].TakeOrdinal  = TakeRegistered.Resolve();
    Poses[Target].Registered     = true;

    return Outcome<ControlIdentity>::Result(ControlIdentity{ Target, Generations[Target] });
}

std::uint32_t InteractionIndex::Slot(ControlIdentity Target) const
{
    if (!Target.IdentityDeclared() || Target.SlotOrdinal >= RegisteredSlots)
        return ControlCapacity;

    if (Generations[Target.SlotOrdinal] != Target.SlotGeneration)
        return ControlCapacity;

    return Target.SlotOrdinal;
}

bool InteractionIndex::Resolves(ControlIdentity Target) const
{
    return Slot(Target) != ControlCapacity;
}

std::uint32_t InteractionIndex::RegisteredCount() const
{
    return RegisteredSlots;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                          THE TICK
//------------------------------------------------------------------------------------------------------------------------

void InteractionIndex::Advance(const PointerCondition& Sampled, double Elapsed)
{
    static_cast<void>(Elapsed);

    // 📝 The tick's pointer is retained so that Grab can stamp the arrival ordinates without every control
    //    having to pass them in — and, more to the point, without a control being able to pass in a position
    //    it computed rather than the one the window system reported.
    SampledX  = Sampled.PositionX;
    SampledY = Sampled.PositionY;

    // 📝 🔴 The release is retired here and not at the seizing control. A control whose extent left the
    //    arrangement between two ticks never runs again, and a grab retired only by that control would
    //    stand for the life of the process — every later contact rejected because something invisible holds it.
    ReleasedControl = {};
    ReleasedPart    = ControlPart::Nothing;

    if (GrabbedPart != ControlPart::Nothing && !Sampled.ContactHeld)
    {
        ReleasedControl    = GrabbedControl;
        ReleasedPart       = GrabbedPart;
        GrabbedControl      = {};
        GrabbedPart         = ControlPart::Nothing;
        GrabbedInitial     = 0.0f;
        InitialRecorded   = false;
    }
}

bool InteractionIndex::Grab(ControlIdentity Target, ControlPart Part)
{
    if (Part == ControlPart::Nothing || Slot(Target) == ControlCapacity)
        return false;

    // 🔴 One grab. A second claim while one stands is rejected rather than replacing it, which is what
    //    keeps a drag addressing the control it began on when the pointer crosses a neighbour.
    if (GrabbedPart != ControlPart::Nothing)
        return false;

    GrabbedControl      = Target;
    GrabbedPart         = Part;
    GrabbedOriginX  = SampledX;
    GrabbedOriginY = SampledY;
    GrabbedInitial     = 0.0f;
    InitialRecorded   = false;

    return true;
}

bool InteractionIndex::Holding(ControlIdentity Target) const
{
    return GrabbedPart != ControlPart::Nothing && GrabbedControl == Target && Slot(Target) != ControlCapacity;
}

ControlPart InteractionIndex::HeldPart(ControlIdentity Target) const
{
    return Holding(Target) ? GrabbedPart : ControlPart::Nothing;
}

bool InteractionIndex::Released(ControlIdentity Target) const
{
    return ReleasedControl.IdentityDeclared() && ReleasedControl == Target;
}

ControlPart InteractionIndex::ReleasedControlPart(ControlIdentity Target) const
{
    return Released(Target) ? ReleasedPart : ControlPart::Nothing;
}

bool InteractionIndex::RecordInitial(ControlIdentity Target, float Coordinate)
{
    if (!Holding(Target))
        return false;

    GrabbedInitial   = Coordinate;
    InitialRecorded = true;

    return true;
}

Outcome<float> InteractionIndex::InitialReading(ControlIdentity Target) const
{
    if (!Holding(Target) || !InitialRecorded)
    {
        return Outcome<float>::Refuse(Refusal{ RefusalReason::IdentityStale,
                                                       "this control holds no grab to depart from" });
    }

    return Outcome<float>::Result(GrabbedInitial);
}

float InteractionIndex::OriginX() const
{
    return GrabbedOriginX;
}

float InteractionIndex::OriginY() const
{
    return GrabbedOriginY;
}

void InteractionIndex::Abandon()
{
    GrabbedControl    = {};
    GrabbedPart       = ControlPart::Nothing;
    ReleasedControl  = {};
    ReleasedPart     = ControlPart::Nothing;
    GrabbedInitial   = 0.0f;
    InitialRecorded = false;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE DISCLOSURE
//------------------------------------------------------------------------------------------------------------------------

bool InteractionIndex::Disclose(ControlIdentity Target)
{
    if (Slot(Target) == ControlCapacity)
        return false;

    // 🔴 Whatever stood before is closed by the assignment itself. Two open popups cannot be represented.
    DisclosedControl = Target;

    return true;
}

void InteractionIndex::Withdraw()
{
    DisclosedControl = {};
}

bool InteractionIndex::Disclosed(ControlIdentity Target) const
{
    return DisclosedControl.IdentityDeclared() && DisclosedControl == Target
        && Slot(Target) != ControlCapacity;
}

bool InteractionIndex::AnyDisclosed() const
{
    return DisclosedControl.IdentityDeclared();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                         THE FADES
//------------------------------------------------------------------------------------------------------------------------

namespace
{

/// 🧩 Departs one eased traverse toward a declared pose, if it is not already heading there.
/// note  📐 Previous from where it **stands**, never from zero or one. A fade re-departed from its endpoint
///        jumps backward the moment the pointer crosses an edge twice inside one duration, which is exactly
///        what a pointer travelling along a column of rows does.
/// cost  ✔️
bool DepartToward(MotionIntegrator& Motion, std::uint32_t Ordinal, bool Toward, double Duration,
                  EaseCurve Shape = EaseCurve::Standard)
{
    EasedInterpolant& Fade    = Motion.Eased(Ordinal);
    const double      Arrival = Toward ? 1.0 : 0.0;

    if (Fade.Settled && Fade.Current() == Arrival)
        return false;

    if (!Fade.Settled && Fade.Incoming == Arrival)
        return false;

    Fade.Depart(Fade.Current(), Arrival, Duration, 0.0, Shape);

    return true;
}

}   // namespace

bool InteractionIndex::DeclareHovered(ControlIdentity Target, bool Hovered, double Duration)
{
    const std::uint32_t Ordinal = Slot(Target);

    if (Ordinal == ControlCapacity || Motion == nullptr || !Poses[Ordinal].Registered)
        return false;

    return DepartToward(*Motion, Poses[Ordinal].HoverOrdinal, Hovered, Duration);
}

bool InteractionIndex::DeclareTaken(ControlIdentity Target, bool Taken, double Duration, EaseCurve Shape)
{
    const std::uint32_t Ordinal = Slot(Target);

    if (Ordinal == ControlCapacity || Motion == nullptr || !Poses[Ordinal].Registered)
        return false;

    return DepartToward(*Motion, Poses[Ordinal].TakeOrdinal, Taken, Duration, Shape);
}

float InteractionIndex::HoveredFraction(ControlIdentity Target) const
{
    const std::uint32_t Ordinal = Slot(Target);

    if (Ordinal == ControlCapacity || Motion == nullptr || !Poses[Ordinal].Registered)
        return 0.0f;

    return static_cast<float>(Motion->Eased(Poses[Ordinal].HoverOrdinal).Current());
}

float InteractionIndex::TakenFraction(ControlIdentity Target) const
{
    const std::uint32_t Ordinal = Slot(Target);

    if (Ordinal == ControlCapacity || Motion == nullptr || !Poses[Ordinal].Registered)
        return 0.0f;

    return static_cast<float>(Motion->Eased(Poses[Ordinal].TakeOrdinal).Current());
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        RETIREMENT
//------------------------------------------------------------------------------------------------------------------------

void InteractionIndex::Reset()
{
    // 🔴 The generation rises past every ordinal ever registered, so no identity from before this call can ever
    //    resolve again. Rewinding it to zero would make a stale identity name a fresh slot silently.
    ++RegisteredGeneration;

    for (std::uint32_t Ordinal = 0u; Ordinal < RegisteredSlots; ++Ordinal)
    {
        Generations[Ordinal] = 0u;
        Poses[Ordinal]       = ControlPose{};
    }

    RegisteredSlots    = 0u;
    DisclosedControl = {};

    Abandon();
}

}   // namespace Slate
