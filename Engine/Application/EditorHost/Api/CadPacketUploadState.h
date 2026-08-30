//============================================================================================================================================
//                                                   CADPACKETUPLOADSTATE.H
//============================================================================================================================================
// Upload bookkeeping for the shared CAD packet. The packet is uploaded once, then recorded through
// each viewport leaf's projection; this state deliberately does not own viewport projection data.

#pragma once

#include <cstdint>

namespace Slate
{

struct CadPacketUploadState
{
    std::uint64_t UploadedFingerprint = 0ull;
    bool          HasUploadedPacket = false;

    void Invalidate()
    {
        UploadedFingerprint = 0ull;
        HasUploadedPacket = false;
    }

    bool NeedsUpload(std::uint64_t PacketFingerprint) const
    {
        return !HasUploadedPacket || UploadedFingerprint != PacketFingerprint;
    }

    void MarkUploaded(std::uint64_t PacketFingerprint)
    {
        UploadedFingerprint = PacketFingerprint;
        HasUploadedPacket = true;
    }
};

} // namespace Slate
