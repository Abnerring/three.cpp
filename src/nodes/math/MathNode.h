#pragma once
#include "core/JSModuleObject.h"
JSValue nativeMathNodeConstructor();
JSValue nativeMathNodeModule();
JSValue nativeMathNodeBinding(const std::string& name);

class MathNode {
    JSValue object_;
public:
    MathNode(const JSValue& method={},const JSValue& aNode={}):object_(nativeMathNodeConstructor().construct({method,aNode})){}
    MathNode(const JSValue& method,const JSValue& aNode,const JSValue& bNode):object_(nativeMathNodeConstructor().construct({method,aNode,bNode})){}
    MathNode(const JSValue& method,const JSValue& aNode,const JSValue& bNode,const JSValue& cNode):object_(nativeMathNodeConstructor().construct({method,aNode,bNode,cNode})){}
    explicit MathNode(const std::vector<JSValue>& args):object_(nativeMathNodeConstructor().construct(args)){}
    JSValue object()const{return object_;}
    operator JSValue()const{return object_;}
    static JSValue constructor(){return nativeMathNodeConstructor();}
    static JSValue prototype(){return constructor().get("prototype");}
    JSValue getInputType(const JSValue& builder)const{return object_.get("getInputType").call(object_,{builder});}
    JSValue generateNodeType(const JSValue& builder)const{return object_.get("generateNodeType").call(object_,{builder});}
    JSValue setup(const JSValue& builder)const{return object_.get("setup").call(object_,{builder});}
    JSValue generate(const JSValue& builder,const JSValue& output={})const{return object_.get("generate").call(object_,{builder,output});}
    JSValue serialize(const JSValue& data)const{return object_.get("serialize").call(object_,{data});}
    JSValue deserialize(const JSValue& data)const{return object_.get("deserialize").call(object_,{data});}
    inline static JSModuleMember ALL{nativeMathNodeConstructor,"ALL"};
    inline static JSModuleMember ANY{nativeMathNodeConstructor,"ANY"};
    inline static JSModuleMember RADIANS{nativeMathNodeConstructor,"RADIANS"};
    inline static JSModuleMember DEGREES{nativeMathNodeConstructor,"DEGREES"};
    inline static JSModuleMember EXP{nativeMathNodeConstructor,"EXP"};
    inline static JSModuleMember EXP2{nativeMathNodeConstructor,"EXP2"};
    inline static JSModuleMember LOG{nativeMathNodeConstructor,"LOG"};
    inline static JSModuleMember LOG2{nativeMathNodeConstructor,"LOG2"};
    inline static JSModuleMember SQRT{nativeMathNodeConstructor,"SQRT"};
    inline static JSModuleMember INVERSE_SQRT{nativeMathNodeConstructor,"INVERSE_SQRT"};
    inline static JSModuleMember FLOOR{nativeMathNodeConstructor,"FLOOR"};
    inline static JSModuleMember CEIL{nativeMathNodeConstructor,"CEIL"};
    inline static JSModuleMember NORMALIZE{nativeMathNodeConstructor,"NORMALIZE"};
    inline static JSModuleMember FRACT{nativeMathNodeConstructor,"FRACT"};
    inline static JSModuleMember SIN{nativeMathNodeConstructor,"SIN"};
    inline static JSModuleMember SINH{nativeMathNodeConstructor,"SINH"};
    inline static JSModuleMember COS{nativeMathNodeConstructor,"COS"};
    inline static JSModuleMember COSH{nativeMathNodeConstructor,"COSH"};
    inline static JSModuleMember TAN{nativeMathNodeConstructor,"TAN"};
    inline static JSModuleMember TANH{nativeMathNodeConstructor,"TANH"};
    inline static JSModuleMember ASIN{nativeMathNodeConstructor,"ASIN"};
    inline static JSModuleMember ASINH{nativeMathNodeConstructor,"ASINH"};
    inline static JSModuleMember ACOS{nativeMathNodeConstructor,"ACOS"};
    inline static JSModuleMember ACOSH{nativeMathNodeConstructor,"ACOSH"};
    inline static JSModuleMember ATAN{nativeMathNodeConstructor,"ATAN"};
    inline static JSModuleMember ATANH{nativeMathNodeConstructor,"ATANH"};
    inline static JSModuleMember ABS{nativeMathNodeConstructor,"ABS"};
    inline static JSModuleMember SIGN{nativeMathNodeConstructor,"SIGN"};
    inline static JSModuleMember LENGTH{nativeMathNodeConstructor,"LENGTH"};
    inline static JSModuleMember NEGATE{nativeMathNodeConstructor,"NEGATE"};
    inline static JSModuleMember ONE_MINUS{nativeMathNodeConstructor,"ONE_MINUS"};
    inline static JSModuleMember DFDX{nativeMathNodeConstructor,"DFDX"};
    inline static JSModuleMember DFDY{nativeMathNodeConstructor,"DFDY"};
    inline static JSModuleMember ROUND{nativeMathNodeConstructor,"ROUND"};
    inline static JSModuleMember RECIPROCAL{nativeMathNodeConstructor,"RECIPROCAL"};
    inline static JSModuleMember TRUNC{nativeMathNodeConstructor,"TRUNC"};
    inline static JSModuleMember FWIDTH{nativeMathNodeConstructor,"FWIDTH"};
    inline static JSModuleMember TRANSPOSE{nativeMathNodeConstructor,"TRANSPOSE"};
    inline static JSModuleMember DETERMINANT{nativeMathNodeConstructor,"DETERMINANT"};
    inline static JSModuleMember INVERSE{nativeMathNodeConstructor,"INVERSE"};
    inline static JSModuleMember EQUALS{nativeMathNodeConstructor,"EQUALS"};
    inline static JSModuleMember MIN{nativeMathNodeConstructor,"MIN"};
    inline static JSModuleMember MAX{nativeMathNodeConstructor,"MAX"};
    inline static JSModuleMember STEP{nativeMathNodeConstructor,"STEP"};
    inline static JSModuleMember REFLECT{nativeMathNodeConstructor,"REFLECT"};
    inline static JSModuleMember DISTANCE{nativeMathNodeConstructor,"DISTANCE"};
    inline static JSModuleMember DIFFERENCE{nativeMathNodeConstructor,"DIFFERENCE"};
    inline static JSModuleMember DOT{nativeMathNodeConstructor,"DOT"};
    inline static JSModuleMember CROSS{nativeMathNodeConstructor,"CROSS"};
    inline static JSModuleMember POW{nativeMathNodeConstructor,"POW"};
    inline static JSModuleMember TRANSFORM_DIRECTION{nativeMathNodeConstructor,"TRANSFORM_DIRECTION"};
    inline static JSModuleMember MIX{nativeMathNodeConstructor,"MIX"};
    inline static JSModuleMember CLAMP{nativeMathNodeConstructor,"CLAMP"};
    inline static JSModuleMember REFRACT{nativeMathNodeConstructor,"REFRACT"};
    inline static JSModuleMember SMOOTHSTEP{nativeMathNodeConstructor,"SMOOTHSTEP"};
    inline static JSModuleMember FACEFORWARD{nativeMathNodeConstructor,"FACEFORWARD"};
};
