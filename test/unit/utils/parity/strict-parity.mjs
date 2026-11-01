function display(value) {
  if (typeof value === 'number') return Number.isNaN(value) ? '#NaN' : value === Infinity ? '#Infinity' : value === -Infinity ? '#-Infinity' : Object.is(value, -0) ? '#-0' : value;
  if (value === undefined) return '#undefined';
  return typeof value === 'symbol' ? String(value) : value;
}

export function strictDifferences(expected, actual, path = '$', differences = []) {
  const add = (kind, a = expected, b = actual) => differences.push({path, kind, expectedType: typeof a, actualType: typeof b, expected: display(a), actual: display(b)});
  if (typeof expected !== typeof actual) { add('type'); return differences; }
  if (expected === null || actual === null) { if (!Object.is(expected, actual)) add('null'); return differences; }
  if (typeof expected !== 'object') { if (!Object.is(expected, actual)) add(typeof expected === 'number' ? 'number-Object.is' : 'value-Object.is'); return differences; }
  if (Array.isArray(expected) !== Array.isArray(actual)) { add('array-kind'); return differences; }
  if (Array.isArray(expected) && expected.length !== actual.length) add('array-length', expected.length, actual.length);
  const aKeys = Reflect.ownKeys(expected), bKeys = Reflect.ownKeys(actual);
  if (aKeys.length !== bKeys.length || aKeys.some((key, i) => !Object.is(key, bKeys[i]))) {
    add('ordered-own-keys', aKeys.map(display), bKeys.map(display));
  }
  for (const key of aKeys) {
    if (!Object.hasOwn(actual, key)) continue;
    strictDifferences(expected[key], actual[key], path + '[' + JSON.stringify(display(key)) + ']', differences);
  }
  return differences;
}

export function inspectNumberTransport(value) {
  const rawSpecialNumbers = [], encodedTags = {'#-0': 0, '#NaN': 0, '#Infinity': 0, '#-Infinity': 0};
  const walk = (v, path) => {
    if (typeof v === 'number' && (!Number.isFinite(v) || Object.is(v, -0))) rawSpecialNumbers.push({path, value: display(v)});
    if (typeof v === 'string' && Object.hasOwn(encodedTags, v)) encodedTags[v]++;
    if (v && typeof v === 'object') for (const key of Reflect.ownKeys(v)) if (key !== 'length') walk(v[key], path + '[' + JSON.stringify(display(key)) + ']');
  };
  walk(value, '$');
  return {rawSpecialNumbers, encodedTags, rawSpecialNumberCount: rawSpecialNumbers.length};
}
