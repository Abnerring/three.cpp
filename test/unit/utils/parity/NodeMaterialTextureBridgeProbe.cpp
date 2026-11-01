#include "core/NativeObject.h"
#include "textures/Texture.h"
#include "math/Matrix3.h"
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::logic_error(message);
}
void identity(const JSValue& matrix) {
    const auto elements = matrix.get("elements");
    const double values[] = {1,0,0,0,1,0,0,0,1};
    for (unsigned i=0; i<9; ++i)
        require(elements.get(JSValue(i)).sameValue(values[i]), "Managed Matrix3 identity differs");
}
void managed() {
    auto texture = nativeTextureConstructor().construct();
    auto matrix = texture.get("matrix");
    const auto descriptor = matrix.getOwnPropertyDescriptor("elements");
    require(descriptor && descriptor->value.has_value(), "Managed Matrix3.elements must be own data");
    require(descriptor->writable.value_or(false) && descriptor->enumerable.value_or(false) && descriptor->configurable.value_or(false), "Managed Matrix3.elements descriptor differs");
    require(matrix.getPrototype().sameValue(nativeMatrix3Constructor().get("prototype")), "Managed Matrix3 must inherit its actual constructor prototype");
    require(matrix.get("elements").type()==JSValue::Type::Array, "Managed Matrix3.elements must be Array");
    identity(matrix);
    texture.get("offset").set("x", .25); texture.get("offset").set("y", .5);
    texture.get("repeat").set("x", 2); texture.get("repeat").set("y", 3);
    texture.get("updateMatrix").call(texture);
    require(texture.get("matrix").sameValue(matrix), "Managed updateMatrix replaced matrix identity");
    require(matrix.get("elements").get("6").sameValue(.25) && matrix.get("elements").get("7").sameValue(.5), "Managed updateMatrix lost source matrix mutation");
    const auto clone = texture.get("clone").call(texture);
    require(!clone.get("matrix").sameValue(matrix), "Managed clone aliased the source matrix");
    require(clone.get("matrix").get("elements").get("6").sameValue(.25), "Managed clone did not copy matrix elements");
    matrix.get("elements").set("6", .75);
    require(clone.get("matrix").get("elements").get("6").sameValue(.25), "Managed clone elements aliased the source");
}
void typedAlias() {
    auto texture = std::make_shared<Texture>();
    auto facade = nativeTexture(texture);
    auto matrix = facade.get("matrix");
    auto elements = matrix.get("elements");
    require(facade.get("matrix").sameValue(matrix), "Typed Texture matrix facade identity changed");
    elements.set("6", .75);
    require(texture->matrix.elements()[6]==.75, "Facade write did not reach typed Texture::matrix");
    texture->matrix.elements()[7]=.625;
    require(elements.get("7").sameValue(.625), "Typed Texture::matrix write did not reach facade elements");
    texture->offset.set(.25,.5); texture->repeat.set(2,3);
    facade.get("updateMatrix").call(facade);
    require(facade.get("matrix").sameValue(matrix) && matrix.get("elements").sameValue(elements), "Typed updateMatrix replaced active aliases");
    require(texture->matrix.elements()[0]==2 && texture->matrix.elements()[4]==3 && texture->matrix.elements()[6]==.25 && texture->matrix.elements()[7]==.5, "Typed updateMatrix failed to update payload");
    require(elements.get("6").sameValue(.25) && elements.get("7").sameValue(.5), "Typed updateMatrix failed to update existing element view");
    std::weak_ptr<Texture> owner=texture;
    facade={};texture.reset();
    matrix.collectGarbage();
    require(!owner.expired(), "Retained typed matrix alias lost its Texture owner");
    elements.set("6", .125);
    require(owner.lock()->matrix.elements()[6]==.125, "Retained typed alias is disconnected from its owner");
}
}
int main() {
    try { managed(); typedAlias(); std::cout << "Managed matrix and typed Texture aliases passed\n"; }
    catch(const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
