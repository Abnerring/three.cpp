#include "extras/ShapeUtils.h"
#include "core/JSValue.h"
#include "extras/lib/earcut.h"
#include "extras/Earcut.h"
#include "extras/ShapeUtilsSources.inc"
#include "nodes/lighting/LightingNodeRuntime.h"

namespace {
const Vector2& value(const Vector2& point) { return point; }
const Vector2& value(const std::shared_ptr<Vector2>& point) {
    if (!point) throw JSTypeError("Cannot read coordinates of missing contour point");
    return *point;
}
template<class Points> double signedArea(const Points& contour) {
    double area = 0;
    for (std::size_t q = 0, p = contour.empty() ? 0 : contour.size() - 1; q < contour.size(); p = q++) {
        area += value(contour[p]).x * value(contour[q]).y - value(contour[q]).x * value(contour[p]).y;
    }
    return area * 0.5;
}
template<class Points> void removeDuplicateEnd(Points& points) {
    if (points.size() > 2 && value(points.back()).equals(value(points.front()))) points.pop_back();
}
template<class Points> void addContour(std::vector<double>& vertices, const Points& contour) {
    for (const auto& point : contour) { vertices.push_back(value(point).x); vertices.push_back(value(point).y); }
}
template<class Points> ShapeUtils::Faces triangulate(Points& contour, std::vector<Points>& holes) {
    std::vector<double> vertices;
    std::vector<std::size_t> holeIndices;
    removeDuplicateEnd(contour);
    addContour(vertices, contour);
    auto holeIndex = contour.size();
    for (auto& hole : holes) removeDuplicateEnd(hole);
    for (const auto& hole : holes) {
        holeIndices.push_back(holeIndex); holeIndex += hole.size(); addContour(vertices, hole);
    }
    const auto triangles = earcut(vertices, holeIndices, 2);
    ShapeUtils::Faces faces;
    for (std::size_t i = 0; i < triangles.size(); i += 3) faces.push_back({triangles[i], triangles[i + 1], triangles[i + 2]});
    return faces;
}
}
double ShapeUtils::area(const Points& contour) { return signedArea(contour); }
double ShapeUtils::area(const std::vector<Vector2>& contour) { return signedArea(contour); }
bool ShapeUtils::isClockWise(const Points& points) { return area(points) < 0; }
bool ShapeUtils::isClockWise(const std::vector<Vector2>& points) { return area(points) < 0; }
ShapeUtils::Faces ShapeUtils::triangulateShape(Points& contour, std::vector<Points>& holes) { return triangulate(contour, holes); }
ShapeUtils::Faces ShapeUtils::triangulateShape(std::vector<Vector2>& contour, std::vector<std::vector<Vector2>>& holes) { return triangulate(contour, holes); }

namespace {
using namespace LightingNodeRuntime;
JSWeakValue& shapeLexical(){static thread_local JSWeakValue weak;return weak;}

JSValue managedArea(const JSValue&,const Args& args,const Args&){
    const auto contour=arg(args,0),n=contour.get("length");
    double a=0;
    for(double p=n.toNumber()-1,q=0;q<n.toNumber();p=q++){
        const auto x0=contour.get(p).get("x");
        const auto y1=contour.get(q).get("y");
        const auto x0Number=x0.toNumber();const auto y1Number=y1.toNumber();
        const auto first=x0Number*y1Number;
        const auto x1=contour.get(q).get("x");
        const auto y0=contour.get(p).get("y");
        const auto x1Number=x1.toNumber();const auto y0Number=y0.toNumber();
        a+=first-x1Number*y0Number;
    }
    return a*.5;
}
JSValue managedRemoveDuplicateEnd(const JSValue&,const Args& args,const Args&){
    const auto points=arg(args,0),length=points.get("length");
    if(length.toNumber()>2){
        const auto last=points.get(length.toNumber()-1);
        const auto equals=invoke(last,"equals");
        if(equals({points.get("0")}).toBoolean())invoke(points,"pop")();
    }
    return {};
}
JSValue managedAddContour(const JSValue&,const Args& args,const Args&){
    const auto vertices=arg(args,0),contour=arg(args,1);
    for(double i=0;i<contour.get("length").toNumber();++i){
        invoke(vertices,"push")({contour.get(i).get("x")});
        invoke(vertices,"push")({contour.get(i).get("y")});
    }
    return {};
}
JSValue managedTriangulate(const JSValue&,const Args& args,const Args& captures){
    const auto contour=arg(args,0),holes=arg(args,1),imports=captures[0];
    const auto vertices=JSValue::array(),holeIndices=JSValue::array(),faces=JSValue::array();
    imports.get("removeDupEndPts").call({}, {contour});
    imports.get("addContour").call({}, {vertices,contour});
    auto holeIndex=contour.get("length");
    invoke(holes,"forEach")({imports.get("removeDupEndPts")});
    for(double i=0;i<holes.get("length").toNumber();++i){
        invoke(holeIndices,"push")({holeIndex});
        const auto previous=holeIndex,length=holes.get(i).get("length");
        holeIndex=add(previous,length);
        imports.get("addContour").call({}, {vertices,holes.get(i)});
    }
    const auto triangles=invoke(imports.get("Earcut"),"triangulate")({vertices,holeIndices});
    for(double i=0;i<triangles.get("length").toNumber();i+=3)
        invoke(faces,"push")({invoke(triangles,"slice")({i,i+3})});
    return faces;
}
}
JSValue nativeShapeUtilsModule(){
    static thread_local JSWeakValue weak;
    if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;
    static thread_local bool active=false;JSModuleInitializationScope scope(active,"ShapeUtils");
    auto imports=JSValue::object(nullptr);
    auto earcutClass=JSValue::constructor(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};},{},"Earcut",0);
    earcutClass.setFunctionSourceText(ShapeUtilsSource::earcutClassSource);
    method(earcutClass,"triangulate",2,+[](const JSValue&,const Args& a,const Args&)->JSValue{
        return Earcut::triangulate(arg(a,0),arg(a,1),arg(a,2));
    },{},ShapeUtilsSource::earcutTriangulateSource);
    own(imports,"Earcut",earcutClass);
    auto remove=JSValue::function(managedRemoveDuplicateEnd,{},"removeDupEndPts",1);remove.setFunctionSourceText(ShapeUtilsSource::removeDupEndPtsSource);own(imports,"removeDupEndPts",remove);
    auto flatten=JSValue::function(managedAddContour,{},"addContour",2);flatten.setFunctionSourceText(ShapeUtilsSource::addContourSource);own(imports,"addContour",flatten);
    auto ctor=JSValue::constructor(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};},{},"ShapeUtils",0);
    ctor.setFunctionSourceText(ShapeUtilsSource::classSource);own(imports,"ShapeUtils",ctor);
    method(ctor,"area",1,managedArea,{},ShapeUtilsSource::areaSource);
    method(ctor,"isClockWise",1,+[](const JSValue&,const Args& a,const Args& c)->JSValue{
        return invoke(c[0],"area")({arg(a,0)}).toNumber()<0;
    },{ctor},ShapeUtilsSource::isClockWiseSource);
    method(ctor,"triangulateShape",2,managedTriangulate,{imports},ShapeUtilsSource::triangulateShapeSource);
    auto module=JSValue::object();own(module,"ShapeUtils",ctor);
    imports.retainForHeapLifetime();shapeLexical()=JSWeakValue(imports);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeShapeUtilsConstructor(){const auto keeper=nativeShapeUtilsModule();return shapeLexical().lock()->get("ShapeUtils");}
JSValue nativeShapeUtilsBinding(const std::string& name){const auto keeper=nativeShapeUtilsModule();return shapeLexical().lock()->get(name);}
