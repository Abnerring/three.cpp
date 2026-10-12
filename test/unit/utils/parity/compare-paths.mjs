import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {execFileSync} from 'node:child_process';

const upstream = path.resolve(process.argv[2]);
const executable = path.resolve(process.argv[3]);
const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json', import.meta.url)));
const commit = execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding:'utf8'}).trim();
if (commit !== lock.commit) throw new Error(`Expected ${lock.commit}, got ${commit}`);
const source = name => import(pathToFileURL(path.join(upstream, `src/${name}.js`)));
const {CurvePath} = await source('extras/core/CurvePath');
const {Path} = await source('extras/core/Path');
const {Shape} = await source('extras/core/Shape');
const {ShapePath} = await source('extras/core/ShapePath');
const {ShapeUtils} = await source('extras/ShapeUtils');
const {Vector2} = await source('math/Vector2');
const {Vector3} = await source('math/Vector3');
const curves = await source('extras/curves/Curves');
const {LineCurve, LineCurve3, QuadraticBezierCurve3, CubicBezierCurve3, CatmullRomCurve3, ArcCurve} = curves;
const points = values => values.map(p => p === null ? null : p.toArray());
const polygon = values => values.map(v => new Vector2(...v));

function curvePathSamples(p, V) {
    const result = {json:p.toJSON(), length:p.getLength(), lengths:[...p.getCurveLengths()],
        points:points(p.getPoints(3)), spaced:points(p.getSpacedPoints(6)),
        samples:[-.2,0,.15,.5,.95,1,1.1].map(t => p.getPoint(t)?.toArray() ?? null)};
    const target = new V(); result.targetIdentity = p.getPoint(.3, target) === target;
    result.target = target.toArray(); result.baseLength = p.getLength(); result.basePoints = points(p.getPoints(3));
    result.baseDefaultPointsCount = p.getPoints().length; result.baseDefaultSpacedCount = p.getSpacedPoints().length;
    p.autoClose = true;
    const closed = p.getPoints(3), spaced = p.getSpacedPoints(3);
    result.autoClose = {points:points(closed), spaced:points(spaced), pointAlias:closed[0] === closed.at(-1), spacedAlias:spaced[0] === spaced.at(-1)};
    const copy = p.clone(); result.clone = {json:copy.toJSON(), arrayDistinct:copy.curves !== p.curves, curveDistinct:copy.curves[0] !== p.curves[0]};
    result.restored = new CurvePath().fromJSON(p.toJSON()).toJSON();
    p.closePath(); result.closedJSON = p.toJSON(); result.closedLength = p.getLength();
    return result;
}
function mainPath() {
    const p = new Path(), originalCurrent = p.currentPoint;
    p.moveTo(-2,1).lineTo(3,1).quadraticCurveTo(4,5,7,2).bezierCurveTo(9,-2,11,6,13,1);
    p.splineThru(polygon([[15,4],[17,1]])).absellipse(20,2,3,2,.2,2.2,false,.3).arc(1,-2,1.5,.3,2.5,true);
    const result = curvePathSamples(p, Vector2);
    result.currentIdentity = p.currentPoint === originalCurrent; result.currentPoint = p.currentPoint.toArray();
    const restored = new Path(), current = restored.currentPoint; restored.fromJSON(p.toJSON());
    result.pathRestored = restored.toJSON(); result.restoredCurrentIdentity = current === restored.currentPoint;
    return result;
}
function spatialPath() {
    const p = new CurvePath();
    p.add(new LineCurve3(new Vector3(0,0,0), new Vector3(3,0,4)));
    p.add(new QuadraticBezierCurve3(new Vector3(3,0,4),new Vector3(5,7,-2),new Vector3(8,2,1)));
    p.add(new CubicBezierCurve3(new Vector3(8,2,1),new Vector3(9,-3,4),new Vector3(12,5,2),new Vector3(13,2,1)));
    p.add(new CatmullRomCurve3([[13,2,1],[15,5,-1],[17,-2,6],[20,1,3]].map(v => new Vector3(...v))));
    return curvePathSamples(p, Vector3);
}
function cacheAndEdges() {
    const result = {}, p = new CurvePath(), line = new LineCurve(new Vector2(), new Vector2(3,0)); p.add(line);
    const cache = p.getCurveLengths(); result.cacheIdentity = p.getCurveLengths() === cache;
    line.v2.x = 6; p.updateArcLengths(); result.pathOnlyUpdate = p.getLength(); result.cacheReplaced = p.cacheLengths !== cache; result.needsUpdate = p.needsUpdate;
    line.updateArcLengths(); p.updateArcLengths(); result.bothUpdate = p.getLength();
    const stale = p.getCurveLengths(), source = new CurvePath(); source.add(new LineCurve(new Vector2(),new Vector2(20,0)));
    p.copy(source); result.copyCacheIdentity = p.cacheLengths === stale; result.copyStaleLength = p.getLength();
    p.updateArcLengths(); result.copyUpdatedLength = p.getLength(); source.copy(source); result.selfCopyCount = source.curves.length;
    const empty = new Path(); result.emptyPoint = empty.getPoint(.5); result.emptyPoints = points(empty.getPoints()); result.emptySpaced = points(empty.getSpacedPoints(2));
    result.zeroDivisions = points(p.getSpacedPoints(0));
    const abs = new Path(), rel = new Path(); abs.absarc(); rel.arc(); result.defaultAbsArc = abs.toJSON(); result.defaultRelativeArc = rel.toJSON();
    const spline = new Path(); let thrown = false; try { spline.splineThru([]); } catch { thrown = true; }
    result.emptySpline = {throws:thrown,count:spline.curves.length,currentPoint:spline.currentPoint.toArray()};
    const arc = new CurvePath(); arc.add(new ArcCurve(2,3,4,.1,2.8,true)); result.arcRestored = new CurvePath().fromJSON(arc.toJSON()).toJSON();
    return result;
}
function shapeScenario() {
    const shape = new Shape(polygon([[0,0],[10,0],[10,10],[0,10],[0,0]])); shape.uuid = 'shape-fixed-uuid';
    const hole = new Path(polygon([[2,2],[2,4],[4,4],[4,2],[2,2]])); shape.holes.push(hole);
    const extracted = shape.extractPoints(4), clone = shape.clone(), copy = new Shape(), copyUUID = copy.uuid; copy.copy(shape);
    return {json:shape.toJSON(),shape:points(extracted.shape),holes:extracted.holes.map(points),
        cloneUUIDNew:clone.uuid !== shape.uuid,cloneHoleNew:clone.holes[0] !== hole,cloneCurveNew:clone.holes[0].curves[0] !== hole.curves[0],
        copyUUIDRetained:copy.uuid === copyUUID,restored:new Shape().fromJSON(shape.toJSON()).toJSON()};
}
function rectangle(p, lo, hi, reversed = false, offset = 0) {
    p.moveTo(lo + offset,lo);
    if (reversed) p.lineTo(lo + offset,hi).lineTo(hi + offset,hi).lineTo(hi + offset,lo);
    else p.lineTo(hi + offset,lo).lineTo(hi + offset,hi).lineTo(lo + offset,hi);
    p.lineTo(lo + offset,lo);
}
function addPolygon(p, values) { p.moveTo(...values[0]); for (const point of values.slice(1)) p.lineTo(...point); }
function shapePathScenario(scenario, evenOdd) {
    const p = new ShapePath();
    if (scenario === 0) { rectangle(p,0,20); rectangle(p,2,18); rectangle(p,4,16,true); rectangle(p,6,14,true); rectangle(p,8,12); }
    if (scenario === 1) { rectangle(p,3,7,true); rectangle(p,0,10); rectangle(p,0,10,false,20); p.moveTo(0,0).lineTo(1,1).lineTo(2,2); }
    if (scenario === 2) {
        addPolygon(p,[[0,0],[8,0],[8,2],[2,2],[2,8],[0,8],[0,0]]);
        addPolygon(p,[[.5,.5],[.5,1.5],[1.5,1.5],[1.5,.5],[.5,.5]]);
        addPolygon(p,[[0,3],[0,4],[1,4],[1,3],[0,3]]);
    }
    if (scenario === 3) { rectangle(p,0,10); rectangle(p,0,10,true); rectangle(p,2,8); }
    if (scenario === 4) { rectangle(p,0,10,true); rectangle(p,2,8,true); rectangle(p,3,7); }
    if (scenario >= 5) {
        const count = scenario === 5 ? 13 : 96;
        for (let i = 0; i < count; ++i) {
            const size = i % 7 === 2 ? NaN : i % 11 === 6 ? Infinity : (i * 17) % 31 + 1;
            rectangle(p,0,size,i % 3 === 1,i * 100);
        }
    }
    if (evenOdd) p.userData.style = {fillRule:'evenodd'};
    const origin = curves => p.subPaths.findIndex(sub => sub.curves === curves);
    return p.toShapes().map(shape => ({origin:origin(shape.curves),points:points(shape.getPoints()),
        holes:shape.holes.map(hole => ({origin:origin(hole.curves),points:points(hole.getPoints()),newPath:hole !== p.subPaths[origin(hole.curves)]})),
        currentPoint:shape.currentPoint.toArray(),autoClose:shape.autoClose}));
}
function triangulation(scenario) {
    let contour = [], holes = [];
    if (scenario === 0) contour = [[0,0],[10,0],[10,10],[0,10],[0,0]];
    if (scenario === 1) { contour = [[0,0],[10,0],[10,10],[0,10],[0,0]]; holes = [[[2,2],[2,4],[4,4],[4,2],[2,2]]]; }
    if (scenario === 2) { contour = [[0,0],[12,0],[12,12],[0,12]]; holes = [[[1,1],[1,3],[3,3],[3,1]],[[7,7],[7,10],[10,10],[10,7]]]; }
    if (scenario === 3) contour = [[0,0],[8,0],[8,2],[2,2],[2,8],[0,8],[0,0]];
    if (scenario === 4) contour = [[0,0],[0,5],[3,5],[3,5],[5,5],[5,0],[0,0]];
    if (scenario === 5) contour = [[0,0],[4,4],[0,4],[4,0]];
    if (scenario === 6) { contour = [[0,0],[10,0],[10,10],[0,10]]; holes = [[[5,5]]]; }
    if (scenario === 7) for (let i = 0; i < 120; ++i) { const a = i * Math.PI * 2 / 120, r = i % 2 ? 9 : 10; contour.push([Math.cos(a) * r, Math.sin(a) * r]); }
    contour = polygon(contour); holes = holes.map(polygon);
    const area = ShapeUtils.area(contour), clockwise = ShapeUtils.isClockWise(contour), triangles = ShapeUtils.triangulateShape(contour,holes);
    return {area,clockwise,contour:points(contour),holes:holes.map(points),triangles};
}
function normalized(value) {
    if (typeof value === 'number') return Number.isNaN(value) ? 'NaN' : !Number.isFinite(value) ? String(value) : Object.is(value,-0) ? '-0' : value;
    if (Array.isArray(value)) return value.map(normalized);
    if (value !== null && typeof value === 'object') return Object.fromEntries(Object.entries(value).map(([key,v]) => [key,normalized(v)]));
    return value;
}
function virtualSampling() {
    class DerivedPath extends CurvePath {
        calls = [];
        getPoint(t, target = new Vector2()) { this.calls.push(t); return target.set(99, t); }
    }
    const p = new DerivedPath(); p.add(new LineCurve(new Vector2(), new Vector2(1, 0)));
    const samples = points(p.getSpacedPoints(2)), calls = [...p.calls];
    p.autoClose = true;
    const closed = p.getSpacedPoints(1);
    class ThrowingPath extends CurvePath { getPoint() { throw new RangeError('derived failure'); } }
    let error = ''; try { new ThrowingPath().getSpacedPoints(1); } catch (value) { error = value.message; }
    return {samples, calls, closed: points(closed), identity: closed[0] === closed.at(-1), error};
}
function samplingDefaults() {
    class DefaultHole extends Path { getPoints(divisions = 7) { return [new Vector2(divisions, 0)]; } }
    class DefaultShape extends Shape { getPoints(divisions = 9) { return [new Vector2(divisions, 0)]; } }
    const shape = new DefaultShape(); shape.holes.push(new DefaultHole());
    const omitted = shape.extractPoints(), explicitValue = shape.extractPoints(3);
    return [shape.getPointsHoles()[0][0].x, omitted.shape[0].x, omitted.holes[0][0].x,
        explicitValue.shape[0].x, explicitValue.holes[0][0].x];
}
function replacedSubPaths() {
    const owner = new ShapePath();
    class ReplacingPath extends Path { getPoints() { owner.subPaths = []; return super.getPoints(); } }
    const first = new ReplacingPath(); first.moveTo(0, 0).lineTo(2, 0).lineTo(0, 2);
    const second = new Path(); second.moveTo(10, 0).lineTo(12, 0).lineTo(10, 2);
    owner.subPaths.push(first, second);
    const shapes = owner.toShapes().map(shape => ({points: points(shape.getPoints()),
        first: shape.curves === first.curves, second: shape.curves === second.curves}));
    return {shapes, subPathsCount: owner.subPaths.length};
}
function curveMutation(copying) {
    const source = new CurvePath(), destination = new CurvePath();
    class MutatingCurve extends LineCurve {
        replace() {
            source.curves = [new LineCurve(), new LineCurve(new Vector2(), new Vector2(20, 0))];
            if (copying) destination.curves = [];
        }
        toJSON() { this.replace(); return super.toJSON(); }
        clone() { this.replace(); return super.clone(); }
    }
    source.add(new MutatingCurve()); source.add(new LineCurve(new Vector2(), new Vector2(10, 0)));
    if (copying) { destination.copy(source); return [destination.curves.length, destination.curves.at(-1).getPoint(1).x]; }
    const entries = source.toJSON().curves;
    return [entries.length, entries[1].v2[0]];
}
function holeMutation(copying) {
    const source = new Shape(), destination = new Shape();
    class MutatingHole extends Path {
        replace() {
            source.holes = [new Path(), new Path().lineTo(20, 0)];
            if (copying) destination.holes = [];
        }
        toJSON() { this.replace(); return super.toJSON(); }
        clone() { this.replace(); return super.clone(); }
    }
    source.holes.push(new MutatingHole(), new Path().lineTo(10, 0));
    if (copying) { destination.copy(source); return [destination.holes.length, destination.holes.at(-1).currentPoint.x]; }
    const entries = source.toJSON().holes;
    return [entries.length, entries[1].currentPoint[0]];
}
function missingSubPaths() {
    return [false,true].map(duringCallback => {
        const owner = new ShapePath();
        class ReplacingPath extends Path { getPoints() { owner.subPaths[1] = null; return super.getPoints(); } }
        if (duringCallback) owner.subPaths.push(new ReplacingPath().moveTo(0,0).lineTo(2,0).lineTo(0,2), new Path());
        else owner.subPaths.push(null);
        try { owner.toShapes(); return ''; } catch (error) { return error.name; }
    });
}
function missingContourPoints() {
    return [0,1,2,3].map(mode => {
        const contour = [null,new Vector2(1,0),new Vector2(0,1)];
        try {
            if (mode === 0) ShapeUtils.area(contour);
            else if (mode === 1) ShapeUtils.isClockWise(contour);
            else if (mode === 2) ShapeUtils.triangulateShape(contour,[]);
            else {
                class MissingPointPath extends Path { getPoints() { return contour; } }
                const owner = new ShapePath(); owner.subPaths.push(new MissingPointPath()); owner.toShapes();
            }
            return '';
        } catch (error) { return error.name; }
    });
}
const expected = normalized({path:mainPath(),spatial:spatialPath(),cacheEdges:cacheAndEdges(),shape:shapeScenario(),
    classifications:Array.from({length:7},(_,i) => [shapePathScenario(i,false),shapePathScenario(i,true)]).flat(),
    triangulations:Array.from({length:9},(_,i) => triangulation(i)), virtualSampling: virtualSampling(),
    samplingDefaults: samplingDefaults(), replacedSubPaths: replacedSubPaths(), missingSubPaths: missingSubPaths(),
    missingContourPoints: missingContourPoints(),
    mutatingContainers: [curveMutation(false), curveMutation(true), holeMutation(false), holeMutation(true)]});
const actual = JSON.parse(execFileSync(executable, [], {encoding:'utf8',maxBuffer:20 * 1024 * 1024}));
let checks = 0;
function compare(a, b, location = '$') {
    ++checks;
    if (typeof a === 'number' && typeof b === 'number') {
        const tolerance = 2e-10 * Math.max(1,Math.abs(a),Math.abs(b));
        if (Math.abs(a-b) > tolerance) throw new Error(`${location}: ${a} != ${b}`);
    } else if (Array.isArray(a) && Array.isArray(b)) {
        if (a.length !== b.length) throw new Error(`${location}: length ${a.length} != ${b.length}`);
        a.forEach((value,i) => compare(value,b[i],`${location}[${i}]`));
    } else if (a !== null && b !== null && typeof a === 'object' && typeof b === 'object') {
        const keys = Object.keys(a);
        if (keys.length !== Object.keys(b).length || keys.some(key => !(key in b))) throw new Error(`${location}: keys mismatch`);
        for (const key of keys) compare(a[key],b[key],`${location}.${key}`);
    } else if (a !== b) throw new Error(`${location}: ${JSON.stringify(a)} != ${JSON.stringify(b)}`);
}
compare(actual,expected);
console.log(`Path parity passed: ${checks} values/identities; fixed upstream ${commit}`);
