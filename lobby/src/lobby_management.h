#ifndef _LOBBY_MANAGEMENT_H_
#define _LOBBY_MANAGEMENT_H_ value

#include <queue>
#include <tuple>

#include "../../common/simple_server.h"

namespace ysd_simple_server
{

	///////////////////////////////////////////////////////
	// This class manage all entity in the lobby,
	// create match for every three players,
	// and also work as a char room.
	///////////////////////////////////////////////////////
	class LobbyLogic
	{
		// three player's uid
		typedef uint16_t MatchResult[3];
	public:

		LobbyLogic (const LobbyLogic& other) = delete;
		LobbyLogic& operator= (LobbyLogic& other) = delete;

		// when a player want to match
		void WantPlay (uint16_t uid)
		{
			match_queue_.push(uid);
		}

		// nonblock return match result
		bool GetMatch (MatchResult match_res)
		{
			int i = 0;
			while (match_queue_.size() > 0 && i++ < 3)
			{
				match_res[i] = match_queue_.front();
				match_queue_.pop();
			}

			return i == 3;
		}

	private:
		std::queue<uint16_t> match_queue_;	// player in this queue wait for a match
	};
}

#endif