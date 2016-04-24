#ifndef _LOBBY_MANAGEMENT_H_
#define _LOBBY_MANAGEMENT_H_ value

#include <list>
#include <tuple>

#include "../../common/simple_server.h"

namespace ysd_simple_server
{

	///////////////////////////////////////////////////////
	// This class manage all entity in the lobby,
	// create match for every three players.
	// Singel thread to prevent wrong insert/remove operation.
	///////////////////////////////////////////////////////
	class LobbyLogic
	{
	public:
		LobbyLogic ( ) = default;

		LobbyLogic (const LobbyLogic& other) = delete;
		LobbyLogic& operator= (LobbyLogic& other) = delete;

		// when a player want to match
		void WantPlay (uint16_t uid)
		{
			// promise that no same user id
			match_queue_.remove(uid);
			match_queue_.push_back(uid);
		}

		// when a player cancel match
		void CancelMatch (uint16_t uid)
		{
			match_queue_.remove(uid);
		}

		// nonblock return match result
		bool GetMatch (MatchResult* match_res)
		{
			if (match_queue_.size() >= 2)
			{
				match_res->first = match_queue_.front();
				match_queue_.pop_front();
				match_res->second = match_queue_.front();
				match_queue_.pop_front();
				return true;
			}
			return false;
		}

	private:
		std::list<uint16_t> match_queue_;	// player in this queue wait for a match


	};
}

#endif