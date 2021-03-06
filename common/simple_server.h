#ifndef _SIMPLE_SERVER_H_
#define _SIMPLE_SERVER_H_

#include <thread>
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

	// three player's uid
	typedef std::pair<uint16_t, uint16_t> MatchResult;

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

#define _PKG_HEAD_SIZE_		3
#define _DATA_HEAD_SIZE_	3

	// Package struct, parse in gateway server
	//     2           1 		   len - 3
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
	// the msg type enum
	enum NetMsgType
	{
		kChat		= 0,	// bidirectional, --len2--|--uid2--|-------content--------
		kPlay		= 1,	// client to server, --uid2--
		kCancelPlay = 2,	// client to server, --uid2--
		kMatch		= 3,	// server to client, --uid2--|--uid2--
		kFloat		= 4,
		kFloat2		= 8,	// 2d vector
		kFloat3		= 16,	// 3d vector

		kConnSuc	= 126,	// server to client, --uid2--
		kConnFail	= 127,	// server to client
	};

	class Utility
	{
	public:

		// client read connection state from server
		static uint16_t ReadConnStt (int serv_sock_fd)
		{
			// data length
			short data_len = Utility::ReadPkgHead(serv_sock_fd, nullptr) - _PKG_HEAD_SIZE_;

			if (data_len != sizeof(uint16_t) + _DATA_HEAD_SIZE_)
			{
				perror("wrong connection state data length");
				return MaxConnection;
			}

			char data[data_len];
			read(serv_sock_fd, data, data_len);
			NetMsgType stt = (NetMsgType)data[2];
			if (stt == kConnSuc)
			{
				return Utility::Char_Uint16(data[3], data[4]);
			}
			else
			{
				// connect failed
				return MaxConnection;
			}
		}

		// tell client whether it connect successful and assign it user id
		// directly sent from gateway server
		static void WriteConnStt (NetMsgType conn_stt, uint16_t uid, int cln_sock)
		{

			size_t data_len = _DATA_HEAD_SIZE_ + sizeof(uint16_t);
			char out[data_len];

			// msg head
			out[0] = data_len;
			out[1] = (data_len & 0xFF00) >> 8;

			if (conn_stt != kConnSuc)
			{
				conn_stt = kConnFail;
			}
			out[2] = conn_stt;

			// assign user id to client
			out[3] = uid;
			out[4] = (uid & 0xFF00) >> 8;

			Utility::GatewayWritePkg(out, data_len, cln_sock);

		}

		// from lobby to gateway server
		static void LobbyWriteMatchInfo (const MatchResult res, int fifo_fd)
		{
			size_t data_len = sizeof(uint16_t) * 2;
			char out[_DATA_HEAD_SIZE_ + data_len];

			// head
			Utility::WriteDataHead(data_len, kMatch, out);

			// data. from low to high
			uint16_t tmp = res.first;
			out[3] = tmp;
			out[4] = (tmp & 0xFF00) >> 8;

			tmp = res.second;
			out[5] = tmp;
			out[6] = (tmp & 0xFF00) >> 8;

			write(fifo_fd, out, sizeof(out));

		}

		// lobby server send char msg to gateway server
		static void LobbyWriteChatMsg (uint16_t str_len, uint16_t uid, const char* ctt, int fifo_fd)
		{
			char out[_DATA_HEAD_SIZE_ + 4 + str_len];

			// head
			Utility::WriteDataHead(4 + str_len, kChat, out);

			// chat msg head low to high
			out[_DATA_HEAD_SIZE_ + 0] = str_len;
			out[_DATA_HEAD_SIZE_ + 1] = (str_len & 0xFF00) >> 8;
			out[_DATA_HEAD_SIZE_ + 2] = uid;
			out[_DATA_HEAD_SIZE_ + 3] = (uid & 0xFF00) >> 8;

			// chat content
			memcpy(out + 4 + _DATA_HEAD_SIZE_, ctt, str_len);

			// write to gateway server
			write(fifo_fd, out, sizeof(out));
		}

		// send data from server to client, dont need data direction
		static void GatewayWritePkg (const char* data, size_t data_len, int sock)
		{

			int len = data_len + _PKG_HEAD_SIZE_;
			char buffer[len];

			buffer[0] = len & 0x00FF;
			buffer[1] = (len & 0xFF00) >> 8;
			buffer[2] = 0;

			memcpy(buffer + 3, data, data_len);

			size_t write_size = write(sock, buffer, len);
			if (write_size != len)
				perror("write ");

		}

		// return pkg length, include pkg head
		static short ReadPkgHead (int sock, DataDir* dir)
		{
			char head_buf[_PKG_HEAD_SIZE_];
			int head_size = read(sock, head_buf, _PKG_HEAD_SIZE_);

			if (head_size < 0)
			{
				perror("read pkg head ");
				return -1;
			}
			else if (head_size == 0)
			{
				return 0;
			}

			// pkg length
			short len = (((unsigned short)head_buf[1]) << 8) + head_buf[0];

			// to where
			if (dir != nullptr)
				*dir = (DataDir)head_buf[2];

			return len;
		}

		// return data length, includes data head
		static short ReadDataHead (int fifo_Fd, NetMsgType* type)
		{
			char head_buf[_DATA_HEAD_SIZE_];
			int head_size = read(fifo_Fd, head_buf, _DATA_HEAD_SIZE_);

			if (head_size < _DATA_HEAD_SIZE_)
			{
				perror("read data head ");
				return -1;
			}

			// pkg length
			short len = (((unsigned short)head_buf[1]) << 8) + head_buf[0];

			// to where
			if (type != nullptr)
				*type = (NetMsgType)head_buf[2];

			return len;
		}

		static uint16_t Char_Uint16 (char l, char h)
		{
			uint8_t low = l;
			uint16_t high = h;
			return (high << 8) | low;
		}

	private:
		// from lobby / game server to gateway server
		static void WriteDataHead (uint16_t data_len, NetMsgType type, char* head)
		{
			// data head
			head[0] = _DATA_HEAD_SIZE_ + data_len;
			head[1] = ((_DATA_HEAD_SIZE_ + data_len) & 0xFF00) >> 8;
			head[2] = type;
		}
	};


}

#endif