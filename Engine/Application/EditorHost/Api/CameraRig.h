//============================================================================================================================================
//                                                             CAMERARIG.H
//============================================================================================================================================
// 🧩 The editor's fly camera — WASD + QE movement with Unreal-style right-button
//    look, exponential camera lag, and the settings the artist toggles.
//
//    The rig is a plain state machine, owned by the host and advanced once per
//    tick with the seam's `CameraCondition` and the artist's `CameraSettings`.
//    It knows no device and no vendor: the host maps keys to the condition, and
//    the harness proofs synthesise the same condition to render the same motion.
//
//    Convention: the second axis is up (the atmosphere's own), yaw is clockwise
//    from north, pitch is above the horizon — the same frame the sky dome's
//    azimuth/elevation use, so the rig's yaw and pitch ARE the viewport crop.

#pragma once

#include "SlateUI/Interface/InterfaceExchange/Api/RecordingSurface.h"

namespace Slate
{

/// 🧩 The artist's camera settings, edited through the scene directory's camera
///    details and properties: the fly speed, the lag's presence and its time
///    constant, and the look gesture's pitch direction.
/// tag   contract, nonallocating, nonthrowing
struct CameraSettings
{
    double FlySpeed    = 50.0;    // [m/s] - the movement integrator's rate
    double LagSeconds  = 0.18;    // [s]   - the lag's time constant
    bool   LagEnabled  = true;    // [-]   - the camera eases toward its target when set
    bool   InvertPitch = false;   // [-]   - the look gesture turns pitch the other way
    double LookSensitivity = 0.12; // [deg/px] - the look gesture's turn rate
};

/// 🧩 The fly camera itself: the target the artist drives and the lagged position
///    that lags behind it while `LagEnabled` stands.
/// tag   owning, nonallocating, nonthrowing
class CameraRig
{
public:

    CameraRig()  = default;
    ~CameraRig() = default;

    CameraRig(const CameraRig&)            = delete;
    CameraRig& operator=(const CameraRig&) = delete;

    /// 🧩 Advances the rig by one tick: the look gesture turns the target yaw and pitch, the held
    ///    movement keys drive the target position along the camera's own frame, and the lagged
    ///    presentation eases toward the target.
    /// in    Seconds   [s]   how long this tick lasted
    /// in    Input     [-]   the seam's camera condition
    /// in    Settings  [-]   the artist's camera settings
    /// note  📐 Movement is Unreal-fly: W/S along the view direction (pitch included), A/D strafing
    ///        across it, E/Q along world up. The yaw's lag is wrapped to the shortest turn so a camera
    ///        that spins past north eases through the few degrees, not the three hundred.
    /// cost  🚩
    /// tag   api, nonthrowing
    void Advance(double Seconds, const CameraCondition& Input, const CameraSettings& Settings);

    /// 🧩 Sets the lagged presentation onto the target, cancelling any residual lag.
    /// note  🔴 Called at bring-up and on a device rebuild, so the re-created viewport does not ease in
    ///        from the previous session's pose.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    void Snap();

    double YawDegrees   = 100.0;   // [deg] - the target yaw; the viewport crop reads the lagged one
    double PitchDegrees = 15.0;    // [deg] - the target pitch, clamped to ±89°
    double Position[3]  = { 0.0, 1.5, 0.0 };   // [m] - the target position; Y is up

    double LaggedYawDegrees   = 100.0;   // [deg] - what the viewport actually shows
    double LaggedPitchDegrees = 15.0;    // [deg]
    double LaggedPosition[3]  = { 0.0, 1.5, 0.0 };   // [m]

private:

    void Ease(double& Lagged, double Target, double Seconds, const CameraSettings& Settings);
};

} // namespace Slate
