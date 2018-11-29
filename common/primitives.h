//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#pragma once

#include "types.h"

#include <unordered_map>

namespace micro_profiler
{
	template <typename T>
	class range;

	typedef range<const byte> const_byte_range;
	typedef range<byte> byte_range;

	struct address_compare
	{
		size_t operator ()(unsigned int key) const throw();
		size_t operator ()(unsigned long long int key) const throw();
		size_t operator ()(const void *key) const throw();
	};

	struct function_statistics
	{
		explicit function_statistics(count_t times_called = 0, unsigned int max_reentrance = 0, timestamp_t inclusive_time = 0, timestamp_t exclusive_time = 0, timestamp_t max_call_time = 0);

		void add_call(unsigned int level, timestamp_t inclusive_time, timestamp_t exclusive_time);

		void operator +=(const function_statistics &rhs);

		count_t times_called;
		unsigned int max_reentrance;
		timestamp_t inclusive_time;
		timestamp_t exclusive_time;
		timestamp_t max_call_time;
	};

	template <typename AddressT>
	struct function_statistics_detailed_t : function_statistics
	{
		typedef std::unordered_map<AddressT, function_statistics, address_compare> callees_map;
		typedef std::unordered_map<AddressT, count_t, address_compare> callers_map;

		callees_map callees;
		callers_map callers;
	};

	template <typename AddressT>
	struct statistics_map_detailed_t : std::unordered_map<AddressT, function_statistics_detailed_t<AddressT>, address_compare>
	{
	};

	template <typename T>
	class range
	{
	private:
		typedef T value_type;

	public:
		template <typename U>
		range(const range<U> &u);
		range(T *start, size_t length);

		T *begin() const;
		T *end() const;
		size_t length() const;
		bool inside(const T *ptr) const;

	private:
		T *_start;
		size_t _length;
	};
	


	// address_compare - inline definitions
	inline size_t address_compare::operator ()(unsigned int key) const throw()
	{	return (key >> 4) * 2654435761;	}

	inline size_t address_compare::operator ()(unsigned long long int key) const throw()
	{	return static_cast<size_t>((key >> 4) * 0x7FFFFFFFFFFFFFFF);	}

	inline size_t address_compare::operator ()(const void *key) const throw()
	{	return (*this)(reinterpret_cast<size_t>(key));	}


	// function_statistics - inline definitions
	inline function_statistics::function_statistics(count_t times_called_, unsigned int max_reentrance_, timestamp_t inclusive_time_, timestamp_t exclusive_time_, timestamp_t max_call_time_)
		: times_called(times_called_), max_reentrance(max_reentrance_), inclusive_time(inclusive_time_), exclusive_time(exclusive_time_), max_call_time(max_call_time_)
	{	}

	inline void function_statistics::add_call(unsigned int level, timestamp_t inclusive_time_, timestamp_t exclusive_time_)
	{
		++times_called;
		if (level > max_reentrance)
			max_reentrance = level;
		if (!level)
			inclusive_time += inclusive_time_;
		exclusive_time += exclusive_time_;
		if (inclusive_time_ > max_call_time)
			max_call_time = inclusive_time_;
	}

	inline void function_statistics::operator +=(const function_statistics &rhs)
	{
		times_called += rhs.times_called;
		if (rhs.max_reentrance > max_reentrance)
			max_reentrance = rhs.max_reentrance;
		inclusive_time += rhs.inclusive_time;
		exclusive_time += rhs.exclusive_time;
		if (rhs.max_call_time > max_call_time)
			max_call_time = rhs.max_call_time;
	}

	
	template <typename T>
	template <typename U>
	inline range<T>::range(const range<U> &u)
		: _start(u.begin()), _length(u.length())
	{	}

	template <typename T>
	inline range<T>::range(T *start, size_t length)
		: _start(start), _length(length)
	{	}

	template <typename T>
	inline T *range<T>::begin() const
	{	return _start;	}

	template <typename T>
	inline T *range<T>::end() const
	{	return _start + _length;	}

	template <typename T>
	inline size_t range<T>::length() const
	{	return _length;	}

	template <typename T>
	inline bool range<T>::inside(const T *ptr) const
	{	return (begin() <= ptr) & (ptr < end());	}
	

	// helper methods - inline definitions
	template <typename AddressT>
	inline void add_child_statistics(function_statistics_detailed_t<AddressT> &s, AddressT function, unsigned int level, timestamp_t inclusive_time, timestamp_t exclusive_time)
	{	s.callees[function].add_call(level, inclusive_time, exclusive_time);	}

	template <typename AddressT, typename AddressCompareT>
	inline void update_parent_statistics(std::unordered_map<AddressT, function_statistics_detailed_t<AddressT>, AddressCompareT> &s, AddressT address, const function_statistics_detailed_t<AddressT> &f)
	{
		for (typename function_statistics_detailed_t<AddressT>::callees_map::const_iterator i = f.callees.begin(); i != f.callees.end(); ++i)
			s[i->first].callers[address] = i->second.times_called;
	}
}