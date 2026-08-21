//============================================================================================================================================
//                                                             CAMERARIG.CPP
//============================================================================================================================================

#include "Application/EditorHost/Api/CameraRig.h"

#include <algorithm>
#include <cmath>

namespace Slate
{

namespace
{

constexpr double HalfTurn = 3.14159265358979323846;

}   // namespace

void CameraRig::Ease(double& Lagged, double Target, double Seconds, const CameraSettings& Settings)
{
    if (!Settings.LagEnabled || Settings.LagSeconds <= 0.0 || Seconds <= 0.0)
    {
        Lagged = Target;
        return;
    }

    // 📐 The exponential approach: each tick closes a fraction of the remaining gap determined by the
    //    time constant. `1 - exp(-dt/τ)` is the exact integral of the first-order lag, so the rate is
    //    frame-rate independent.
    const double Fraction = 1.0 - std::exp(-Seconds / Settings.LagSeconds);
    Lagged += (Target - Lagged) * Fraction;
}

void CameraRig::Advance(double Seconds, const CameraCondition& Input, const CameraSettings& Settings)
{
    if (Seconds <= 0.0)
        return;

    // ① The look gesture: the pointer's travel turns the target. Positive X is rightward, so a drag to
    //    the right yaws clockwise; positive Y is downward, so a drag down pitches the view down and
    //    `InvertPitch` flips that.
    if (Input.LookHeld)
    {
        YawDegrees += static_cast<double>(Input.LookDeltaX) * Settings.LookSensitivity;
        // 📐 Positive Y is downward on the display, so a drag down pitches the view down (pitch
        //    decreases) by default, and `InvertPitch` flips that to the flight-sim convention.
        PitchDegrees += static_cast<double>(Input.LookDeltaY) * Settings.LookSensitivity
                      * (Settings.InvertPitch ? 1.0 : -1.0);
        PitchDegrees = std::clamp(PitchDegrees, -89.0, 89.0);
    }

    // ② The movement keys drive the target position along the camera's own frame: forward is the view
    //    direction (pitch included), right is the yaw's cross, up is world up.
    const double Yaw   = YawDegrees * HalfTurn / 180.0;
    const double Pitch = PitchDegrees * HalfTurn / 180.0;
    const double CosPitch = std::cos(Pitch);

    const double Forward[3] =
    {
        CosPitch * std::sin(Yaw),
        std::sin(Pitch),
        CosPitch * std::cos(Yaw)
    };
    const double Right[3] =
    {
        CosPitch * std::cos(Yaw),
        0.0,
        -CosPitch * std::sin(Yaw)
    };

    double Velocity[3] = { 0.0, 0.0, 0.0 };

    if (Input.ForwardHeld)
    {
        Velocity[0] += Forward[0];
        Velocity[1] += Forward[1];
        Velocity[2] += Forward[2];
    }
    if (Input.BackwardHeld)
    {
        Velocity[0] -= Forward[0];
        Velocity[1] -= Forward[1];
        Velocity[2] -= Forward[2];
    }
    if (Input.RightHeld)
    {
        Velocity[0] += Right[0];
        Velocity[1] += Right[1];
        Velocity[2] += Right[2];
    }
    if (Input.LeftHeld)
    {
        Velocity[0] -= Right[0];
        Velocity[1] -= Right[1];
        Velocity[2] -= Right[2];
    }
    if (Input.UpHeld)
        Velocity[1] += 1.0;
    if (Input.DownHeld)
        Velocity[1] -= 1.0;

    const double Speed = std::sqrt(Velocity[0] * Velocity[0]
                                 + Velocity[1] * Velocity[1]
                                 + Velocity[2] * Velocity[2]);

    if (Speed > 0.0)
    {
        const double Scale = Settings.FlySpeed * Seconds / Speed;

        Position[0] += Velocity[0] * Scale;
        Position[1] += Velocity[1] * Scale;
        Position[2] += Velocity[2] * Scale;
    }

    // ③ The lagged presentation eases toward the target. The yaw is wrapped so the lag never takes the
    //    long way around the compass.
    double WrappedYaw = YawDegrees;
    while (WrappedYaw - LaggedYawDegrees > 180.0)
        WrappedYaw -= 360.0;
    while (WrappedYaw - LaggedYawDegrees < -180.0)
        WrappedYaw += 360.0;

    Ease(LaggedYawDegrees, WrappedYaw, Seconds, Settings);
    Ease(LaggedPitchDegrees, PitchDegrees, Seconds, Settings);

    for (std::uint32_t Axis = 0u; Axis < 3u; ++Axis)
        Ease(LaggedPosition[Axis], Position[Axis], Seconds, Settings);
}

void CameraRig::Snap()
{
    LaggedYawDegrees   = YawDegrees;
    LaggedPitchDegrees = PitchDegrees;
    LaggedPosition[0]  = Position[0];
    LaggedPosition[1]  = Position[1];
    LaggedPosition[2]  = Position[2];
}

} // namespace Slate
