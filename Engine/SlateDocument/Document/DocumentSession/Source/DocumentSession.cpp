//============================================================================================================================================
//                                                           DOCUMENTSESSION.CPP
//============================================================================================================================================
// 🧩 `48` §1 — one open document and everything true of it only while it is open, plus every open session at once.

#include "SlateDocument/Document/DocumentSession/Api/DocumentSession.h"

#include <cstddef>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    WHAT IT HOLDS
//------------------------------------------------------------------------------------------------------------------------

OutlinerSequence& DocumentSession::Document()
{
    return Population;
}

const OutlinerSequence& DocumentSession::Document() const
{
    return Population;
}

ReferenceIndex& DocumentSession::References()
{
    return External;
}

const ReferenceIndex& DocumentSession::References() const
{
    return External;
}

RecoverySequence& DocumentSession::Journal()
{
    return Recovery;
}

const RecoverySequence& DocumentSession::Journal() const
{
    return Recovery;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE LOCATION
//------------------------------------------------------------------------------------------------------------------------

Result<bool> DocumentSession::DeclareStorage(const std::string& DeclaredPath, const std::string& JournalPath)
{
    const Result<bool> Paired = Recovery.DeclareDocument(DeclaredPath, JournalPath);

    if (!Paired.Resolved)
    {
        return Paired;
    }

    StoragePath     = DeclaredPath;
    StorageDeclared = StorageStanding::Declared;

    return Result<bool>::Result(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE SEALING
//------------------------------------------------------------------------------------------------------------------------

Result<SealedContent> DocumentSession::Seal(const std::vector<std::uint8_t>& Encoded, std::uint64_t SealedAt) const
{
    if (StorageDeclared != StorageStanding::Declared)
    {
        return Result<SealedContent>::Refuse(
            { RefusalReason::ContentUnsupported, "this session has no storage location to save to — `48` §2" });
    }

    // 🔴 `48` §3: a save reads sealed state. An open transaction refuses the capture rather than being sealed
    //    by it — a half-finished drag is not a state the artist asked to keep, and sealing it on their behalf
    //    puts an edit they had not decided on into `RevisionSequence` where they meet it only afterwards.
    if (Population.Revisions().TransactionOpen())
    {
        return Result<SealedContent>::Refuse(
            { RefusalReason::ExtentExhausted, "a transaction is open; a save reads sealed state only — `48` §3" });
    }

    SealedContent Capturing;
    Capturing.Content       = Encoded;
    Capturing.TargetPath    = StoragePath;
    Capturing.SavedThrough  = Population.Revisions().ScrubPosition();
    Capturing.SealedAt      = SealedAt;
    Capturing.StreamVersion = CurrentStreamVersion;

    // 📝 The revision ordinal is the **scrub position** and not the committed count. An artist who undoes three
    //    transactions and saves has saved the document they are looking at, and a journal retired against the
    //    committed count would discard the three the file does not carry.
    return Result<SealedContent>::Result(Capturing);
}

void DocumentSession::DeclareSaved(const PersistenceConclusion& Concluded)
{
    if (Concluded.Reached != PersistenceStep::Replaced) { return; }

    SavedRevision      = Concluded.SavedThrough;
    SavedStamp         = Concluded.SavedAt;
    AmendmentsDeclared = false;

    // 📝 `48` §3 ④, run here because the journal belongs to the session and the save ran off the tick. What
    //    remains past the save is exactly what a crash after this point would have to replay.
    Recovery.Retire(Concluded.SavedThrough);
}

void DocumentSession::DeclareAmended()
{
    AmendmentsDeclared = true;
}

bool DocumentSession::AmendmentsStanding() const
{
    return AmendmentsDeclared;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     SESSION STATE
//------------------------------------------------------------------------------------------------------------------------

void DocumentSession::DeclarePresentedCamera(OccupantIdentity Presenting)
{
    Presented = Presenting;
}

OccupantIdentity DocumentSession::PresentedCamera() const
{
    return Presented;
}

void DocumentSession::DeclareScrollPosition(std::uint32_t VisiblePosition)
{
    ScrollVisible = VisiblePosition;
}

std::uint32_t DocumentSession::ScrollPosition() const
{
    return ScrollVisible;
}

const std::string& DocumentSession::StorageOrigin() const
{
    return StoragePath;
}

StorageStanding DocumentSession::Standing() const
{
    return StorageDeclared;
}

std::uint64_t DocumentSession::SavedThrough() const
{
    return SavedRevision;
}

std::uint64_t DocumentSession::SavedAt() const
{
    return SavedStamp;
}

void DocumentSession::DeclareReadVersion(std::uint32_t ReadFrom)
{
    VersionRead = ReadFrom;
}

std::uint32_t DocumentSession::ReadVersion() const
{
    return VersionRead;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                   EVERY OPEN SESSION
//------------------------------------------------------------------------------------------------------------------------

Result<std::uint32_t> SessionIndex::Open()
{
    // 📝 A closed slot is reused before the span grows, so a session opened and closed repeatedly does not walk
    //    the ordinal upward until it meets the ceiling.
    for (std::size_t Ordinal = 0u; Ordinal < Sessions.size(); ++Ordinal)
    {
        if (Sessions[Ordinal] != nullptr) { continue; }

        Sessions[Ordinal] = std::make_unique<DocumentSession>();
        ++OpenTotal;

        if (PresentedSession == SessionCeiling)
        {
            PresentedSession = static_cast<std::uint32_t>(Ordinal);
        }

        return Result<std::uint32_t>::Result(static_cast<std::uint32_t>(Ordinal));
    }

    if (Sessions.size() >= static_cast<std::size_t>(SessionCeiling))
    {
        return Result<std::uint32_t>::Refuse(
            { RefusalReason::ExtentExhausted, "the declared session ceiling is reached — `48` §6" });
    }

    const std::uint32_t Issued = static_cast<std::uint32_t>(Sessions.size());

    Sessions.push_back(std::make_unique<DocumentSession>());
    ++OpenTotal;

    if (PresentedSession == SessionCeiling)
    {
        PresentedSession = Issued;
    }

    return Result<std::uint32_t>::Result(Issued);
}

Result<bool> SessionIndex::Close(std::uint32_t SessionOrdinal)
{
    if (SessionOrdinal >= Sessions.size() || Sessions[SessionOrdinal] == nullptr)
    {
        return Result<bool>::Refuse({ RefusalReason::ExtentExhausted, "no session is open at that ordinal" });
    }

    Sessions[SessionOrdinal].reset();
    --OpenTotal;

    if (PresentedSession != SessionOrdinal)
    {
        return Result<bool>::Result(true);
    }

    // 📝 The presentation moves to the first session still open rather than to none. Closing one of two open
    //    documents and being left presenting nothing reads as the application having closed both.
    PresentedSession = SessionCeiling;

    for (std::size_t Ordinal = 0u; Ordinal < Sessions.size(); ++Ordinal)
    {
        if (Sessions[Ordinal] == nullptr) { continue; }

        PresentedSession = static_cast<std::uint32_t>(Ordinal);
        break;
    }

    return Result<bool>::Result(true);
}

Result<DocumentSession*> SessionIndex::Resolve(std::uint32_t SessionOrdinal)
{
    if (SessionOrdinal >= Sessions.size() || Sessions[SessionOrdinal] == nullptr)
    {
        return Result<DocumentSession*>::Refuse({ RefusalReason::ExtentExhausted, "no session is open at that ordinal" });
    }

    return Result<DocumentSession*>::Result(Sessions[SessionOrdinal].get());
}

Result<const DocumentSession*> SessionIndex::Resolve(std::uint32_t SessionOrdinal) const
{
    if (SessionOrdinal >= Sessions.size() || Sessions[SessionOrdinal] == nullptr)
    {
        return Result<const DocumentSession*>::Refuse({ RefusalReason::ExtentExhausted, "no session is open at that ordinal" });
    }

    return Result<const DocumentSession*>::Result(Sessions[SessionOrdinal].get());
}

Result<bool> SessionIndex::DeclarePresented(std::uint32_t SessionOrdinal)
{
    if (SessionOrdinal >= Sessions.size() || Sessions[SessionOrdinal] == nullptr)
    {
        return Result<bool>::Refuse({ RefusalReason::ExtentExhausted, "no session is open at that ordinal" });
    }

    PresentedSession = SessionOrdinal;

    return Result<bool>::Result(true);
}

Result<DocumentSession*> SessionIndex::Presenting()
{
    return Resolve(PresentedSession);
}

Result<const DocumentSession*> SessionIndex::Presenting() const
{
    return Resolve(PresentedSession);
}

std::uint32_t SessionIndex::PresentedOrdinal() const
{
    return PresentedSession;
}

Result<std::uint32_t> SessionIndex::Located(const std::string& StoragePath) const
{
    for (std::size_t Remaining = Sessions.size(); Remaining > 0u; --Remaining)
    {
        const std::size_t Ordinal = Remaining - 1u;

        if (Sessions[Ordinal] == nullptr)                             { continue; }
        if (Sessions[Ordinal]->StorageOrigin() != StoragePath)        { continue; }

        return Result<std::uint32_t>::Result(static_cast<std::uint32_t>(Ordinal));
    }

    return Result<std::uint32_t>::Refuse({ RefusalReason::ExtentExhausted, "no open session holds that location" });
}

std::uint32_t SessionIndex::OpenCount() const
{
    return OpenTotal;
}

std::uint32_t SessionIndex::SpannedCount() const
{
    return static_cast<std::uint32_t>(Sessions.size());
}

void SessionIndex::Reclaim()
{
    Sessions.clear();
    PresentedSession = SessionCeiling;
    OpenTotal        = 0u;
}

}   // namespace Slate
