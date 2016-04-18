#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <vector>
#include <memory>
#include <mutex>
#include <string>
#include <sstream>
#include "sys/mman.h"

#include "../../common/simple_server.h"

namespace ysd_simple_server
{

	/////////////////////////////////////////////////////////////////
	// A client maintain system resources for a client:
	// socket and IO event
	// fifo for IPC between Gateway and other backend servers;
	// ....
	/////////////////////////////////////////////////////////////////
	class Client final
	{
	public:

		Client (unsigned short id)
			: uid_ (id)
		{
		}

		// release resource when deleted
		~Client ( )
		{

			// delete lobby epoll event
			if (-1 == epoll_ctl(epoll_lobby_fd_, EPOLL_CTL_DEL, rl_fifo_fd_, NULL))
			{
				perror("delete lobby epoll ");
			}

			// delete socket epoll event
			if (-1 == epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, sock_fd_, NULL))
			{
				perror("delete socket epoll ");
			}

			// close FIFO
			close(wl_fifo_fd_);
			close(rl_fifo_fd_);

			unlink(wl_fifo_name_);
			unlink(rl_fifo_name_);

			// close socket
			close(sock_fd_);

			printf("all resources of %d have released\n", uid_);
		}

		void AddSockEvent (int epoll_fd, int sock_fd)
		{
			AddEvent(epoll_fd, sock_fd);
			epoll_fd_ = epoll_fd;
			sock_fd_ = sock_fd;
		}

		void AddLobbyEvent (int epoll_fd, int recv_fd)
		{
			AddEvent(epoll_fd, recv_fd);
			epoll_lobby_fd_= epoll_fd;
			rl_fifo_fd_ = recv_fd;
		}

		// return read fifo fd
		int CreateLobbyFIFO (std::string name);

		// receive message from the client
		void Recv ( );

		// write into the shared memory
		void SendLobby (const char* data, size_t len);

		// read from the shared memory
		void RecvLobby ( );

		unsigned short uid ( ) const { return uid_; }
		int sock_fd ( ) const { return sock_fd_; }

	private:

		void AddEvent (int epoll_fd, int listen_fd)
		{

			epoll_event ev;
			ev.data.fd = listen_fd;
			ev.events = EPOLLIN | EPOLLET;
			if (-1 == epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev))
				perror("epoll add ");

		}

		unsigned short uid_;	// unique id for this user

		int sock_fd_;			// socket connected with client

		// these is not this client's resources
		int epoll_fd_;			// epoll checked in the gateway
		int epoll_lobby_fd_;	// epoll listen from lobby server
		int epoll_game_fd_;

		// FIFO
		int wl_fifo_fd_;
		int rl_fifo_fd_;
		char rl_fifo_name_[32];
		char wl_fifo_name_[32];

		std::mutex mutex_;

	};
}

#endif