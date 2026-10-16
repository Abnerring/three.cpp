#pragma once
#include "loaders/Loader.h"

// Native port of src/loaders/FileLoader.js from the pinned three.js revision (see upstream.json).
//
// The pinned class drives fetch / Request / Response / ReadableStream / AbortController /
// ProgressEvent / DOMParser / TextDecoder, none of which exist natively. That host boundary is
// reduced to one injectable module-level function, documented in docs/HANDOFF.md item 10:
//
//   transport( request ) -> response
//   request:  { url, method, headers, credentials, signal: { aborted } }
//   response: { status, statusText, url, headers: { get(name) }, body }
//
// `body` is the whole body as a string, so the loader raises a single progress event carrying the
// total byte count instead of one event per stream chunk, and a status of 0 does not emit the
// upstream console warning because there is no console here. Those are the only two
// approximations, and both are recorded in the handoff note. Everything else - the path and
// resolveURL composition, the `file:` cache key, the duplicate-request map, the responseType
// conversion, the success-only cache write, the callback fan-out and the
// itemStart/itemEnd/itemError order - follows the pinned source line by line. document and blob
// have no native counterpart and fail loudly instead of pretending.
class FileLoader : public Loader {
  public:
    explicit FileLoader(JSValue manager={});
    static JSValue prototype();
    static void setTransport(JSValue transport);
    static JSValue transport();
    JSMember mimeType{&object_,"mimeType"},responseType{&object_,"responseType"},_abortController{&object_,"_abortController"};
    FileLoader& setResponseType(JSValue value);
    FileLoader& setMimeType(JSValue value);
    FileLoader& abort() override;
    JSValue load(JSValue url={},JSValue onLoad={},JSValue onProgress={},JSValue onError={}) override;
    static constexpr const char* RESPONSE_TYPES_SUPPORTED="arraybuffer, text, json";
};