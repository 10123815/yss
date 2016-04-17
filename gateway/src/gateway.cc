#include "gateway.h"

using namespace ysd_simple_server;

// set file descriptor non block
void setnonblocking(int sock)
{
	int opts;
	opts = fcntl(sock, F_GETFL);
	if (opts < 0)
	{
		perror("fcntl(sock,GETFL)");
		exit(1);
	}
	opts = opts | O_NONBLOCK;
	if (fcntl(sock, F_SETFL, opts) < 0)
	{
		perror("fcntl(sock,SETFL,opts)");
		exit(1);
	}
}

// region gateway class

void Gateway::CreateFIFO (std::string name, unsigned short uid, int* wfd, int* rfd, int* rrfd, int* wwfd)
{
	std::stringstream ss;
	ss << name << "_" << uid;
	const char* read_name = ss.str().c_str();
	if (-1 == mkfifo(read_name, 0666))
		perror("make client read fifo ");

	*rfd = open(read_name, O_RDONLY | O_NONBLOCK);
	*wwfd = open(read_name, O_WRONLY | O_NONBLOCK);
	if (*rfd == -1 || *wwfd == -1)
		perror("open client read fifo ");

	ss.clear();
	ss << name << "_" << uid * 2;
	const char* write_name = ss.str().c_str();
	if (-1 == mkfifo(write_name, 0666))
		perror("make client write fifo ");

	*rrfd = open(write_name, O_RDONLY | O_NONBLOCK);

}

void Gateway::NewUserArrv ( )
{

	// user addr info
	sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	int client_sock_fd = accept(listen_fd_, (sockaddr*)&client_addr, &client_addr_len);
	if (client_sock_fd == -1)
		perror("accept error ");

	if (clients_.size() < MaxConnection)
	{
		// set this fd nonblocking
		setnonblocking(client_sock_fd);

		// create a new client

		// available user id
		const uint16_t uid = avai_ids_.top();
		avai_ids_.pop();

		// client hold resources it use,
		// suck as socket, shared memory...
		std::unique_ptr<Client> client(new Client(uid));

		// add user data arrival listener
		// events = EPOLLIN | EPOLLET
		client->AddSockEvent(epoll_fd_, client_sock_fd);

		// let lobby server open fifo,
		// this will block lobby server until client open the fifo
		UserArr arr_info;
		arr_info.id = uid;
		arr_info.arr_lea = 0;
		write(lua_fifo_fd_, &arr_info, sizeof(UserArr));

		// create fifo to lobby server
		// this will block gateway server until lobby server open the fifo
		int recv_fd = client->CreateLobbyFIFO("lobby");

		// add lobby server fifo data arrival listener
		client->AddLobbyEvent(epoll_lobby_fd_, recv_fd);

		// TODO : create shared memory with game server
		// TODO : add game server data arrival listener

		// add client
		sock_uid_[client_sock_fd] = uid;
		fifo_uid_[recv_fd] = uid;
		clients_[uid] = std::unique_ptr<Client>(client.release());

		// return success to client
		Utility::WriteConnStt(kConnSuc, uid, client_sock_fd);

		printf("uid:%d connected. \n", uid);

	}
	else
	{
		// reject
		Utility::WriteConnStt(kConnFail, MaxConnection, client_sock_fd);
	}


}

void Gateway::RecvDataLobby ( )
{
	while (1)
	{
		int nfds = epoll_wait(epoll_lobby_fd_, &*lobby_events_.begin(), lobby_events_.size(), -1);
		if (nfds == -1)
			perror("epoll wait lobby ");

		if (nfds == lobby_events_.size())
			lobby_events_.resize(nfds * 2);

		for (int i = 0; i < nfds; ++i)
		{
			if (lobby_events_[i].events & EPOLLIN)
			{
				// some data from lobby server
				int fifo_id = lobby_events_[i].data.fd;
				uint16_t uid = fifo_uid_[fifo_id];

				// send to socket
				char buffer[255];
				size_t size = read(fifo_id, buffer, 255);
				int sock = clients_[uid]->sock_fd();
				Utility::GatewayWritePkg(buffer, size, sock);
			}
		}

	}
}

void Gateway::Run ( )
{

	epoll_event event;
	event.data.fd = listen_fd_;
	event.events = EPOLLIN | EPOLLET;	// listen read event
	epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd_, &event);

	// TODO : recv data from logic server and forward to socket
	auto func_obj = std::bind(&Gateway::RecvDataLobby, this);
	std::thread send_client_thr(func_obj);
	send_client_thr.detach();

	// recv data from socket and forward to logic server
	while (1)
	{
		// events_ will filled by io event
		int nfds = epoll_wait(epoll_fd_, &*events_.begin(), events_.size(), -1);
		if (nfds == -1)
		{
			perror("epoll wait");
		}

		if (nfds == events_.size())
			events_.resize(nfds * 2);

		for (int i = 0; i < nfds; ++i)
		{
			if (events_[i].data.fd == listen_fd_)
			{
				NewUserArrv();
			}
			else if (events_[i].events & EPOLLIN)
			{
				// a client socket want to wirte data to server
				int cln_sock_fd = events_[i].data.fd;
				if (cln_sock_fd < 0)
					perror("client socket");

				uint16_t uid = sock_uid_[cln_sock_fd];

				// read data length
				DataDir dir;
				short len = Utility::ReadPkgHead(cln_sock_fd, &dir);

				if (len == 0)
				{
					// client close tcp
					printf("client close\n");

					// TODO : let lobby server detach shared memory

					uint16_t id = sock_uid_[cln_sock_fd];
					UserArr lea_info;
					lea_info.id = id;
					lea_info.arr_lea = 1;
					write(lua_fifo_fd_, &lea_info, sizeof(UserArr));

					// delete client
					avai_ids_.push(id);
					clients_[id].reset(nullptr);
					clients_.erase(id);
				}
				else
				{
					size_t data_len = len - _PKG_HEAD_SIZE_;
					char data[data_len];
					read(cln_sock_fd, data, sizeof(data));
					uint16_t uid = sock_uid_[cln_sock_fd];
					switch (dir)
					{
						case kLobby:
						{
							clients_[uid]->SendLobby(data, data_len);
						}
						break;
						case kGame:
							break;
						default:
							break;
					}

				}
			}
		}
	}

}

// endregion
