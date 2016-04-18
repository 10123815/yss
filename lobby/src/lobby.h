#ifndef _LOBBY_H_
#define _LOBBY_H_

#include <map>
#include <memory>
#include "sys/mman.h"

#include "../../common/simple_server.h"
#include "data_fifo.h"
#include "lobby_logic.h"

namespace ysd_simple_server
{


	/////////////////////////////////////////////////////////
	// Lobby server get data from gateway
	// server and process them with LobbyLogic then return
	// them to the gateway server.
	/////////////////////////////////////////////////////////
	class Lobby
	{

		typedef std::map<uint16_t, std::unique_ptr<FifoDataPipe>> FIFOSet;

	public:
		Lobby (int max_size)
			: logic_ ( )
		{

			epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
			events_.resize(100);

			// block until gateway server start
			lua_fifo_fd_ = open(_LUA_FIFO_, O_RDONLY);
			if (lua_fifo_fd_ == -1)
				perror("open lua fifo ");

			epoll_event ev;
			ev.data.fd = lua_fifo_fd_;
			ev.events = EPOLLIN | EPOLLET;
			if (-1 == epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, lua_fifo_fd_, &ev))
				perror("epoll add listen gateway ");

		}

		~Lobby ( )
		{
			close(lua_fifo_fd_);
			unlink(_LUA_FIFO_);
			close(epoll_fd_);
		}
		Lobby (const Lobby& other) = delete;
		Lobby& operator= (Lobby other) = delete;

		void Run ( );

	private:

		void UserArrv ( );

		void CreateFIFO (unsigned short id);

		int lua_fifo_fd_;		// control info from gateway server

		int epoll_fd_;			// epoll for listen data from gateway server
		EpollEventSet events_;	// events from gateway server

		FIFOSet fifos_;

		// recv fifo id to uid
		std::map<int, int> recvf_uid;

		LobbyLogic logic_;
	};
}

#endif