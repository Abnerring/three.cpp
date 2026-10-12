// Direct port of three.js src/extras/lib/earcut.js at
// 11738a9d55e502fb484cefffcea3a66dc44e9fe9 (Mapbox earcut 3.0.2).
// https://github.com/mapbox/earcut/tree/v3.0.2
/*
ISC License

Copyright (c) 2024, Mapbox

Permission to use, copy, modify, and/or distribute this software for any purpose
with or without fee is hereby granted, provided that the above copyright notice
and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.
*/
#include "extras/lib/earcut.h"
#include "core/JSValue.h"
#include "core/ArraySort.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>

namespace {
bool truthy(double value) { return value != 0 && !std::isnan(value); }
double minimum(double a, double b) {
    if (std::isnan(a) || std::isnan(b)) return std::numeric_limits<double>::quiet_NaN();
    if (a == b && a == 0) return std::signbit(a) || std::signbit(b) ? -0.0 : 0.0;
    return a < b ? a : b;
}
double maximum(double a, double b) {
    if (std::isnan(a) || std::isnan(b)) return std::numeric_limits<double>::quiet_NaN();
    if (a == b && a == 0) return std::signbit(a) && std::signbit(b) ? -0.0 : 0.0;
    return a > b ? a : b;
}
std::uint32_t uint32(double value) {
    if (!std::isfinite(value) || value == 0) return 0;
    auto modulo = std::fmod(std::trunc(value), 4294967296.0);
    if (modulo < 0) modulo += 4294967296.0;
    return static_cast<std::uint32_t>(modulo);
}
double coordinate(const std::vector<double>& data, double index) {
    return index >= 0 && index < static_cast<double>(data.size()) && std::floor(index) == index
        ? data[static_cast<std::size_t>(index)] : std::numeric_limits<double>::quiet_NaN();
}
double signedArea(const std::vector<double>& data, double start, double end, double dim) {
    double sum = 0;
    for (double i = start, j = end - dim; i < end; i += dim) {
        sum += (coordinate(data, j) - coordinate(data, i)) * (coordinate(data, i + 1) + coordinate(data, j + 1));
        j = i;
    }
    return sum;
}
struct Node {
    std::size_t i;
    double x, y;
    Node* prev = nullptr;
    Node* next = nullptr;
    std::int32_t z = 0;
    Node* prevZ = nullptr;
    Node* nextZ = nullptr;
    bool steiner = false;
};

class EarcutContext {
    std::vector<std::unique_ptr<Node>> nodes;
    static double area(const Node* p, const Node* q, const Node* r) {
        return (q->y - p->y) * (r->x - q->x) - (q->x - p->x) * (r->y - q->y);
    }
    static bool equals(const Node* p1, const Node* p2) { return p1->x == p2->x && p1->y == p2->y; }
    static int sign(double num) { return num > 0 ? 1 : num < 0 ? -1 : 0; }
    Node* createNode(std::size_t i, double x, double y) {
        nodes.push_back(std::make_unique<Node>());
        auto* p = nodes.back().get(); p->i = i; p->x = x; p->y = y; return p;
    }
    Node* insertNode(std::size_t i, double x, double y, Node* last) {
        auto* p = createNode(i, x, y);
        if (!last) { p->prev = p; p->next = p; }
        else { p->next = last->next; p->prev = last; last->next->prev = p; last->next = p; }
        return p;
    }
    static void removeNode(Node* p) {
        p->next->prev = p->prev; p->prev->next = p->next;
        if (p->prevZ) p->prevZ->nextZ = p->nextZ;
        if (p->nextZ) p->nextZ->prevZ = p->prevZ;
    }
    Node* linkedList(const std::vector<double>& data, double start, double end, double dim, bool clockwise) {
        Node* last = nullptr;
        auto insert = [&](double i) {
            const auto index = std::bit_cast<std::int32_t>(uint32(i / dim));
            last = insertNode(static_cast<std::size_t>(index), coordinate(data, i), coordinate(data, i + 1), last);
        };
        if (clockwise == (signedArea(data, start, end, dim) > 0)) {
            for (double i = start; i < end; i += dim) insert(i);
        } else {
            for (double i = end - dim; i >= start; i -= dim) insert(i);
        }
        if (last && equals(last, last->next)) { removeNode(last); last = last->next; }
        return last;
    }
    static Node* filterPoints(Node* start, Node* end = nullptr) {
        if (!start) return start;
        if (!end) end = start;
        auto* p = start;
        bool again;
        do {
            again = false;
            if (!p->steiner && (equals(p, p->next) || area(p->prev, p, p->next) == 0)) {
                removeNode(p); p = end = p->prev;
                if (p == p->next) break;
                again = true;
            } else p = p->next;
        } while (again || p != end);
        return end;
    }
    static bool pointInTriangle(double ax, double ay, double bx, double by, double cx, double cy, double px, double py) {
        return (cx - px) * (ay - py) >= (ax - px) * (cy - py) &&
               (ax - px) * (by - py) >= (bx - px) * (ay - py) &&
               (bx - px) * (cy - py) >= (cx - px) * (by - py);
    }
    static bool pointInTriangleExceptFirst(double ax, double ay, double bx, double by, double cx, double cy, double px, double py) {
        return !(ax == px && ay == py) && pointInTriangle(ax, ay, bx, by, cx, cy, px, py);
    }
    static bool isEar(Node* ear) {
        const auto *a = ear->prev, *b = ear, *c = ear->next;
        if (area(a, b, c) >= 0) return false;
        const double ax = a->x, bx = b->x, cx = c->x, ay = a->y, by = b->y, cy = c->y;
        const double x0 = minimum(minimum(ax, bx), cx), y0 = minimum(minimum(ay, by), cy);
        const double x1 = maximum(maximum(ax, bx), cx), y1 = maximum(maximum(ay, by), cy);
        auto* p = c->next;
        while (p != a) {
            if (p->x >= x0 && p->x <= x1 && p->y >= y0 && p->y <= y1 &&
                pointInTriangleExceptFirst(ax, ay, bx, by, cx, cy, p->x, p->y) && area(p->prev, p, p->next) >= 0) return false;
            p = p->next;
        }
        return true;
    }
    static std::int32_t zOrder(double x, double y, double minX, double minY, double invSize) {
        auto ix = uint32((x - minX) * invSize), iy = uint32((y - minY) * invSize);
        ix = (ix | (ix << 8)) & 0x00ff00ff;
        ix = (ix | (ix << 4)) & 0x0f0f0f0f;
        ix = (ix | (ix << 2)) & 0x33333333;
        ix = (ix | (ix << 1)) & 0x55555555;
        iy = (iy | (iy << 8)) & 0x00ff00ff;
        iy = (iy | (iy << 4)) & 0x0f0f0f0f;
        iy = (iy | (iy << 2)) & 0x33333333;
        iy = (iy | (iy << 1)) & 0x55555555;
        return std::bit_cast<std::int32_t>(ix | (iy << 1));
    }
    static bool isEarHashed(Node* ear, double minX, double minY, double invSize) {
        const auto *a = ear->prev, *b = ear, *c = ear->next;
        if (area(a, b, c) >= 0) return false;
        const double ax = a->x, bx = b->x, cx = c->x, ay = a->y, by = b->y, cy = c->y;
        const double x0 = minimum(minimum(ax, bx), cx), y0 = minimum(minimum(ay, by), cy);
        const double x1 = maximum(maximum(ax, bx), cx), y1 = maximum(maximum(ay, by), cy);
        const auto minZ = zOrder(x0, y0, minX, minY, invSize), maxZ = zOrder(x1, y1, minX, minY, invSize);
        auto *p = ear->prevZ, *n = ear->nextZ;
        while (p && p->z >= minZ && n && n->z <= maxZ) {
            if (p->x >= x0 && p->x <= x1 && p->y >= y0 && p->y <= y1 && p != a && p != c &&
                pointInTriangleExceptFirst(ax, ay, bx, by, cx, cy, p->x, p->y) && area(p->prev, p, p->next) >= 0) return false;
            p = p->prevZ;
            if (n->x >= x0 && n->x <= x1 && n->y >= y0 && n->y <= y1 && n != a && n != c &&
                pointInTriangleExceptFirst(ax, ay, bx, by, cx, cy, n->x, n->y) && area(n->prev, n, n->next) >= 0) return false;
            n = n->nextZ;
        }
        while (p && p->z >= minZ) {
            if (p->x >= x0 && p->x <= x1 && p->y >= y0 && p->y <= y1 && p != a && p != c &&
                pointInTriangleExceptFirst(ax, ay, bx, by, cx, cy, p->x, p->y) && area(p->prev, p, p->next) >= 0) return false;
            p = p->prevZ;
        }
        while (n && n->z <= maxZ) {
            if (n->x >= x0 && n->x <= x1 && n->y >= y0 && n->y <= y1 && n != a && n != c &&
                pointInTriangleExceptFirst(ax, ay, bx, by, cx, cy, n->x, n->y) && area(n->prev, n, n->next) >= 0) return false;
            n = n->nextZ;
        }
        return true;
    }
    static bool onSegment(const Node* p, const Node* q, const Node* r) {
        return q->x <= maximum(p->x, r->x) && q->x >= minimum(p->x, r->x) &&
               q->y <= maximum(p->y, r->y) && q->y >= minimum(p->y, r->y);
    }
    static bool intersects(const Node* p1, const Node* q1, const Node* p2, const Node* q2) {
        const auto o1 = sign(area(p1, q1, p2)), o2 = sign(area(p1, q1, q2));
        const auto o3 = sign(area(p2, q2, p1)), o4 = sign(area(p2, q2, q1));
        if (o1 != o2 && o3 != o4) return true;
        if (o1 == 0 && onSegment(p1, p2, q1)) return true;
        if (o2 == 0 && onSegment(p1, q2, q1)) return true;
        if (o3 == 0 && onSegment(p2, p1, q2)) return true;
        if (o4 == 0 && onSegment(p2, q1, q2)) return true;
        return false;
    }
    static bool locallyInside(const Node* a, const Node* b) {
        return area(a->prev, a, a->next) < 0 ?
            area(a, b, a->next) >= 0 && area(a, a->prev, b) >= 0 :
            area(a, b, a->prev) < 0 || area(a, a->next, b) < 0;
    }
    static Node* cureLocalIntersections(Node* start, std::vector<std::size_t>& triangles) {
        auto* p = start;
        do {
            auto *a = p->prev, *b = p->next->next;
            if (!equals(a, b) && intersects(a, p, p->next, b) && locallyInside(a, b) && locallyInside(b, a)) {
                triangles.insert(triangles.end(), {a->i, p->i, b->i});
                removeNode(p); removeNode(p->next); p = start = b;
            }
            p = p->next;
        } while (p != start);
        return filterPoints(p);
    }
    static bool intersectsPolygon(Node* a, const Node* b) {
        auto* p = a;
        do {
            if (p->i != a->i && p->next->i != a->i && p->i != b->i && p->next->i != b->i &&
                intersects(p, p->next, a, b)) return true;
            p = p->next;
        } while (p != a);
        return false;
    }
    static bool middleInside(Node* a, const Node* b) {
        auto* p = a;
        bool inside = false;
        const double px = (a->x + b->x) / 2, py = (a->y + b->y) / 2;
        do {
            if (((p->y > py) != (p->next->y > py)) && p->next->y != p->y &&
                (px < (p->next->x - p->x) * (py - p->y) / (p->next->y - p->y) + p->x)) inside = !inside;
            p = p->next;
        } while (p != a);
        return inside;
    }
    static bool isValidDiagonal(Node* a, Node* b) {
        return a->next->i != b->i && a->prev->i != b->i && !intersectsPolygon(a, b) &&
            ((locallyInside(a, b) && locallyInside(b, a) && middleInside(a, b) &&
              (truthy(area(a->prev, a, b->prev)) || truthy(area(a, b->prev, b)))) ||
             (equals(a, b) && area(a->prev, a, a->next) > 0 && area(b->prev, b, b->next) > 0));
    }
    Node* splitPolygon(Node* a, Node* b) {
        auto *a2 = createNode(a->i, a->x, a->y), *b2 = createNode(b->i, b->x, b->y), *an = a->next, *bp = b->prev;
        a->next = b; b->prev = a;
        a2->next = an; an->prev = a2;
        b2->next = a2; a2->prev = b2;
        bp->next = b2; b2->prev = bp;
        return b2;
    }
    void splitEarcut(Node* start, std::vector<std::size_t>& triangles, double dim, double minX, double minY, double invSize) {
        auto* a = start;
        do {
            auto* b = a->next->next;
            while (b != a->prev) {
                if (a->i != b->i && isValidDiagonal(a, b)) {
                    auto* c = splitPolygon(a, b);
                    a = filterPoints(a, a->next); c = filterPoints(c, c->next);
                    earcutLinked(a, triangles, dim, minX, minY, invSize, 0);
                    earcutLinked(c, triangles, dim, minX, minY, invSize, 0);
                    return;
                }
                b = b->next;
            }
            a = a->next;
        } while (a != start);
    }
    static Node* sortLinked(Node* list) {
        std::size_t numMerges, inSize = 1;
        do {
            auto* p = list;
            Node *e, *tail = nullptr;
            list = nullptr; numMerges = 0;
            while (p) {
                ++numMerges;
                auto* q = p;
                std::size_t pSize = 0;
                for (std::size_t i = 0; i < inSize; ++i) { ++pSize; q = q->nextZ; if (!q) break; }
                auto qSize = inSize;
                while (pSize > 0 || (qSize > 0 && q)) {
                    if (pSize != 0 && (qSize == 0 || !q || p->z <= q->z)) { e = p; p = p->nextZ; --pSize; }
                    else { e = q; q = q->nextZ; --qSize; }
                    if (tail) tail->nextZ = e; else list = e;
                    e->prevZ = tail; tail = e;
                }
                p = q;
            }
            tail->nextZ = nullptr; inSize *= 2;
        } while (numMerges > 1);
        return list;
    }
    static void indexCurve(Node* start, double minX, double minY, double invSize) {
        auto* p = start;
        do {
            if (p->z == 0) p->z = zOrder(p->x, p->y, minX, minY, invSize);
            p->prevZ = p->prev; p->nextZ = p->next; p = p->next;
        } while (p != start);
        p->prevZ->nextZ = nullptr; p->prevZ = nullptr;
        sortLinked(p);
    }
    void earcutLinked(Node* ear, std::vector<std::size_t>& triangles, double dim, double minX, double minY, double invSize, unsigned pass) {
        if (!ear) return;
        if (!pass && truthy(invSize)) indexCurve(ear, minX, minY, invSize);
        auto* stop = ear;
        while (ear->prev != ear->next) {
            auto *prev = ear->prev, *next = ear->next;
            if (truthy(invSize) ? isEarHashed(ear, minX, minY, invSize) : isEar(ear)) {
                triangles.insert(triangles.end(), {prev->i, ear->i, next->i});
                removeNode(ear); ear = next->next; stop = next->next; continue;
            }
            ear = next;
            if (ear == stop) {
                if (!pass) earcutLinked(filterPoints(ear), triangles, dim, minX, minY, invSize, 1);
                else if (pass == 1) {
                    ear = cureLocalIntersections(filterPoints(ear), triangles);
                    earcutLinked(ear, triangles, dim, minX, minY, invSize, 2);
                } else if (pass == 2) splitEarcut(ear, triangles, dim, minX, minY, invSize);
                break;
            }
        }
    }
    static Node* getLeftmost(Node* start) {
        auto *p = start, *leftmost = start;
        do {
            if (p->x < leftmost->x || (p->x == leftmost->x && p->y < leftmost->y)) leftmost = p;
            p = p->next;
        } while (p != start);
        return leftmost;
    }
    static bool sectorContainsSector(const Node* m, const Node* p) {
        return area(m->prev, m, p->prev) < 0 && area(p->next, m, m->next) < 0;
    }
    static Node* findHoleBridge(Node* hole, Node* outerNode) {
        auto* p = outerNode;
        const double hx = hole->x, hy = hole->y;
        double qx = -std::numeric_limits<double>::infinity();
        Node* m = nullptr;
        if (equals(hole, p)) return p;
        do {
            if (equals(hole, p->next)) return p->next;
            else if (hy <= p->y && hy >= p->next->y && p->next->y != p->y) {
                const double x = p->x + (hy - p->y) * (p->next->x - p->x) / (p->next->y - p->y);
                if (x <= hx && x > qx) {
                    qx = x; m = p->x < p->next->x ? p : p->next;
                    if (x == hx) return m;
                }
            }
            p = p->next;
        } while (p != outerNode);
        if (!m) return nullptr;
        const auto* stop = m;
        const double mx = m->x, my = m->y;
        double tanMin = std::numeric_limits<double>::infinity();
        p = m;
        do {
            if (hx >= p->x && p->x >= mx && hx != p->x &&
                pointInTriangle(hy < my ? hx : qx, hy, mx, my, hy < my ? qx : hx, hy, p->x, p->y)) {
                const double tan = std::abs(hy - p->y) / (hx - p->x);
                if (locallyInside(p, hole) && (tan < tanMin ||
                    (tan == tanMin && (p->x > m->x || (p->x == m->x && sectorContainsSector(m, p)))))) {
                    m = p; tanMin = tan;
                }
            }
            p = p->next;
        } while (p != stop);
        return m;
    }
    Node* eliminateHole(Node* hole, Node* outerNode) {
        auto* bridge = findHoleBridge(hole, outerNode);
        if (!bridge) return outerNode;
        auto* bridgeReverse = splitPolygon(bridge, hole);
        filterPoints(bridgeReverse, bridgeReverse->next);
        return filterPoints(bridge, bridge->next);
    }
    static double compareXYSlope(const Node* a, const Node* b) {
        double result = a->x - b->x;
        if (result == 0) {
            result = a->y - b->y;
            if (result == 0) {
                const double aSlope = (a->next->y - a->y) / (a->next->x - a->x);
                const double bSlope = (b->next->y - b->y) / (b->next->x - b->x);
                result = aSlope - bSlope;
            }
        }
        return result;
    }
    Node* eliminateHoles(const std::vector<double>& data, const std::vector<std::size_t>& holes, Node* outerNode, double dim) {
        std::vector<Node*> queue;
        for (std::size_t i = 0, len = holes.size(); i < len; ++i) {
            const double start = static_cast<double>(holes[i]) * dim;
            const double end = i < len - 1 ? static_cast<double>(holes[i + 1]) * dim : static_cast<double>(data.size());
            auto* list = linkedList(data, start, end, dim, false);
            if (!list) throw JSTypeError("earcut: hole list is undefined");
            if (list == list->next) list->steiner = true;
            queue.push_back(getLeftmost(list));
        }
        v8ArraySort(queue, compareXYSlope);
        for (auto* hole : queue) outerNode = eliminateHole(hole, outerNode);
        return outerNode;
    }
  public:
    std::vector<std::size_t> run(const std::vector<double>& data, const std::vector<std::size_t>& holes, double dim) {
        const bool hasHoles = !holes.empty();
        const double outerLen = hasHoles ? static_cast<double>(holes[0]) * dim : static_cast<double>(data.size());
        auto* outerNode = linkedList(data, 0, outerLen, dim, true);
        std::vector<std::size_t> triangles;
        if (!outerNode || outerNode->next == outerNode->prev) return triangles;
        double minX = std::numeric_limits<double>::quiet_NaN(), minY = minX, invSize = minX;
        if (hasHoles) outerNode = eliminateHoles(data, holes, outerNode, dim);
        if (static_cast<double>(data.size()) > 80 * dim) {
            minX = coordinate(data, 0); minY = coordinate(data, 1);
            double maxX = minX, maxY = minY;
            for (double i = dim; i < outerLen; i += dim) {
                const double x = coordinate(data, i), y = coordinate(data, i + 1);
                if (x < minX) minX = x;
                if (y < minY) minY = y;
                if (x > maxX) maxX = x;
                if (y > maxY) maxY = y;
            }
            invSize = maximum(maxX - minX, maxY - minY);
            invSize = invSize != 0 ? 32767 / invSize : 0;
        }
        earcutLinked(outerNode, triangles, dim, minX, minY, invSize, 0);
        return triangles;
    }
};
}

std::vector<std::size_t> earcut(const std::vector<double>& data, const std::vector<std::size_t>& holes, std::size_t dim) {
    // A zero stride makes the upstream numeric loops nonterminating. C++ callers
    // receive an explicit error instead of hanging this native thread.
    if (dim == 0) throw JSRangeError("earcut: dimension must be positive");
    return EarcutContext{}.run(data, holes, static_cast<double>(dim));
}
double deviation(const std::vector<double>& data, const std::vector<std::size_t>& holes,
                 std::size_t dim, const std::vector<std::size_t>& triangles) {
    if (dim == 0) throw JSRangeError("earcut: dimension must be positive");
    const bool hasHoles = !holes.empty();
    const double stride = static_cast<double>(dim);
    const double outerLen = hasHoles ? static_cast<double>(holes[0]) * stride : static_cast<double>(data.size());
    double polygonArea = std::abs(signedArea(data, 0, outerLen, stride));
    if (hasHoles) {
        for (std::size_t i = 0, len = holes.size(); i < len; ++i) {
            const double start = static_cast<double>(holes[i]) * stride;
            const double end = i < len - 1 ? static_cast<double>(holes[i + 1]) * stride : static_cast<double>(data.size());
            polygonArea -= std::abs(signedArea(data, start, end, stride));
        }
    }
    double trianglesArea = 0;
    const auto triangle = [&](std::size_t i) { return i < triangles.size() ? static_cast<double>(triangles[i]) * stride : std::numeric_limits<double>::quiet_NaN(); };
    for (std::size_t i = 0; i < triangles.size(); i += 3) {
        const double a = triangle(i), b = triangle(i + 1), c = triangle(i + 2);
        trianglesArea += std::abs((coordinate(data, a) - coordinate(data, c)) * (coordinate(data, b + 1) - coordinate(data, a + 1)) -
                                 (coordinate(data, a) - coordinate(data, b)) * (coordinate(data, c + 1) - coordinate(data, a + 1)));
    }
    return polygonArea == 0 && trianglesArea == 0 ? 0 : std::abs((trianglesArea - polygonArea) / polygonArea);
}
EarcutFlattenResult flatten(const std::vector<std::vector<std::vector<double>>>& data) {
    if (data.empty() || data[0].empty()) throw JSTypeError("earcut.flatten: first point is undefined");
    EarcutFlattenResult result{{}, {}, data[0][0].size()};
    std::size_t holeIndex = 0, prevLen = 0;
    for (const auto& ring : data) {
        for (const auto& point : ring) {
            for (std::size_t d = 0; d < result.dimensions; ++d)
                result.vertices.push_back(d < point.size() ? point[d] : std::numeric_limits<double>::quiet_NaN());
        }
        if (prevLen) { holeIndex += prevLen; result.holes.push_back(holeIndex); }
        prevLen = ring.size();
    }
    return result;
}
