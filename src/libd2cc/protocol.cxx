// d2cc: protocol structures and constants
// (c) 2015 Michał Górny
// Distributed under the terms of the 2-clause BSD license

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "protocol.h"
#include "serialization.h"

#include <cstring>
#include <iterator>

extern "C"
{
#	include <arpa/inet.h>
#	include <unistd.h>
};

const char d2cc::protocol::unix_socket_path[] = D2CC_SOCKET_PATH;

d2cc::protocol::buffer_iterator
d2cc::protocol::Message::prepare_buffer(buffer_type& buf)
{
	buffer_type::size_type prev_size = buf.size();
	buf.resize(prev_size + header_length + body_length);

	buffer_iterator it = buf.begin();
	std::advance(it, prev_size);

	it = copy::uint8_array(it, message_type);
	it = copy::uint32(it, header_length + body_length);

	return it;
}

void d2cc::protocol::HelloMessage::serialize(buffer_type& buf)
{
	constexpr std::array<uint8_t, 4> hello_type{'D', '2', 'C', 'C'};
	copy::uint8_array(message_type.begin(), hello_type);
	body_length = 4;

	buffer_iterator it = Message::prepare_buffer(buf);
	it = copy::uint16(it, protocol_version);
	it = copy::uint16(it, protocol_flags);
}

void d2cc::protocol::DiscardMessage::serialize(buffer_type& buf)
{
	constexpr std::array<uint8_t, 4> dscr_type{'D', 'S', 'C', 'R'};
	copy::uint8_array(message_type.begin(), dscr_type);
	body_length = 0;

	Message::prepare_buffer(buf);
}

void d2cc::protocol::ArgVMessage::serialize(buffer_type& buf,
		const argv_getter_func& argv_get)
{
	constexpr std::array<uint8_t, 4> argv_type{'A', 'R', 'G', 'V'};
	copy::uint8_array(message_type.begin(), argv_type);
	body_length = 0; // initially empty

	buffer_type::size_type initial_size = buf.size();
	Message::prepare_buffer(buf);

	argv_get([&buf] (const char* val)
		{
			// (yield() func)
			const uint8_t* p = reinterpret_cast<const uint8_t*>(val);
			do
			{
				buf.push_back(*p);
			}
			while (*p++ != '\0');
		});

	buffer_iterator it = buf.begin();
	std::advance(it, initial_size + 4);

	body_length = buf.size() - initial_size - header_length;
	copy::uint32(it, header_length + body_length);
}

bool d2cc::protocol::DataMessage::serialize(buffer_type& buf,
		const data_read_func& data_read)
{
	constexpr std::array<uint8_t, 4> argv_type{'D', 'A', 'T', 'A'};
	copy::uint8_array(message_type.begin(), argv_type);
	body_length = BUFSIZ; // initial data size

	buffer_type::size_type initial_size = buf.size();
	Message::prepare_buffer(buf);

	// in the future, we may be able to use continuous iterator here
	ssize_t rd = data_read(&buf.data()[initial_size + header_length],
			body_length);

	if (rd <= 0)
	{
		buf.resize(initial_size);
		return (rd == 0);
	}

	// shrink the data if necessary
	if (rd != body_length)
	{
		body_length = rd;
		buf.resize(initial_size + header_length + body_length);
	}

	buffer_iterator it = buf.begin();
	std::advance(it, initial_size + 4);

	copy::uint32(it, header_length + body_length);

	return true;
}

void d2cc::protocol::CompileRequestMessage::serialize(buffer_type& buf)
{
	constexpr std::array<uint8_t, 4> creq_type{'C', 'R', 'E', 'Q'};
	copy::uint8_array(message_type.begin(), creq_type);
	body_length = 0;

	Message::prepare_buffer(buf);
}
