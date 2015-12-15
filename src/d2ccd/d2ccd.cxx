// d2cc: main loop of the daemon
// (c) 2015 Michał Górny
// Distributed under the terms of the 2-clause BSD license

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include <cstring>

extern "C"
{
#	include <sys/types.h>
#	include <sys/socket.h>
#	include <sys/un.h>
#	include <unistd.h>
};

#include <libd2cc/protocol.h>

int main(int argc, char* argv[])
{
	// FIXME: this is just a quick hack to get I/O rolling
	int sock = socket(AF_UNIX, SOCK_STREAM, 0);

	sockaddr_un addr = {};
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, d2cc::protocol::unix_socket_path);

	unlink(addr.sun_path);
	bind(sock, static_cast<sockaddr*>(static_cast<void*>(&addr)), sizeof(addr));

	listen(sock, 0);
	int fd = accept(sock, nullptr, nullptr);

	while (1)
	{
		char buf[4096];

		int rd = read(fd, buf, sizeof(buf));
		if (rd <= 0)
			break;
		write(1, buf, rd);
	}

	return 0;
}
