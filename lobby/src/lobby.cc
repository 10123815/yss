#include "lobby.h"
#include <iostream>

using namespace ysd_simple_server;

// region lobby

void Lobby::Run ( )
{

	while (1)
	{
		int nfds = epoll_wait(epoll_fd_, &*events_.begin(), events_.size(), -1);
		if (nfds == -1)
			perror("epoll wait ");

		if (nfds == events_.size())
			events_.resize(nfds * 2);

		// Data struct
		//		2			1				len - 3
		//-----len-----/--type--/------------msg--------------//
		//--------head----------/
		for (int i = 0; i < nfds; ++i)
		{
			if (events_[i].data.fd == lua_fifo_fd_)
			{
				// new user com / old user leave
				UserArrv();
			}
			else if (events_[i].events & EPOLLIN)
			{
				// user message arrival
				int fifo_in_fd = events_[i].data.fd;

				NetMsgType type;
				short len = Utility::ReadDataHead(fifo_in_fd, &type);
				if (len <= _DATA_HEAD_SIZE_)
					continue;

				char msg[len - _DATA_HEAD_SIZE_];
				if (type == kChat)
				{
					// chat message
					// kChat --len--|--uid--|-------content--------

					char chat_head[4];
					int head_size = read(fifo_in_fd, chat_head, 4);

					if (head_size != 4)
					{
						perror("read char msg head ");
						continue;
					}

					// from who
					uint16_t uid = Utility::Char_Uint16(chat_head[2], chat_head[3]);

					// chat msg length
					uint16_t len = Utility::Char_Uint16(chat_head[0], chat_head[1]);

					char chat_msg[len];
					int msg_size = read(fifo_in_fd, chat_msg, len);
					if (msg_size != len)
						continue;

					// broadcast to another player
					for (const auto& fifo : fifos_)
					{
						int this_uid = fifo.first;
						if (this_uid != uid)
						{
							int send_fd = fifo.second->send_fd();
							Utility::LobbyWriteChatMsg(len, uid, chat_msg, send_fd);
						}
					}

				}
				// only add/remove uid to the queue in this loop
				// do not return the match result
				else if (type == kPlay)
				{
					// player want to play
					// kPlay --uid--
					char data[2];
					if (2 != read(fifo_in_fd, data, 2))
						continue;

					uint16_t uid = Utility::Char_Uint16(data[0], data[1]);
					logic_.WantPlay(uid);
				}
				else if (type == kCancelPlay)
				{
					// player cancel match
					//kCancelPlayer --uid--
					char data[2];
					if (2 != read(fifo_in_fd, data, 2))
						continue;

					uint16_t uid = Utility::Char_Uint16(data[0], data[1]);
					logic_.CancelMatch(uid);
				}
			}
		}

		// after the completion of all events, return the match results
		// this make sure that the player cancel to play will be removed from the queue before
		// it returned by the match result
		MatchResult match_res;
		while (logic_.GetMatch(&match_res))
		{
			int first_fifo = fifos_[match_res.first]->send_fd();
			int second_fifo = fifos_[match_res.second]->send_fd();
			Utility::LobbyWriteMatchInfo(match_res, first_fifo);
			Utility::LobbyWriteMatchInfo(match_res, second_fifo);
		}

	}

}

void Lobby::UserArrv ( )
{

	UserArr arr_info;
	size_t kb = sizeof(arr_info);
	bzero(&arr_info, kb);
	int size = read(lua_fifo_fd_, &arr_info, kb);
	if (kb != size)
		perror("read fifo ctl ");

	if (arr_info.arr_lea == 0)
	{
		// new user arraivl

		// TODO : create game entity
		CreateFIFO(arr_info.id);
	}
	else
	{
		// user close
		auto ite = fifos_.find(arr_info.id);
		if (ite != fifos_.end())
		{
			fifos_[arr_info.id].reset(nullptr);
			fifos_.erase(arr_info.id);
		}
	}

}

void Lobby::CreateFIFO (unsigned short id)
{
	// TODO : link shared memory
	std::unique_ptr<FifoDataPipe> fifo(new FifoDataPipe(id, "lobby"));

	// add listen
	epoll_event ev;
	int read_fd = fifo->recv_fd();
	ev.data.fd = read_fd;
	ev.events = EPOLLIN | EPOLLET;
	if (-1 == epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, read_fd, &ev))
		perror("epoll add listen gateway ");

	fifos_[id] = std::unique_ptr<FifoDataPipe>(fifo.release());
	recvf_uid[read_fd] = id;
}

// endregion