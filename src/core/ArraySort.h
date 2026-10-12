#pragma once

// Native port of ArrayTimSortImpl and its helpers from Node.js v26.5.0,
// V8 14.6.202.34-node.24, deps/v8/third_party/v8/builtins/array-sort.tq.
// Source SHA-256: 431b4a52e984d0e3add8bf2f0929a73390ad867b3bcebf41cc071d0c3c9e5310
// https://github.com/nodejs/node/blob/v26.5.0/deps/v8/third_party/v8/builtins/array-sort.tq
// Changes: C++ vectors replace FixedArrays; signed native indices replace Smis;
// dense native values use a supplied numeric comparator. Receiver properties,
// holes and undefined compaction belong to the caller, outside this algorithm.
// Copyright (c) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
// 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Python Software Foundation;
// All Rights Reserved
// Complete upstream license chain and adaptation notice: ArraySort.LICENSE.
/*
Copyright 2014, the V8 project authors. All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of Google Inc. nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace ArraySort_detail {
template <class T, class Comparator> class TimSort {
    using Index = std::ptrdiff_t;
    struct Run { Index base, length; };
    std::vector<T> work, temporary;
    std::vector<Run> pending;
    Comparator& comparator;
    Index minGallop = 7;

    double compare(const T& a, const T& b) {
        const double result = comparator(a, b);
        return std::isnan(result) ? 0 : result;
    }
    static void copy(const std::vector<T>& from, Index source, std::vector<T>& to, Index target, Index count) {
        if (source < target) {
            for (Index i = count; i > 0; --i) to[target + i - 1] = from[source + i - 1];
        } else {
            for (Index i = 0; i < count; ++i) to[target + i] = from[source + i];
        }
    }
    static Index growOffset(Index offset, Index maximum) {
        return offset > (maximum - 1) / 2 ? maximum : offset * 2 + 1;
    }
    void binaryInsertion(Index low, Index start, Index high) {
        if (low == start) ++start;
        for (; start < high; ++start) {
            Index left = low, right = start;
            const T pivot = work[right];
            while (left < right) {
                const auto middle = left + ((right - left) >> 1);
                if (compare(pivot, work[middle]) < 0) right = middle;
                else left = middle + 1;
            }
            for (Index p = start; p > left; --p) work[p] = work[p - 1];
            work[left] = pivot;
        }
    }
    Index countRun(Index low, Index high) {
        if (low + 1 == high) return 1;
        Index length = 2;
        T previous = work[low + 1];
        const bool descending = compare(previous, work[low]) < 0;
        for (Index i = low + 2; i < high; ++i) {
            const T current = work[i];
            const auto order = compare(current, previous);
            if (descending ? order >= 0 : order < 0) break;
            previous = current; ++length;
        }
        if (descending) std::reverse(work.begin() + low, work.begin() + low + length);
        return length;
    }
    Index gallopLeft(const std::vector<T>& array, const T& key, Index base, Index length, Index hint) {
        Index last = 0, offset = 1;
        if (compare(array[base + hint], key) < 0) {
            const auto maximum = length - hint;
            while (offset < maximum) {
                if (compare(array[base + hint + offset], key) >= 0) break;
                last = offset; offset = growOffset(offset, maximum);
            }
            offset = std::min(offset, maximum);
            last += hint; offset += hint;
        } else {
            const auto maximum = hint + 1;
            while (offset < maximum) {
                if (compare(array[base + hint - offset], key) < 0) break;
                last = offset; offset = growOffset(offset, maximum);
            }
            offset = std::min(offset, maximum);
            const auto saved = last; last = hint - offset; offset = hint - saved;
        }
        ++last;
        while (last < offset) {
            const auto middle = last + ((offset - last) >> 1);
            if (compare(array[base + middle], key) < 0) last = middle + 1;
            else offset = middle;
        }
        return offset;
    }
    Index gallopRight(const std::vector<T>& array, const T& key, Index base, Index length, Index hint) {
        Index last = 0, offset = 1;
        if (compare(key, array[base + hint]) < 0) {
            const auto maximum = hint + 1;
            while (offset < maximum) {
                if (compare(key, array[base + hint - offset]) >= 0) break;
                last = offset; offset = growOffset(offset, maximum);
            }
            offset = std::min(offset, maximum);
            const auto saved = last; last = hint - offset; offset = hint - saved;
        } else {
            const auto maximum = length - hint;
            while (offset < maximum) {
                if (compare(key, array[base + hint + offset]) < 0) break;
                last = offset; offset = growOffset(offset, maximum);
            }
            offset = std::min(offset, maximum);
            last += hint; offset += hint;
        }
        ++last;
        while (last < offset) {
            const auto middle = last + ((offset - last) >> 1);
            if (compare(key, array[base + middle]) < 0) offset = middle;
            else last = middle + 1;
        }
        return offset;
    }
    void mergeLow(Index baseA, Index lengthA, Index baseB, Index lengthB) {
        temporary.assign(work.begin() + baseA, work.begin() + baseA + lengthA);
        Index destination = baseA, cursorA = 0, cursorB = baseB, gallop = minGallop;
        work[destination++] = work[cursorB++];
        if (--lengthB == 0) goto succeed;
        if (lengthA == 1) goto copyB;
        for (;;) {
            Index winsA = 0, winsB = 0;
            for (;;) {
                if (compare(work[cursorB], temporary[cursorA]) < 0) {
                    work[destination++] = work[cursorB++];
                    ++winsB; --lengthB; winsA = 0;
                    if (lengthB == 0) goto succeed;
                    if (winsB >= gallop) break;
                } else {
                    work[destination++] = temporary[cursorA++];
                    ++winsA; --lengthA; winsB = 0;
                    if (lengthA == 1) goto copyB;
                    if (winsA >= gallop) break;
                }
            }
            ++gallop;
            bool first = true;
            while (winsA >= 7 || winsB >= 7 || first) {
                first = false; gallop = std::max<Index>(1, gallop - 1); minGallop = gallop;
                winsA = gallopRight(temporary, work[cursorB], cursorA, lengthA, 0);
                if (winsA > 0) {
                    copy(temporary, cursorA, work, destination, winsA);
                    destination += winsA; cursorA += winsA; lengthA -= winsA;
                    if (lengthA == 1) goto copyB;
                    if (lengthA == 0) goto succeed;
                }
                work[destination++] = work[cursorB++];
                if (--lengthB == 0) goto succeed;
                winsB = gallopLeft(work, temporary[cursorA], cursorB, lengthB, 0);
                if (winsB > 0) {
                    copy(work, cursorB, work, destination, winsB);
                    destination += winsB; cursorB += winsB; lengthB -= winsB;
                    if (lengthB == 0) goto succeed;
                }
                work[destination++] = temporary[cursorA++];
                if (--lengthA == 1) goto copyB;
            }
            minGallop = ++gallop;
        }
      succeed:
        if (lengthA > 0) copy(temporary, cursorA, work, destination, lengthA);
        return;
      copyB:
        copy(work, cursorB, work, destination, lengthB);
        work[destination + lengthB] = temporary[cursorA];
    }
    void mergeHigh(Index baseA, Index lengthA, Index baseB, Index lengthB) {
        temporary.assign(work.begin() + baseB, work.begin() + baseB + lengthB);
        Index destination = baseB + lengthB - 1, cursorB = lengthB - 1, cursorA = baseA + lengthA - 1;
        Index gallop = minGallop;
        work[destination--] = work[cursorA--];
        if (--lengthA == 0) goto succeed;
        if (lengthB == 1) goto copyA;
        for (;;) {
            Index winsA = 0, winsB = 0;
            for (;;) {
                if (compare(temporary[cursorB], work[cursorA]) < 0) {
                    work[destination--] = work[cursorA--];
                    ++winsA; --lengthA; winsB = 0;
                    if (lengthA == 0) goto succeed;
                    if (winsA >= gallop) break;
                } else {
                    work[destination--] = temporary[cursorB--];
                    ++winsB; --lengthB; winsA = 0;
                    if (lengthB == 1) goto copyA;
                    if (winsB >= gallop) break;
                }
            }
            ++gallop;
            bool first = true;
            while (winsA >= 7 || winsB >= 7 || first) {
                first = false; gallop = std::max<Index>(1, gallop - 1); minGallop = gallop;
                winsA = lengthA - gallopRight(work, temporary[cursorB], baseA, lengthA, lengthA - 1);
                if (winsA > 0) {
                    destination -= winsA; cursorA -= winsA;
                    copy(work, cursorA + 1, work, destination + 1, winsA);
                    lengthA -= winsA;
                    if (lengthA == 0) goto succeed;
                }
                work[destination--] = temporary[cursorB--];
                if (--lengthB == 1) goto copyA;
                winsB = lengthB - gallopLeft(temporary, work[cursorA], 0, lengthB, lengthB - 1);
                if (winsB > 0) {
                    destination -= winsB; cursorB -= winsB;
                    copy(temporary, cursorB + 1, work, destination + 1, winsB);
                    lengthB -= winsB;
                    if (lengthB == 1) goto copyA;
                    if (lengthB == 0) goto succeed;
                }
                work[destination--] = work[cursorA--];
                if (--lengthA == 0) goto succeed;
            }
            minGallop = ++gallop;
        }
      succeed:
        if (lengthB > 0) copy(temporary, 0, work, destination - (lengthB - 1), lengthB);
        return;
      copyA:
        destination -= lengthA; cursorA -= lengthA;
        copy(work, cursorA + 1, work, destination + 1, lengthA);
        work[destination] = temporary[cursorB];
    }
    void mergeAt(Index i) {
        Index baseA = pending[i].base, lengthA = pending[i].length;
        const Index baseB = pending[i + 1].base;
        Index lengthB = pending[i + 1].length;
        pending[i].length = lengthA + lengthB;
        if (i == Index(pending.size()) - 3) pending[i + 1] = pending[i + 2];
        pending.pop_back();
        const auto skip = gallopRight(work, work[baseB], baseA, lengthA, 0);
        baseA += skip; lengthA -= skip;
        if (lengthA == 0) return;
        lengthB = gallopLeft(work, work[baseA + lengthA - 1], baseB, lengthB, lengthB - 1);
        if (lengthB == 0) return;
        if (lengthA <= lengthB) mergeLow(baseA, lengthA, baseB, lengthB);
        else mergeHigh(baseA, lengthA, baseB, lengthB);
    }
    bool invariant(Index n) const {
        return n < 2 || pending[n - 2].length > pending[n - 1].length + pending[n].length;
    }
    void collapse() {
        while (pending.size() > 1) {
            Index n = Index(pending.size()) - 2;
            if (!invariant(n + 1) || !invariant(n)) {
                if (pending[n - 1].length < pending[n + 1].length) --n;
                mergeAt(n);
            } else if (pending[n].length <= pending[n + 1].length) mergeAt(n);
            else break;
        }
    }
  public:
    TimSort(const std::vector<T>& input, Comparator& compare) : work(input), comparator(compare) {}
    void run(std::vector<T>& receiver) {
        if (work.size() > std::size_t(std::numeric_limits<Index>::max())) throw std::length_error("Array sort length exceeds native indices");
        const auto initial = work.size();
        Index remaining = Index(initial), low = 0, minimum = remaining, trailing = 0;
        if (remaining < 2) return;
        // ArrayTimSort bypasses run detection for fewer than eight elements.
        if (remaining < 8) {
            binaryInsertion(0, 0, remaining);
            remaining = 0;
        }
        while (minimum >= 64) { trailing |= minimum & 1; minimum >>= 1; }
        minimum += trailing;
        while (remaining != 0) {
            auto length = countRun(low, low + remaining);
            if (length < minimum) {
                const auto forced = std::min(minimum, remaining);
                binaryInsertion(low, low + length, low + forced); length = forced;
            }
            pending.push_back({low, length}); collapse();
            low += length; remaining -= length;
        }
        while (pending.size() > 1) {
            auto n = Index(pending.size()) - 2;
            if (n > 0 && pending[n - 1].length < pending[n + 1].length) --n;
            mergeAt(n);
        }
        // Array.sort writes back the original span, preserving comparator-appended elements.
        if (receiver.size() > initial) work.insert(work.end(), receiver.begin() + Index(initial), receiver.end());
        receiver.swap(work);
    }
};
} // namespace ArraySort_detail

// A dense-array sort with the pinned V8 comparison order. T must be copyable;
// comparator returns a JS numeric comparison result, including NaN or infinities.
template <class T, class Comparator> void v8ArraySort(std::vector<T>& values, Comparator&& comparator) {
    ArraySort_detail::TimSort<T, std::remove_reference_t<Comparator>> state(values, comparator);
    state.run(values);
}
