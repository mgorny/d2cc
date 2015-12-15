// d2cc: protocol structures and constants
// (c) 2015 Michał Górny
// Distributed under the terms of the 2-clause BSD license

#pragma once

#ifndef D2CC_PROTOCOL_H
#define D2CC_PROTOCOL_H 1

#define D2CC_ARGV_INPUT_TAG "@d2cc_input@"
#define D2CC_ARGV_OUTPUT_TAG "@d2cc_output@"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace d2cc
{
	namespace protocol
	{
		extern const char unix_socket_path[];

		typedef std::vector<uint8_t> buffer_type;
		typedef buffer_type::iterator buffer_iterator;

		struct Message
		{
			std::array<uint8_t, 4> message_type;
			uint32_t body_length;

			static constexpr uint32_t header_length = 8;

			// prepare a serialization buffer with message header
			buffer_iterator prepare_buffer(buffer_type& buf);
		};

		struct HelloMessage : Message
		{
			uint16_t protocol_version;
			uint16_t protocol_flags;

			void serialize(buffer_type& buf);
		};

		struct DiscardMessage : Message
		{
			void serialize(buffer_type& buf);
		};

		struct ArgVMessage : Message
		{
			typedef std::function<void(const char*)>
				argv_yield_func;
			typedef std::function<void(const argv_yield_func&)>
				argv_getter_func;

			void serialize(buffer_type& buf,
					const argv_getter_func& argv_get);
		};

		struct DataMessage : Message
		{
			typedef std::function<ssize_t(uint8_t* buf, size_t buf_size)>
				data_read_func;

			bool serialize(buffer_type& buf,
					const data_read_func& data_read);
		};

		struct CompileRequestMessage : Message
		{
			void serialize(buffer_type& buf);
		};
	};
};

#endif /*D2CC_PROTOCOL_H*/
