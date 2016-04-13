#include "client.h"

using namespace ysd_simple_server;

// region client

int Client::CreateLobbyFIFO (std::string name)
{
	std::ostringstream ss;
	ss << _DATA_FIFO_ << name << "_" << uid_;
	sprintf(rl_fifo_name_, "%s", ss.str().c_str());
	if (-1 == mkfifo(rl_fifo_name_, 0666) && errno != EEXIST)
		perror("make client read fifo ");

	rl_fifo_fd_ = open(rl_fifo_name_, O_RDONLY);
	if (rl_fifo_fd_ == -1)
		perror("open client read fifo ");

	ss.str("");
	ss << _DATA_FIFO_ << name << "_" << (uid_ * 2);
	sprintf(wl_fifo_name_, "%s", ss.str().c_str());
	if (-1 == mkfifo(wl_fifo_name_, 0666) && errno != EEXIST)
		perror("make client write fifo ");

	wl_fifo_fd_ = open(wl_fifo_name_, O_WRONLY);
	if (wl_fifo_fd_ == -1)
		perror("open client write fifo ");

	return rl_fifo_fd_;
}

void Client::SendLobby (const char& data)
{
	write(wl_fifo_fd_, &data, sizeof(data));
}

void Client::RecvLobby ( )
{

}

// endregion

