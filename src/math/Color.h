/**
 * @file   Color.h
 * @brief  颜色类
 * @author Abnerring
 * @date   2025-2026
 * @note   参考 three.js Color 实现，支持 RGB、HSL、Hex 格式
 */

#ifndef COLOR_H
#define COLOR_H

#include "core/BasicType.h"
#include <string>
#include <vector>
#include "math/ColorManagement.h"
#include "../constants.h"

class Vector3;
class Vector4;

/**
 * @class Color
 * @brief 颜色类，支持 RGB、HSL 和十六进制格式
 * @details RGB 分量范围 [0, 1]，自动处理格式转换
 * @see Vector3 Vector4
 *
 * @example
 * @code
 * Color c1(1.0, 0.0, 0.0);      // 红色
 * Color c2("#ff0000");          // 十六进制
 * Color c3; c3.setHSL(0.5, 1, 0.5);  // 绿色
 * @endcode
 */
class Color
{
public:
    static constexpr bool isColor = true;
    /**
     * @brief 构造 RGB 颜色
     * @param r 红分量 [0, 1]，默认 1
     * @param g 绿分量 [0, 1]，默认 1
     * @param b 蓝分量 [0, 1]，默认 1
     */
    Color();
    Color(HIGH_PRECISION r, HIGH_PRECISION g, HIGH_PRECISION b);
    explicit Color(HIGH_PRECISION hex);

    /**
     * @brief 从十六进制构造
     * @param hex 十六进制颜色值，如 0xff0000
     */
    Color(int hex);

    /**
     * @brief 从颜色字符串构造
     * @param colorString 颜色名（如 "red"）或十六进制（如 "#ff0000"）
     */
    Color(const std::string &colorString);
    Color(const char* colorString) : Color(std::string(colorString)) {}

    /** @brief 析构函数 */
    ~Color();

    /** @brief 获取红分量 @return r 值 [0, 1] */
    HIGH_PRECISION r;

    /** @brief 获取绿分量 @return g 值 [0, 1] */
    HIGH_PRECISION g;

    /** @brief 获取蓝分量 @return b 值 [0, 1] */
    HIGH_PRECISION b;

    /**
     * @brief 设置 RGB 分量
     * @param r 红分量
     * @param g 绿分量
     * @param b 蓝分量
     * @return Color& 自身引用
     */
    Color &set(HIGH_PRECISION r, HIGH_PRECISION g, HIGH_PRECISION b);

    /**
     * @brief 设置所有 RGB 分量为相同值
     * @param scalar 标量值
     * @return Color& 自身引用
     */
    Color &setScalar(HIGH_PRECISION scalar);

    /**
     * @brief 从十六进制设置
     * @param hex 十六进制颜色值
     * @return Color& 自身引用
     */
    Color &setHex(HIGH_PRECISION hex, const std::string &colorSpace = std::string(SRGBColorSpace));

    /**
     * @brief 从 HSL 设置
     * @param h 色相 [0, 1]
     * @param s 饱和度 [0, 1]
     * @param l 亮度 [0, 1]
     * @return Color& 自身引用
     */
    Color &setHSL(HIGH_PRECISION h, HIGH_PRECISION s, HIGH_PRECISION l, const std::string &colorSpace = ColorManagement::workingColorSpace);

    /**
     * @brief 从颜色名设置
     * @param colorName 颜色名，如 "red", "blue"
     * @return Color& 自身引用
     */
    Color &setColorName(const std::string &colorName, const std::string &colorSpace = std::string(SRGBColorSpace));

    /** @brief 克隆颜色 @return 颜色副本 */
    Color clone() const;

    /**
     * @brief 复制颜色
     * @param color 源颜色
     * @return Color& 自身引用
     */
    Color &copy(const Color &color);

    /**
     * @brief 颜色加法
     * @param color 加的颜色
     * @return Color& 自身引用
     */
    Color &add(const Color &color);

    /**
     * @brief 两颜色相加
     * @param color1 颜色 1
     * @param color2 颜色 2
     * @return Color& 自身引用
     */
    Color &addColors(const Color &color1, const Color &color2);

    /**
     * @brief 所有分量加上标量
     * @param s 标量值
     * @return Color& 自身引用
     */
    Color &addScalar(HIGH_PRECISION s);

    /**
     * @brief 颜色减法
     * @param color 减的颜色
     * @return Color& 自身引用
     */
    Color &sub(const Color &color);

    /**
     * @brief 所有分量减去标量
     * @param s 标量值
     * @return Color& 自身引用
     */
    Color &subScalar(HIGH_PRECISION s);

    /**
     * @brief 颜色乘法（逐分量）
     * @param color 乘的颜色
     * @return Color& 自身引用
     */
    Color &multiply(const Color &color);

    /**
     * @brief 所有分量乘以标量
     * @param scalar 标量值
     * @return Color& 自身引用
     */
    Color &multiplyScalar(HIGH_PRECISION scalar);

    /**
     * @brief 所有分量除以标量
     * @param scalar 标量值
     * @return Color& 自身引用
     * @warning scalar 不能为 0
     */
    Color &divideScalar(HIGH_PRECISION scalar);

    /**
     * @brief RGB 线性插值
     * @param color 目标颜色
     * @param alpha 插值因子 [0, 1]
     * @return Color& 自身引用
     */
    Color &lerp(const Color &color, HIGH_PRECISION alpha);

    /**
     * @brief HSL 空间线性插值
     * @param color 目标颜色
     * @param alpha 插值因子 [0, 1]
     * @return Color& 自身引用
     */
    Color &lerpHSL(const Color &color, HIGH_PRECISION alpha);

    /**
     * @brief 判断是否相等
     * @param color 比较颜色
     * @param epsilon 容差
     * @return bool 是否相等
     */
    bool equals(const Color &color, HIGH_PRECISION epsilon = 0) const;

    /**
     * @brief 获取十六进制整数
     * @return int 十六进制颜色值
     */
    int getHex(const std::string &colorSpace = std::string(SRGBColorSpace)) const;

    /**
     * @brief 获取十六进制字符串
     * @return std::string 如 "#ff0000"
     */
    std::string getHexString(const std::string &colorSpace = std::string(SRGBColorSpace)) const;

    /**
     * @brief 获取 CSS 样式字符串
     * @return std::string 如 "rgb(255,0,0)"
     */
    std::string getStyle(const std::string &colorSpace = std::string(SRGBColorSpace)) const;

    /**
     * @brief 获取 HSL 字符串
     * @return std::string 如 "hsl(0,1,0.5)"
     */
    std::string getHSL() const;

    /**
     * @brief 获取 HSL 值
     * @param result 输出 HSL 向量 {h, s, l}
     */
    Vector3 &getHSL(Vector3 &result, const std::string &colorSpace = ColorManagement::workingColorSpace) const;

    /**
     * @brief 从 Vector3 设置
     * @param v 向量 (r, g, b)
     * @return Color& 自身引用
     */
    Color &fromVector3(const Vector3 &v);

    /**
     * @brief 从 Vector4 设置
     * @param v 向量 (r, g, b, a)，a 被忽略
     * @return Color& 自身引用
     */
    Color &fromVector4(const Vector4 &v);

    /**
     * @brief 转换到 Vector3
     * @param result 输出向量
     */
    void toVector3(Vector3 &result) const;

    /**
     * @brief 转换到 Vector4
     * @param result 输出向量，w 设为 1
     */
    void toVector4(Vector4 &result) const;

    /**
     * @brief HSL 偏移
     * @param h 色相偏移
     * @param s 饱和度偏移
     * @param l 亮度偏移
     * @return Color& 自身引用
     */
    Color &offsetHSL(HIGH_PRECISION h, HIGH_PRECISION s, HIGH_PRECISION l);

    // The gamma-factor conversions of older three.js releases are intentionally absent: the
    // pinned r187 Color exposes only copySRGBToLinear / copyLinearToSRGB /
    // convertSRGBToLinear / convertLinearToSRGB below.

public:
    /** @brief 加法运算符 @return 新颜色 */
    Color operator+(const Color &c) const;

    /** @brief 减法运算符 @return 新颜色 */
    Color operator-(const Color &c) const;

    /** @brief 乘法运算符 @return 新颜色 */
    Color operator*(const Color &c) const;

    /** @brief 标量乘法运算符 @return 新颜色 */
    Color operator*(HIGH_PRECISION s) const;

    /** @brief 标量除法运算符 @return 新颜色 */
    Color operator/(HIGH_PRECISION s) const;

    /** @brief 赋值运算符 */
    Color &operator=(const Color &c);

    /** @brief 相等判断 @return 是否相等 */
    bool operator==(const Color &c) const;

    /** @brief 下标访问 @param index 0=r, 1=g, 2=b @return 分量值 */
    HIGH_PRECISION operator[](size_t index) const;

    Color &set(int hex);
    Color &set(const std::string &style);
    Color &set(const Color &color);
    Color &setRGB(HIGH_PRECISION r, HIGH_PRECISION g, HIGH_PRECISION b, const std::string &colorSpace = ColorManagement::workingColorSpace);
    Color &setStyle(const std::string &style, const std::string &colorSpace = std::string(SRGBColorSpace));
    Color &copySRGBToLinear(const Color &color);
    Color &copyLinearToSRGB(const Color &color);
    Color &convertSRGBToLinear();
    Color &convertLinearToSRGB();
    Color &getRGB(Color &result, const std::string &colorSpace = ColorManagement::workingColorSpace) const;
    Color &lerpColors(const Color &a, const Color &b, HIGH_PRECISION alpha);
    Color &setFromVector3(const Vector3 &v);
    Color &fromArray(const std::vector<HIGH_PRECISION> &array, size_t offset = 0);
    std::vector<HIGH_PRECISION> &toArray(std::vector<HIGH_PRECISION> &array, size_t offset = 0) const;
    std::vector<HIGH_PRECISION> toArray() const;
    int toJSON() const;

    Color &applyMatrix3(const class Matrix3 &m);
    Color &fromBufferAttribute(const class BufferAttribute &attribute, double index);

private:

    /**
     * @brief 获取颜色名对应的十六进制值
     * @param name 颜色名
     * @return int 十六进制值，-1 表示未找到
     */
    static int getHexColorName(const std::string &name);
};

#endif
