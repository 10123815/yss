#ifndef _GAME_H_
#define _GAME_H_ value

namespace ysd_simple_server
{

    ////////////////////////////////////////////////////
	// A game server maintain game rooms for every three
	// users. It receive data from gateway server and 
	// pass them to one game room.
    ////////////////////////////////////////////////////
	class Game
	{
	public:

		Game (const Game& other) = delete;
		Game& operator= (Game other) = delete;
		
	};

}

#endif