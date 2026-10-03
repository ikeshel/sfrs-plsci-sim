#pragma once

// Nine 10 mm arrays fit on each 300 mm edge at 30 mm pitch while retaining
// an array at x=0. Centres at +/-150 mm would overhang the scintillator.
namespace SiPMLayout {
inline constexpr int arraysPerEdge=9;
inline constexpr int unitsPerArray=10;
inline constexpr int unitsPerEdge=arraysPerEdge*unitsPerArray;
inline constexpr int totalArrays=2*arraysPerEdge;
inline constexpr int totalUnits=2*unitsPerEdge;
inline constexpr double pitchMm=30.;
inline constexpr double centreMm(int array) {
  return (array-arraysPerEdge/2)*pitchMm;
}
}
