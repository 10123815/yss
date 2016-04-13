#ifndef _SIMPLE_SERVER_H_
#define _SIMPLE_SERVER_H_

#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <cstdlib>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/wait.h>

namespace ysd_simple_server
{

#define _LUA_FIFO_ 		"/home/ysd/ysd/simple_server/lua_fifo"
#define _DATA_FIFO_		"/home/ysd/ysd/simple_server/"

	typedef std::vector<epoll_event> EpollEventSet;

	struct UserArr
	{
		unsigned short id;
		char arr_lea;		// 0 == arr, 1 == leave
	};

	struct Float2
	{
		float x;
		float y;
	};

	struct Float3
	{
		float x;
		float y;
		float z;
	};

	const int MaxConnection = 1000;

	enum ServerClientSignal
	{
		kConnectSuccess = 10000,
		kConnectFailed = 1,
	};

#define _PKG_HEAD_SIZE_		3
#define _DATA_HEAD_SIZE_	3

	// Package struct, parse in gateway server
	//     2           1 		   len - 4
	//-----len-----/--dir--/-------data---------//
	//--------head----------/
	enum DataDir
	{
		kLobby 	= 0,
		kGame	= 1,
	};

	// Data struct, parse in lobby/game server
	//		2			1				len - 3
	//-----len-----/--type--/------------msg--------------//
	//--------head----------/
	enum NetMsgType
	{
		kChat		= 0,	// bidirectional, --len--|--uid--|-------content--------
		kPlay		= 1,	// client to server, --uid--
		kInt 		= 2,
		kFloat		= 4,
		kFloat2		= 8,	// 2d vector
		kFloat3		= 16,	// 3d vector
		kConnStt	= 32
	};

	class Utility
	{
	public:

		template<typename T>
		static void WriteMsg (T data, char* out, uint16_t len, NetMsgType type)
		{
			// low to high
			out[0] = len & 0x00FF;
			out[1] = (len & 0xFF00) >> 8;
			out[2] = type;

			// TODO : write data to out
		}

		// send data from server to client, dont need data direction
		static void WritePkg (const char& data, size_t data_len, int sock)
		{

			int len = data_len + _PKG_HEAD_SIZE_;
			char buffer[len];

			buffer[0] = len & 0x00FF;
			buffer[1] = (len & 0xFF00) >> 8;
			buffer[2] = 0;

			// low to high
			for (int i = _PKG_HEAD_SIZE_; i < len; ++i)
			{
				buffer[i] = data >> (8 * (i - _PKG_HEAD_SIZE_));
			}

			size_t write_size = write(sock, buffer, len);
			if (write_size != len)
				perror("write ");

		}

		// return pkg length, T == DataDir / NetMstType
		template <typename T>
		static short ReadHead (int sock, T* dir)
		{
			char head_buf[_PKG_HEAD_SIZE_];
			int head_size = read(sock, head_buf, _PKG_HEAD_SIZE_);

			if (head_size < 0)
			{
				perror("read head ");
				return -1;
			}
			else if (head_size == 0)
			{
				return 0;
			}

			// pkg length
			short len = (((unsigned short)head_buf[1]) << 8) + head_buf[0];

			// to where
			*dir = (T)head_buf[2];

			return len;
		}

	};


}

#endif