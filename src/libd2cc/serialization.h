// d2cc: serialization helpers
// (c) 2015 Michał Górny
// Distributed under the terms of the 2-clause BSD license

#pragma once

#ifndef D2CC_SERIALIZATION_H
#define D2CC_SERIALIZATION_H 1

#include <array>
#include <vector>

namespace d2cc
{
	namespace copy
	{
		template <class iterator_type>
		inline iterator_type uint8(iterator_type it, uint8_t i)
		{
			*it = i;
			return ++it;
		}

		template <class iterator_type>
		inline iterator_type uint16(iterator_type it, uint16_t i)
		{
			*it = i >> 8;
			*++it = i;
			return ++it;
		}

		template <class iterator_type>
		inline iterator_type uint32(iterator_type it, uint32_t i)
		{
			*it = i >> 24;
			*++it = i >> 16;
			*++it = i >> 8;
			*++it = i;
			return ++it;
		}

		template <class iterator_type, size_t array_size>
		inline iterator_type uint8_array(iterator_type it,
				const std::array<uint8_t, array_size>& input)
		{
			return std::copy(input.begin(), input.end(), it);
		};
	};
};

#endif /*D2CC_SERIALIZATION_H*/
