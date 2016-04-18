////////////////////////////////////////////////////////
// main.cc for lobby server
////////////////////////////////////////////////////////

#include "lobby.h"

using namespace ysd_simple_server;

// void InitLobby ( );

// void ConnectGateway ( );

int main (int argc, char const *argv[])
{
	
	Lobby lobby(100);
	lobby.Run();

	return 0;
}

