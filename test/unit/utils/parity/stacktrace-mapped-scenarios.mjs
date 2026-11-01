import StackTrace from 'file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/core/StackTrace.js';

export function leaf(request) {
  if (request.operation === 'throw') throw 17;
  if (request.operation === 'capture-middle') {
    const target = {};
    Error.captureStackTrace(target, middle);
    return target.stack;
  }
  if (request.operation === 'capture-absent') {
    const target = {};
    Error.captureStackTrace(target, absent);
    return target.stack;
  }
  if (request.operation === 'lazy') return new Error('lazy');
  if (request.operation === 'default-filtered') return new StackTrace().stack;
  return new Error('boom').stack;
}
export function middle(request) {
  return leaf(request);
}
export function outer(request) {
  return middle(request);
}
export function absent() {}
export function formatter(error, frames) {
  const nested = new Error('nested').stack;
  return {names: frames.map(frame => frame.getFunctionName()), nested: new StackTrace(nested).stack};
}
export function resume() {
  return new Error('resumed').stack;
}
export function schedule() {
  return Promise.resolve().then(resume);
}
