/*-----------------------------------------------------------------------
Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"; you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.
-----------------------------------------------------------------------*/
#ifndef CURSOR_H
#define CURSOR_H

#include <functional>

// Tracks a (current, old) pair for a value that changes over time — e.g. the
// active realization index or the pipeline time step. Callers:
//   - read current() / old() for swap detection;
//   - call set(v) when the value changes externally (duplicate sets are
//     ignored so `old` isn't clobbered by a redundant repeat push);
//   - call commit() after consuming the change, so subsequent changed()
//     returns false until the next actual change.
//
// Lessons baked into the contract (from past footguns):
//   - ParaView may push the same property value twice for a single slider
//     move; set() must ignore duplicates so the 2nd push doesn't promote
//     `old` to the new value and break swap detection.
//   - ParaView also pushes XML `default_values` at proxy creation time,
//     which is typically a hardcoded "0" not in the actual data set of
//     available indices. The set(value, validator) overload lets the caller
//     reject such values instead of clobbering the legitimate current.
template <typename T>
class Cursor
{
public:
	Cursor() = default;
	explicit Cursor(T p_initial) : _current(p_initial), _old(p_initial) {}

	T current() const { return _current; }
	T old() const { return _old; }
	bool changed() const { return _current != _old; }

	// Set a new value. Returns true if the value actually changed.
	bool set(T p_value)
	{
		if (p_value == _current) return false;
		_old = _current;
		_current = p_value;
		return true;
	}

	// Set with validation. The validator is called only if p_value differs
	// from the current value. Returns true iff the value was accepted and
	// changed.
	bool set(T p_value, const std::function<bool(const T&)>& p_validator)
	{
		if (p_value == _current) return false;
		if (!p_validator(p_value)) return false;
		_old = _current;
		_current = p_value;
		return true;
	}

	// Align old onto current — call after the caller has consumed the
	// change, so the next changed() returns false until the next set().
	void commit() { _old = _current; }

	// Force-reset both values (for initialization once the data catalog
	// is known). Does NOT count as a change for changed().
	void reset(T p_value) { _current = p_value; _old = p_value; }

private:
	T _current{};
	T _old{};
};

#endif // CURSOR_H
