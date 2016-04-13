#ifndef _GATEWAY_H_
#define _GATEWAY_H_

#include <memory>
#include <map>
#include <stack>

#include "../../common/simple_server.h"
#include "client.h"

namespace ysd_simple_server
{

	class Gateway
	{

		// client management
		typedef std::map<int, std::unique_ptr<Client>> ClientSet;

		typedef std::stack<unsigned short> IDPool;

	public:

		Gateway (int listen_fd, size_t max_size)
			: listen_fd_(listen_fd)
		{

			// epoll to listen client write
			epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
			events_.resize(max_size);

			// epoll to listen lobby server write
			epoll_lobby_fd_ = epoll_create1(EPOLL_CLOEXEC);
			lobby_events_.resize(max_size);

			// control info for gateway server and lobby server
			// use a FIFO to transmit these info

			// FIFO to tell lobby server a new user's arrival
			if (mkfifo(_LUA_FIFO_, 0666) == -1 && errno != EEXIST)
				perror("make lua fifo ");

			// block until lobby server start
			lua_fifo_fd_ = open(_LUA_FIFO_, O_WRONLY);
			if (lua_fifo_fd_ == -1)
				perror("open lua fifo ");

			for (int i = 0; i < MaxConnection; ++i)
			{
				avai_ids_.push(i);
			}

		}

		~Gateway ( )
		{
			close(lua_fifo_fd_);
			unlink(_LUA_FIFO_);
			close(epoll_fd_);
			close(listen_fd_);
		}

		Gateway (const Gateway & other) = delete;
		Gateway& operator= (const Gateway other) = delete;

		void Run ( );

	private:

		// listen user arrival
		void NewUserArrv ( );

		// recv data from lobby server and forward to socket
		void RecvDataLobby ( );

		void CreateFIFO (std::string name, uint16_t uid, int* wfd, int* rfd, int* rrfd, int* wwfd);

		EpollEventSet events_;		// I/O events
		EpollEventSet lobby_events_;
		EpollEventSet game_events_;

		int epoll_fd_;				// epoll file descriptor to listen data from client
		int epoll_lobby_fd_;		// epoll file descriptor to listen data from lobby server
		int epoll_game_fd_;			// epoll file descriptor to listen data from game server

		int listen_fd_;				// socket file descriptor to listen user access

		int lua_fifo_fd_;			// control FIFO to notice lobby server a user's arrival

		ClientSet clients_;			// manage clients' connection
		IDPool avai_ids_;			// all available ids

		std::map<int, int> sock_uid_;	// find user id by sock
		std::map<int, int> fifo_uid_;	// find user id by recv fd from lobby server
	};
}


#endif