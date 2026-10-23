#pragma once
#include <string>

// Port of the outline rules in src/nodes/display/ToonOutlinePassNode.js at the pinned revision.
// The outline is drawn by pushing the back faces of an object outwards along its normal, and
// three details of that push are worth writing down.
class ToonOutlinePass {
  public:
    static constexpr const char* kTypeName = "ToonOutlinePassNode";
    static constexpr const char* kPassName = "Outline Pass";
    static constexpr const char* kMaterialName = "Toon_Outline";

    // The defaults a call without arguments gets.
    static constexpr double kDefaultThickness = 0.003;
    static constexpr double kDefaultAlpha = 1.0;

    // The side the outline is drawn on. It is the back, which is why the normal is turned over
    // before it is used: the faces being drawn are the ones facing away.
    static constexpr const char* kSide = "BackSide";

    // The direction the outline is pushed along, given the two clip space positions the shader
    // works out. The order is the second taken from the first rather than the other way round,
    // which its comment explains as being because the back side's normal points the other way;
    // swapping them would push the outline inwards.
    struct Direction {
        double x = 0, y = 0, z = 0;
    };

    static Direction direction(double fromX, double fromY, double fromZ, double toX, double toY,
                               double toZ);

    // The push distance for a given thickness. It is scaled by the clip space w, which is what
    // keeps the outline the same visual width whatever the object's distance or scale; without
    // it a far object's outline would be thinner than a near one's.
    static double push(double thickness, double clipW);

    // The factor that would let each vertex have its own thickness. It is one in the source and
    // carries a note that per vertex thickness is not supported yet, so the factor does nothing
    // today and exists for a change that has not been made.
    static double thicknessRatio();

    // Whether a material receives an outline. Only the two toon kinds do, and only when the
    // material is not drawn as a wireframe.
    static bool outlines(bool isToonMaterial, bool isToonNodeMaterial, bool wireframeIsTrue);
};
