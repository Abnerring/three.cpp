#pragma once
#include "geometries/ShapeGeometry.h"
#include "extras/core/Curve.h"

class ExtrudeGeometry : public BufferGeometry {
  public:
    using Shapes = ShapeGeometry::Shapes;
    using ShapeList = ShapeGeometry::ShapeList;
    using ShapeMap = ShapeGeometry::ShapeMap;
    // Ordinary Array handle: UV callbacks can retain/mutate it, including undefined slots.
    using Vertices = JSValue;
    using UVs = std::vector<std::shared_ptr<Vector2>>;
    struct UVGenerator {
        virtual ~UVGenerator() = default;
        virtual UVs generateTopUV(ExtrudeGeometry&, Vertices, double a, double b, double c);
        virtual UVs generateSideWallUV(ExtrudeGeometry&, Vertices, double a, double b, double c, double d);
        virtual JsonValue toJSON() const { return JsonValue::Object{}; }
    };
    struct Options {
        // Scalar options retain JS defaulting and shared identity. Setters register native handles.
        JSValue values = JSValue::object();
        std::shared_ptr<Curve<Vector3>> extrudePath;
        std::shared_ptr<UVGenerator> uvGenerator;
        Options() = default;
        explicit Options(JSValue values) : values(std::move(values)) {}
        Options& set(const std::string& key, JSValue value) { values.set(key, value); return *this; }
        Options& setExtrudePath(std::shared_ptr<Curve<Vector3>> path) {
            extrudePath = std::move(path); values.set("extrudePath", extrudePath ? JSValue::object() : JSValue(nullptr)); return *this;
        }
        Options& setUVGenerator(std::shared_ptr<UVGenerator> generator) {
            uvGenerator = std::move(generator); values.set("UVGenerator", uvGenerator ? JSValue::object() : JSValue(nullptr)); return *this;
        }
    };
    using OptionsPtr = std::shared_ptr<Options>;
    struct Parameters { Shapes shapes; OptionsPtr options; };
    Parameters parameters;
    static std::shared_ptr<Shape> defaultShape();
    static std::shared_ptr<UVGenerator> worldUVGenerator();
    ExtrudeGeometry(Shapes shapes = defaultShape(), OptionsPtr options = std::make_shared<Options>());
    BufferGeometry& copy(const BufferGeometry& source) override;
    std::shared_ptr<BufferGeometry> clone() const override;
    JsonValue toJSON() const override;
    static std::shared_ptr<ExtrudeGeometry> fromJSON(const JsonValue& data, const ShapeMap& shapes);
};
