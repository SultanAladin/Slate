//============================================================================================================================================
//                                                           TOOLSEQUENCE.CPP
//============================================================================================================================================
// 🧩 Tool declaration, the arbitration both units ask, and the capture that persists for a whole drag.

#include "SlateDocument/Document/ToolSequence/Api/ToolSequence.h"

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                        THE TOOLS
//------------------------------------------------------------------------------------------------------------------------

Result<std::uint32_t> ToolIndex::Declare(const ToolSpecification& Declaring)
{
    if (Declaring.Identity.empty())
        return Result<std::uint32_t>::Refuse({ RefusalReason::ContentUnsupported, "a tool declares no identity" });

    if (Declaring.Claimed  == PointerPrecedence::PrecedenceCount
     || Declaring.Previewed == PreviewSubject::PreviewCount
     || Declaring.Recorded  == TransactionSubject::SubjectCount)
    {
        return Result<std::uint32_t>::Refuse(
            { RefusalReason::ContentUnsupported, "no such precedence, preview or transaction shape" });
    }

    // 📝 A repeated identity is refused rather than replacing the standing declaration. `Located` below resolves
    //    by identity, and two tools sharing one would make the resolution answer whichever was declared first —
    //    which is a tool the artist activates and does not get.
    for (const ToolSpecification& Held : Declared)
    {
        if (Held.Identity == Declaring.Identity)
        {
            return Result<std::uint32_t>::Refuse(
                { RefusalReason::ContentUnsupported, "a tool already declares that identity" });
        }
    }

    if (Declared.size() >= ToolCeiling)
        return Result<std::uint32_t>::Refuse({ RefusalReason::ExtentExhausted, "the tool ceiling was reached" });

    const std::uint32_t ToolOrdinal = static_cast<std::uint32_t>(Declared.size());

    Declared.push_back(Declaring);

    return Result<std::uint32_t>::Result(ToolOrdinal);
}

Result<const ToolSpecification*> ToolIndex::Resolve(std::uint32_t ToolOrdinal) const
{
    if (ToolOrdinal >= Declared.size())
        return Result<const ToolSpecification*>::Refuse({ RefusalReason::ContentUnsupported, "no such tool" });

    return Result<const ToolSpecification*>::Result(&Declared[ToolOrdinal]);
}

Result<ToolSpecification*> ToolIndex::Amend(std::uint32_t ToolOrdinal)
{
    if (ToolOrdinal >= Declared.size())
        return Result<ToolSpecification*>::Refuse({ RefusalReason::ContentUnsupported, "no such tool" });

    return Result<ToolSpecification*>::Result(&Declared[ToolOrdinal]);
}

Result<std::uint32_t> ToolIndex::Located(const std::string& Identity) const
{
    for (std::size_t Ordinal = 0u; Ordinal < Declared.size(); ++Ordinal)
    {
        if (Declared[Ordinal].Identity == Identity)
            return Result<std::uint32_t>::Result(static_cast<std::uint32_t>(Ordinal));
    }

    return Result<std::uint32_t>::Refuse({ RefusalReason::ContentUnsupported, "nothing declares that tool" });
}

std::uint32_t ToolIndex::DeclaredCount() const
{
    return static_cast<std::uint32_t>(Declared.size());
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE DECLARATIONS
//------------------------------------------------------------------------------------------------------------------------

ToolIndex&        ToolSequence::Tools()         { return DeclaredTools;   }
const ToolIndex&  ToolSequence::Tools() const   { return DeclaredTools;   }
BrushIndex&       ToolSequence::Brushes()       { return DeclaredBrushes; }
const BrushIndex& ToolSequence::Brushes() const { return DeclaredBrushes; }

Result<bool> ToolSequence::DeclareTool(std::uint32_t ToolOrdinal_)
{
    if (!DeclaredTools.Resolve(ToolOrdinal_).Resolved)
        return Result<bool>::Refuse({ RefusalReason::ContentUnsupported, "no such tool" });

    ToolOrdinal = ToolOrdinal_;

    return Result<bool>::Result(true);
}

Result<bool> ToolSequence::DeclareBrush(std::uint32_t BrushOrdinal_)
{
    if (!DeclaredBrushes.Resolve(BrushOrdinal_).Resolved)
        return Result<bool>::Refuse({ RefusalReason::ContentUnsupported, "no such brush" });

    BrushOrdinal = BrushOrdinal_;

    return Result<bool>::Result(true);
}

Result<bool> ToolSequence::DeclareColour(const ColourSpecification& Declaring)
{
    // 🔴 `36` §1: a colour without its space is refused rather than assumed to be in the working space. An
    //    assumed space here is the defect `36` exists to prevent, placed where every stroke reads it.
    if (!Declaring.ColourDeclared())
        return Result<bool>::Refuse({ RefusalReason::ContentUnsupported, "the colour declares no space" });

    ActiveColour = Declaring;

    return Result<bool>::Result(true);
}

Result<bool> ToolSequence::DeclareDisplay(DisplaySubject Declaring)
{
    if (Declaring == DisplaySubject::DisplayCount)
        return Result<bool>::Refuse({ RefusalReason::ContentUnsupported, "no such display mode" });

    PresentedDisplay = Declaring;

    return Result<bool>::Result(true);
}

Result<bool> ToolSequence::DeclareChannel(ChannelSubject Declaring)
{
    if (Declaring == ChannelSubject::ChannelCount)
        return Result<bool>::Refuse({ RefusalReason::ContentUnsupported, "no such channel" });

    PresentedChannel = Declaring;

    return Result<bool>::Result(true);
}

Result<bool> ToolSequence::DeclareOverlay(OverlaySubject Declaring, bool PresenceEnabled)
{
    if (Declaring == OverlaySubject::OverlayCount)
        return Result<bool>::Refuse({ RefusalReason::ContentUnsupported, "no such overlay" });

    OverlayPresent[static_cast<std::size_t>(Declaring)] = PresenceEnabled;

    return Result<bool>::Result(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     POINTER ARBITRATION
//------------------------------------------------------------------------------------------------------------------------

PointerPrecedence ToolSequence::Arbitrate(bool InterfaceReported,
                                          bool ManipulatorOpen,
                                          bool StrokeOpen) const
{
    // 🔴 A standing capture answers unconditionally. `14` §4.2: capture persists for the whole drag, so a drag
    //    that began on a manipulator handle continues to address that handle after the cursor leaves the
    //    workspace, and a stroke is not stolen by a panel it passes under.
    if (StandingCapture.CaptureDeclared)
        return StandingCapture.Holder;

    if (InterfaceReported)
        return PointerPrecedence::Interface;

    if (ManipulatorOpen)
        return PointerPrecedence::Manipulator;

    if (StrokeOpen)
        return PointerPrecedence::Stroke;

    return PointerPrecedence::Workspace;
}

Result<bool> ToolSequence::OpenCapture(PointerPrecedence Claiming, const ResolvedPointer& Opened)
{
    if (Claiming == PointerPrecedence::PrecedenceCount)
        return Result<bool>::Refuse({ RefusalReason::ContentUnsupported, "no such precedence" });

    // 🔴 A stronger claimant does **not** steal a standing capture. Arbitration happens once, before capture is
    //    taken; re-arbitrating mid-drag is the defect where a stroke stops the moment the cursor crosses a
    //    floating panel, and it is exactly the case `14` §4.2 declares against.
    if (StandingCapture.CaptureDeclared)
        return Result<bool>::Refuse({ RefusalReason::HostDenied, "a capture already stands" });

    StandingCapture.Holder          = Claiming;
    StandingCapture.Opened          = Opened;
    StandingCapture.CaptureDeclared = true;

    return Result<bool>::Result(true);
}

Result<bool> ToolSequence::ReleaseCapture()
{
    if (!StandingCapture.CaptureDeclared)
        return Result<bool>::Refuse({ RefusalReason::HostDenied, "no capture stands" });

    StandingCapture = PointerCapture{};

    return Result<bool>::Result(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       WHAT IS READ
//------------------------------------------------------------------------------------------------------------------------

const ColourSpecification& ToolSequence::Colour() const  { return ActiveColour;    }
const PointerCapture&      ToolSequence::Capture() const { return StandingCapture; }

Result<const ToolSpecification*> ToolSequence::ActiveTool() const
{
    if (ToolOrdinal == AbsentTool)
    {
        return Result<const ToolSpecification*>::Refuse(
            { RefusalReason::ContentUnsupported, "no tool is active" });
    }

    return DeclaredTools.Resolve(ToolOrdinal);
}

Result<const BrushSpecification*> ToolSequence::ActiveBrush() const
{
    if (BrushOrdinal == AbsentTool)
    {
        return Result<const BrushSpecification*>::Refuse(
            { RefusalReason::ContentUnsupported, "no brush is active" });
    }

    return DeclaredBrushes.Resolve(BrushOrdinal);
}

std::uint32_t  ToolSequence::ActiveToolOrdinal() const  { return ToolOrdinal;      }
std::uint32_t  ToolSequence::ActiveBrushOrdinal() const { return BrushOrdinal;     }
DisplaySubject ToolSequence::Display() const            { return PresentedDisplay; }
ChannelSubject ToolSequence::IsolatedChannel() const    { return PresentedChannel; }

bool ToolSequence::OverlayStanding(OverlaySubject Subject) const
{
    return Subject != OverlaySubject::OverlayCount
        && OverlayPresent[static_cast<std::size_t>(Subject)];
}

}   // namespace Slate
