#include "helpers/VertexNormalsHelper.h"
#include "helpers/VertexTangentsHelper.h"
#include "core/InterleavedBufferAttribute.h"
#include "objects/Points.h"
#include "objects/Sprite.h"
#include <iostream>

namespace {
using A = JsonValue::Array;
using O = JsonValue::Object;
JsonValue number(double x) {
    if (std::isnan(x)) return "NaN";
    if (std::isinf(x)) return x < 0 ? "-Infinity" : "Infinity";
    if (x == 0 && std::signbit(x)) return "-0";
    return x;
}
template<class T> JsonValue values(const T& v) {
    A out;
    for (const auto x : v) out.push_back(number(x));
    return out;
}
JsonValue attribute(const std::shared_ptr<VertexAttribute>& v) {
    if (!v) return nullptr;
    A out;
    const auto array = v->getArray();
    for (std::size_t i = 0; i < array.size(); ++i) out.push_back(number(array.number(double(i))));
    const auto version = v->getProperty("version");
    const auto data = v->getProperty("data");
    return O{{"array",out},{"count",number(v->getCount())},{"itemSize",number(v->getItemSize())},
             {"version",version.isUndefined() ? JsonValue("undefined") : number(version.toNumber())},
             {"dataVersion",data.isUndefined() ? JsonValue("undefined") : number(data.get("version").toNumber())}};
}
template<class H> JsonValue state(const H& h) {
    const auto material = std::dynamic_pointer_cast<LineBasicMaterial>(h.material);
    O out{{"type",h.type},{"isLine",h.isLine},{"isLineSegments",h.isLineSegments},
          {"matrixAutoUpdate",h.matrixAutoUpdate},{"matrixWorldNeedsUpdate",h.matrixWorldNeedsUpdate},
          {"matrix",values(h.matrix.elements())},{"matrixWorld",values(h.matrixWorld.elements())},
          {"size",h.size.isObject() ? JsonValue(O{{"type","object"}}) : h.size.trace()},
          {"position",h.geometry ? attribute(h.geometry->getAttribute("position")) : JsonValue(nullptr)},
          {"hasBoundingSphere",h.geometry && bool(h.geometry->boundingSphere)},
          {"sphere",h.geometry && h.geometry->boundingSphere ? JsonValue(A{
              number(h.geometry->boundingSphere->center.x),number(h.geometry->boundingSphere->center.y),
              number(h.geometry->boundingSphere->center.z),number(h.geometry->boundingSphere->radius)}) : JsonValue(nullptr)},
          {"material",material ? JsonValue(O{{"type",material->type},{"color",values(material->color.toArray())},
              {"toneMapped",material->toneMapped},{"depthTest",material->depthTest},
              {"depthWrite",material->depthWrite},{"vertexColors",material->vertexColors}}) : JsonValue(nullptr)}};
    if constexpr (std::is_same_v<H,VertexNormalsHelper>) out["flag"] = h.isVertexNormalsHelper;
    else out["flag"] = "undefined";
    return out;
}
struct ProbeMesh : Mesh {
    using Mesh::Mesh;
    int updates = 0;
    std::function<void()> after;
    void updateMatrixWorld(bool force = false) override {
        ++updates;
        Mesh::updateMatrixWorld(force);
        if (after) after();
    }
};
std::shared_ptr<BufferGeometry> sourceGeometry(int mode) {
    auto g = std::make_shared<BufferGeometry>();
    if (mode == 11) {
        g->setAttribute("position",std::make_shared<Float32BufferAttribute>(0,3));
        g->setAttribute("normal",std::make_shared<Float32BufferAttribute>(0,3));
        g->setAttribute("tangent",std::make_shared<Float32BufferAttribute>(0,4));
    } else if (mode == 12) {
        g->setAttribute("position",std::make_shared<Int8BufferAttribute>(std::vector<int>{0,0,0,127,64,-127,-64,32,-16},3,true));
        g->setAttribute("normal",std::make_shared<Int8BufferAttribute>(std::vector<int>{127,127,127,0,127,0,0,0,0},3,true));
        g->setAttribute("tangent",std::make_shared<Int8BufferAttribute>(std::vector<int>{127,127,127,-127,0,127,0,127,0,0,0,-127},4,true));
    } else if (mode == 13) {
        InterleavedBuffer data(Float32Array{0,0,0,1,1,1,1,1,1,-1, 1,2,3,0,1,0,0,1,0,1, -2,1,-1,0,0,0,0,0,0,-1},10);
        g->setAttribute("position",std::make_shared<InterleavedBufferAttribute>(data.object(),3,0));
        g->setAttribute("normal",std::make_shared<InterleavedBufferAttribute>(data.object(),3,3));
        g->setAttribute("tangent",std::make_shared<InterleavedBufferAttribute>(data.object(),4,6));
    } else if (mode == 14) {
        g->setAttribute("position",std::make_shared<Float16BufferAttribute>(std::vector<std::uint16_t>{0,0,0,0x3c00,0x4000,0x4200,0xc000,0x3c00,0xbc00},3));
        g->setAttribute("normal",std::make_shared<Float16BufferAttribute>(std::vector<std::uint16_t>{0x3c00,0x3c00,0x3c00,0,0x3c00,0,0,0,0},3));
        g->setAttribute("tangent",std::make_shared<Float16BufferAttribute>(std::vector<std::uint16_t>{0x3c00,0x3c00,0x3c00,0xbc00,0,0x3c00,0,0x3c00,0,0,0,0xbc00},4));
    } else {
        g->setAttribute("position",std::make_shared<Float32BufferAttribute>(std::vector<double>{0,0,0,1,2,3,-2,1,-1},3));
        g->setAttribute("normal",std::make_shared<Float32BufferAttribute>(std::vector<double>{1,1,1,0,1,0,0,0,0},3));
        g->setAttribute("tangent",std::make_shared<Float32BufferAttribute>(std::vector<double>{1,1,1,-1,0,1,0,1,0,0,0,-1},4));
    }
    return g;
}
JSValue scalarValue(const JSValue&,const std::vector<JSValue>&,const std::vector<JSValue>& captures) {
    auto counter = captures[0];
    const auto count = counter.get("count").toNumber() + 1;
    counter.set("count",count);
    if (counter.get("throw").toBoolean() && count == 2) throw JSTypeError("scalar conversion");
    return count;
}
template<class H> JsonValue scenario(int kind,int mode) {
    const auto target = kind == 0 ? "normal" : "tangent";
    auto g = sourceGeometry(mode);
    auto original = std::make_shared<ProbeMesh>(g);
    std::shared_ptr<Object3D> source = original;
    std::shared_ptr<Object3D> parent;
    std::shared_ptr<H> helper;
    JSValue size = 1;
    HelperColor color = kind == 0 ? 0xff0000 : 0x00ffff;
    std::shared_ptr<Color> colorObject;
    JSValue counter;
    O details;
    A events;
    std::string error;
    if (mode == 1 || mode == 2) {
        original->position.set(1,2,-3);original->rotation.set(.2,-.3,.4);
        original->scale.set(mode == 2 ? -2 : 2,.5,3);
        parent = std::make_shared<Object3D>();parent->position.set(3,-1,2);parent->rotation.set(.1,.2,-.4);
        parent->scale.set(1,2,3);parent->add(original);parent->updateMatrixWorld();
    }
    if (mode == 3) original->scale.set(0,2,0);
    if (mode == 4) {
        original->matrixAutoUpdate=false;original->matrixWorldAutoUpdate=false;
        original->matrixWorld.set(2,1,0,3,0,-2,1,4,1,0,.5,-1,.1,-.2,.3,1);
    }
    if (mode == 5) {original->matrixAutoUpdate=false;original->matrix.makeTranslation(3,4,5);}
    if (mode == 6) {parent=std::make_shared<Object3D>();parent->position.set(7,8,9);parent->add(original);original->position.set(1,2,3);}
    if (mode == 8) {g->getAttribute("normal")->setXYZ(0,0,0,0);g->getAttribute("tangent")->setXYZ(0,0,0,0);}
    if (mode == 9) {g->getAttribute(target)->setX(0,std::numeric_limits<double>::quiet_NaN());}
    if (mode == 10) size=std::numeric_limits<double>::infinity();
    if (mode == 15 || mode == 16 || mode == 17 || mode == 45 || mode == 46 || mode == 59) {
        const auto count=mode==15?5:mode==16?1:mode==17?1.5:mode==45?-1:mode==59?0:std::numeric_limits<double>::quiet_NaN();
        std::dynamic_pointer_cast<BufferAttribute>(g->getAttribute(target))->count=count;
    }
    if (mode == 26) size="2";
    if (mode == 27) size=false;
    if (mode == 29) size=std::numeric_limits<double>::quiet_NaN();
    if (mode == 38) source=std::make_shared<Line>(g);
    if (mode == 39) source=std::make_shared<Points>(g);
    if (mode == 40) {auto sprite=std::make_shared<Sprite>();sprite->geometry=g;source=sprite;}
    if (mode == 41) g->deleteAttribute(target);
    if (mode == 42) original->geometry.reset();
    if (mode == 43) source.reset();
    if (mode == 44) {size=JSValue{};color=HelperColor(JSValue{});}
    if (mode == 47 || mode == 48) {
        counter=JSValue::object();counter.set("count",0);counter.set("throw",mode==48);
        size=JSValue::object();size.set("valueOf",JSValue::function(scalarValue,{counter}));
    }
    if (mode == 49) color="#ff8000";
    if (mode == 50) {colorObject=std::make_shared<Color>(.2,.3,.4);color=colorObject;}
    if (mode == 51) color=HelperColor(JSValue(nullptr));
    if (mode == 52) color=0xffcc00;
    if (mode == 53) size=nullptr;
    if (mode == 54) g->setAttribute("position",std::make_shared<BufferAttribute>(Float64Array{.123456789,0,-0.0,1.123456789,2,3,-2,1,-1},3));
    if (mode == 57) {g->setIndex(std::vector<std::uint32_t>{2,1,0});g->setDrawRange(1,0);}
    if (mode == 58) std::dynamic_pointer_cast<BufferAttribute>(g->getAttribute("position"))->count=2.5;
    try {
        helper=std::make_shared<H>(source,size,color);
        details["sameSource"] = helper->object==source;
        if (mode == 7) {source->position.set(-2,3,4);g->getAttribute(target)->setXYZ(0,2,3,4);helper->size=-2;helper->update();}
        if (mode == 18) {g->deleteAttribute("position");helper->update();}
        if (mode == 19) {g->deleteAttribute(target);helper->update();}
        if (mode == 20) {original->geometry.reset();helper->update();}
        if (mode == 21) {helper->object.reset();helper->update();}
        if (mode == 22) {helper->geometry.reset();helper->update();}
        if (mode == 23) {helper->geometry->deleteAttribute("position");helper->update();}
        if (mode == 24) {helper->geometry->setAttribute("position",std::make_shared<Float32BufferAttribute>(6,3));helper->update();}
        if (mode == 25) {
            InterleavedBuffer data(Float32Array(std::vector<double>(24,7)),4);
            helper->geometry->setAttribute("position",std::make_shared<InterleavedBufferAttribute>(data.object(),3,1));helper->update();
        }
        if (mode == 28) {helper->size=JSValue{};helper->update();}
        if (mode == 30) {
            auto next=std::make_shared<ProbeMesh>(sourceGeometry(0));next->position.set(9,8,7);
            auto other=std::make_shared<H>(next,4);
            details["copyReturnedSelf"]=&helper->copy(*other,false)==helper.get();
            details["sameGeometry"]=helper->geometry==other->geometry;
            details["sameMaterial"]=helper->material==other->material;
            details["sameSourceAfterCopy"]=helper->object==source;
        }
        if (mode == 31 || mode == 32) helper->clone(mode==31);
        if (mode >= 33 && mode <= 36) {
            helper->addEventListener("dispose",[&](Event&){events.emplace_back("helper");});
            helper->geometry->addEventListener("dispose",[&](Event&){events.emplace_back("geometry");});
            helper->material->addEventListener("dispose",[&](Event&){events.emplace_back("material");});
            if (mode == 35) {
                auto replacement=std::make_shared<LineBasicMaterial>();
                replacement->addEventListener("dispose",[&](Event&){events.emplace_back("replacement");});
                helper->geometry->addEventListener("dispose",[&helper,replacement](Event&){helper->material=replacement;});
            }
            if (mode == 36) helper->geometry->addEventListener("dispose",[](Event&){throw std::runtime_error("dispose");});
            helper->dispose();if (mode == 34) helper->dispose();
        }
        if (mode == 37) {
            auto next=std::make_shared<ProbeMesh>(sourceGeometry(0));next->matrixWorld.makeTranslation(7,8,9);
            original->after=[helper,next]{helper->object=next;};helper->update();
            details["replacedSource"]=helper->object==next;
            original->after={};
        }
        if (mode == 50) {colorObject->setRGB(.7,.8,.9);helper->update();}
        if (mode == 55) {g->setAttribute("position",std::make_shared<Float32BufferAttribute>(0,3));g->deleteAttribute(target);helper->update();}
        if (mode == 56) {helper->geometry->computeBoundingSphere();helper->size=4;helper->update();}
    } catch (const JSTypeError&) {error="TypeError";}
      catch (const JSRangeError&) {error="RangeError";}
      catch (const std::exception&) {error="Error";}
    if (!counter.isUndefined()) details["conversions"]=counter.get("count").toNumber();
    details["events"]=events;
    return O{{"kind",kind},{"mode",mode},{"error",error},
             {"helper",helper ? state(*helper) : JsonValue(nullptr)},
             {"sourceWorld",source ? values(source->matrixWorld.elements()) : JsonValue(nullptr)},
             {"sourceUpdates",source==original ? original->updates : 0},{"details",details}};
}
}
int main() {
    try {
        A result;
        for(int kind=0;kind<2;++kind) for(int mode=0;mode<60;++mode)
            result.push_back(kind==0 ? scenario<VertexNormalsHelper>(kind,mode) : scenario<VertexTangentsHelper>(kind,mode));
        std::cout << JsonValue(result).dump() << '\n';
    } catch (const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
