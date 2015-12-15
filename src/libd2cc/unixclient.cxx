// d2cc: UNIX socket client connection library
// (c) 2015 Michał Górny
// Distributed under the terms of the 2-clause BSD license

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include <cerrno>
#include <cstddef>
#include <cstring>
#include <iostream>

extern "C"
{
#	include <sys/types.h>
#	include <sys/socket.h>
#	include <sys/un.h>
#	include <unistd.h>
};

#include "protocol.h"
#include "unixclient.h"

bool d2cc::UNIXClient::send(const std::vector<uint8_t>& data)
{
	const uint8_t* buf = data.data();
	size_t left = data.size();

	while (left > 0)
	{
		ssize_t wr = write(socket_, buf, left);

		if (wr == -1)
		{
			if (errno == EINTR)
				continue;
			return false;
		}

		buf += wr;
		left -= wr;
	}

	return true;
}
	
d2cc::UNIXClient::UNIXClient()
	: socket_(-1), in_request_(false), discard_msgs_(0)
{
}

d2cc::UNIXClient::~UNIXClient()
{
	if (socket_ != -1)
	{
		if (in_request_)
			discard_request();

		close(socket_);
	}
}

bool d2cc::UNIXClient::connect()
{
	socket_ = socket(AF_UNIX, SOCK_STREAM, 0);
	if (socket_ == -1)
	{
		std::cerr << "d2cc: Unable to create UNIX socket: "
			<< strerror(errno) << std::endl;
		return false;
	}

	// basic struct members + path length + null terminator
	constexpr size_t addr_length = offsetof(sockaddr_un, sun_path)
			+ sizeof(D2CC_SOCKET_PATH);
	char addr_buf[addr_length] = {};
	// yes, I love berkeley sockets
	sockaddr_un* addr = reinterpret_cast<sockaddr_un*>(addr_buf);
	sockaddr* addr_cast = reinterpret_cast<sockaddr*>(addr);

	addr->sun_family = AF_UNIX;
	// sizeof counts both the string and the null terminator
	memcpy(addr->sun_path, D2CC_SOCKET_PATH, sizeof(D2CC_SOCKET_PATH));

	if (::connect(socket_, addr_cast, addr_length) == -1)
	{
		std::cerr << "d2cc: Unable to connect to d2cc server: "
			<< strerror(errno) << std::endl;
		return false;
	}

	protocol::HelloMessage hello;
	hello.protocol_version = 0x0000;
	hello.protocol_flags = 0x0000;

	std::vector<uint8_t> hello_msg;
	hello.serialize(hello_msg);
	if (!send(hello_msg))
	{
		std::cerr << "d2cc: Unable to send hello message: "
			<< strerror(errno) << std::endl;
		return false;
	}

	return true;
}

bool d2cc::UNIXClient::send_argv(const d2cc::protocol::ArgVMessage::argv_getter_func& user_func)
{
	std::vector<uint8_t> buffer;

	d2cc::protocol::ArgVMessage msg;
	msg.serialize(buffer, user_func);

	if (!send(buffer))
	{
		std::cerr << "d2cc: Unable to send argv message: "
			<< strerror(errno) << std::endl;
		return false;
	}

	in_request_ = true;
	return true;
}

bool d2cc::UNIXClient::send_data(const d2cc::protocol::DataMessage::data_read_func& data_read)
{
	std::vector<uint8_t> buffer;

	d2cc::protocol::DataMessage msg;
	while (true)
	{
		buffer.clear();
		if (!msg.serialize(buffer, data_read))
			return false;
		if (buffer.empty())
			break;

		if (!send(buffer))
		{
			std::cerr << "d2cc: Unable to send data message: "
				<< strerror(errno) << std::endl;
			return false;
		}
	}

	in_request_ = true;
	return true;
}

bool d2cc::UNIXClient::finish_request()
{
	std::vector<uint8_t> buffer;

	d2cc::protocol::CompileRequestMessage req;
	req.serialize(buffer);

	if (!send(buffer))
	{
		std::cerr << "d2cc: Unable to send CREQ message: "
			<< strerror(errno) << std::endl;
		return false;
	}

	in_request_ = true;
	return true;
}

bool d2cc::UNIXClient::discard_request()
{
	std::vector<uint8_t> buffer;

	d2cc::protocol::DiscardMessage req;
	req.serialize(buffer);

	if (!send(buffer))
	{
		std::cerr << "d2cc: Unable to send DSCR message: "
			<< strerror(errno) << std::endl;
		return false;
	}

	in_request_ = false;
	++discard_msgs_;
	return true;
}
