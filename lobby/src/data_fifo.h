#ifndef _DATA_FIFO_H_
#define _DATA_FIFO_H_

#include <sstream>
#include "../../common/simple_server.h"

namespace ysd_simple_server
{
	class FifoDataPipe
	{
	public:
		FifoDataPipe (unsigned short id, std::string name)
		{

			std::ostringstream ss;
			ss << _DATA_FIFO_ << name << "_" << id;
			sprintf(tg_name_, "%s", ss.str().c_str());

			if (-1 == mkfifo(tg_name_, 0666) && errno != EEXIST)
				perror("make write to gateway fifo ");

			// block
			to_gateway_fd_ = open(tg_name_, O_WRONLY);
			if (to_gateway_fd_ == -1)
				perror("open write to gateway fifo ");

			ss.str("");
			ss << _DATA_FIFO_ << name << "_" << (id * 2);
			sprintf(fg_name_, "%s", ss.str().c_str());

			if (-1 == mkfifo(fg_name_, 0666) && errno != EEXIST)
				perror("make read gateway fifo ");

			// block
			from_gateway_fd_ = open(fg_name_, O_RDONLY);
			if (from_gateway_fd_ == -1)
				perror("open read gateway fifo ");

		}

		FifoDataPipe (const FifoDataPipe&) = delete;
		FifoDataPipe& operator= (FifoDataPipe) = delete;

		~FifoDataPipe ( )
		{
			printf("release fifo ... \n");

			// TODO : detath shared memory
			close(from_gateway_fd_);
			close(to_gateway_fd_);
			unlink(fg_name_);
			unlink(tg_name_);

			printf("fifo has released.\n");
		}

		int recv_fd ( ) const { return from_gateway_fd_; }

	private:

		int from_gateway_fd_;
		int to_gateway_fd_;
		char fg_name_[64];
		char tg_name_[64];

	};
}

#endif