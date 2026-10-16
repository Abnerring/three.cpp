from pathlib import Path
import json, itertools

cases = [{"mode": "metadata"}]
numbers = [0, 1, -1, 2, 3.5, "2", "", True, False, None,
           {"$": "undefined"}, {"$": "NaN"}, {"$": "Infinity"}, {"$": "-Infinity"}, {"$": "-0"}, {"$": "symbol"}]
def add(op, **kwargs):
    case = {"name": f"{op}-{len(cases)}", "op": op, "entry": "export", "watch": True,
            "texture": {"image": {"width": 4, "height": 2}, "repeat": {"x": .25, "y": -.5}, "offset": {"x": 7, "y": 9}}, "aspect": 1}
    case.update(kwargs)
    cases.append(case)
    return case

for op, entry in itertools.product(["contain", "cover", "fill"], ["export", "class", "direct", "construct"]):
    for value in numbers:
        add(op, entry=entry, aspect=value)
    for width, height in itertools.product(numbers, [0, 2, -1, "3", None, {"$": "symbol"}]):
        case=add(op, entry=entry)
        case["texture"]["image"]={"width": width, "height": height}
    for image in [None, False, 0, "", {}, {"height": 2}, {"width": 3}]:
        case=add(op, entry=entry)
        case["texture"]["image"]=image
    for field, value in itertools.product(["repeat", "offset"], [None, {"$":"undefined"}, 1, False, "xy"]):
        case=add(op, entry=entry);case["texture"][field]=value
    for value in [None, {"$":"undefined"}, True, 1, "x"]:
        add(op, entry=entry, texture=value)
    for readonly in ["repeat.x","repeat.y","offset.x","offset.y"]:
        add(op, entry=entry, readonly=readonly)
    for field in ["width", "height"]:
        for value in [0, 2, "3", {"$":"symbol"}, {}]:
            case=add(op, entry=entry)
            case["texture"]["image"][field]={"$convert":{"label":field,"value":value,"gc":True}}
    for values in [[1,3],[4,1],[0,2],[{"$":"NaN"},1],[2,{"$":"symbol"}]]:
        for action in ["swapAt","throwAt","gcAt"]:
            for event in range(1,24):
                add(op,entry=entry,aspectValues=values,**{action:event})
for op in ["contain","cover","fill"]:
    for step, aspect in itertools.product(range(1,24),[1,3,0]):
        add(op,reenterAt=step,reenterAspect=aspect)
    for aspect in [1,3]:
        add(op,watch=False,aspect=aspect)

target=Path(__file__).with_name("texture-utils-transform-scenarios.jsonl")
target.write_text("".join(json.dumps(case,separators=(",",":"))+"\n" for case in cases),encoding="utf-8")
print(len(cases))
