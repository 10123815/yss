#include "gateway.h"

using namespace ysd_simple_server;

int InitGatewayServer( );

int main(int argc, char const *argv[])
{
	/* code */
	int listen_fd = InitGatewayServer();

	Gateway gateway(listen_fd, 100);
	gateway.Run();
	
	return 0;
}

int InitGatewayServer()
{

	// listen new user
	int listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// connect with lobby server
	// TODO

	sockaddr_in serv_addr;
	bzero(&serv_addr, sizeof(serv_addr));
	// TODO : parse config.xml
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serv_addr.sin_port = htons(1234);

	int on = 1;
	int ret = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	if (ret == -1)
	{
		perror("setsockopt error ");
		return -1;
	}

	ret = bind(listen_fd, (sockaddr*)&serv_addr, sizeof(serv_addr));
	if (ret == -1)
	{
		perror("bind error ");
		return -1;
	}

	ret = listen(listen_fd, 1024);
	if (ret == -1)
	{
		perror("listen error ");
		return -1;
	}

	return listen_fd;

}