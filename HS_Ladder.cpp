// Hearthstone Ranked Play Ladder Simulator
// (also includes Arena Simulator)
// Adrian Dale 20/07/2014

// Info:
// http://hearthstone.wikia.com/wiki/Ranked_Play
// http://www.reddit.com/r/hearthstone/comments/205klj/good_news_everyone_bonus_star_table/
// http://www.mmorpg.com/gamelist.cfm/game/974/view/forums/thread/410934/Number-of-matches-to-reach-legend-rank.html
// http://www.arenamastery.com/index.php

#include <algorithm>
#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <random>

using namespace std;

default_random_engine real_generator;
uniform_real_distribution<double> real_distribution(0.0, 1.0);

int TotalGamesPlayed = 0;

class HSPlayer
{
public:
	int Wins;
	int Losses;
	int Stars;
	int WinStreak;
	double Rating;
	int LegendAtWins; // When player got there
	int LegendAtLosses;
	bool isLegend() { return Stars > 95; };
	HSPlayer() : Wins(0), Losses(0), Stars(0), WinStreak(0), Rating(0.0),
				LegendAtWins(0), LegendAtLosses(0) {};
	void UpdatePlayer(bool isWinner, const HSPlayer &opponent);
	
};

bool comparePlayerLegendAtWins(const HSPlayer &p1, const HSPlayer &p2)
{
	return p1.LegendAtWins < p2.LegendAtWins;
}

// Updates this player's stats after a match
void HSPlayer::UpdatePlayer(bool isWinner, const HSPlayer &opponent)
{
	int startingStars = Stars;

	if (isWinner)
	{
		++Wins;
		++WinStreak;
		++Stars;
		// Bonus star for win streak, but only before rank 5
		if (WinStreak > 2 && Stars <= 45 ) ++Stars; 
	}
	else
	{
		++Losses;
		WinStreak = 0;
		// Only lose stars if you are Rank 20 or better.
		// Also, won't let base legends lose stars
		if (Stars > 10 && Stars != 96)
		{
			--Stars;
		}
	}

	// Player became a legend at this turn ;-)
	if (Stars > 95 && startingStars <= 95)
	{
		LegendAtLosses = Losses;
		LegendAtWins = Wins;
	}


	// For now, not fiddling with ratings, so don't need to
	// know about opponent.
}

int StarsToRank(int stars)
{
	int ranks[] = {
		25, 25, 25, 24, 24, 23, 23, 22, 22, 21, 21,
		20, 20, 20, 19, 19, 19, 18, 18, 18, 17, 17, 17, 16, 16, 16,
		15, 15, 15, 15, 14, 14, 14, 14, 13, 13, 13, 13,12,12,12,12,11,11,11,11,
		10,10,10,10,10,9,9,9,9,9,8,8,8,8,8,7,7,7,7,7,6,6,6,6,6,5,5,5,5,5,
		4,4,4,4,4,3,3,3,3,3,2,2,2,2,2,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0
	};

	if (stars < 0)
		return 25;
	if (stars > 95)
		return 0;
	return ranks[stars];
}

vector<HSPlayer> Population;

// Create a population of numPlayers in our Population vector.
// Give them Elo ratings from a normal distribution
void SetupPopulation(int numPlayers)
{
	// Set up random rating distribution
	default_random_engine generator;
	// I fiddled with these numbers until out of 10 million players
	// I got a min of 92 and a max of 2889
	// Will cap ratings at 100 and 2900, just to be safe
	normal_distribution<double> distribution(1500.0, 270.0);
	auto ratingMaker = bind(distribution, generator);

	Population.reserve(numPlayers);
	Population.clear();
	for (int i = 0; i < numPlayers; ++i)
	{
		HSPlayer player;
		double rating = ratingMaker();
		// Clamp rating just in case
		if (rating < 100.0) rating = 100.0;
		if (rating > 2900.0) rating = 2900.0;
		player.Rating = rating;
		Population.push_back(player);
	}
}


void PlaySingleGame(int playerNo)
{
	// playerNo is the player whose turn it is to play.
	// (Will cycle through all players one by one to ensure
	// everyone plays roughly same number of games)
	HSPlayer &p1 = Population[playerNo];
	
	// Legends don't compete any more, unless challenged by a non-legend
	if (p1.isLegend())
		return;

	// Find him a suitable match
	int p2Pos = playerNo;
	int bestMatchPos = -1;
	int bestMatchDiff = 1000;
	
	// NB size()-1 so player doesn't match against himself
	for (unsigned int i = 0; i < Population.size() - 1; ++i)
	{
		++p2Pos;
		p2Pos %= Population.size();
		HSPlayer m = Population[p2Pos];
		if (abs(p1.Stars - m.Stars) < bestMatchDiff)
		{
			bestMatchDiff = abs(p1.Stars - m.Stars);
			bestMatchPos = p2Pos;
		}
		// Trying to speed things up a bit!
		if (bestMatchDiff == 0)
			break;
	}

	if (bestMatchDiff == 1000)
	{
		cout << "No match found!" << endl;
		return;
	}

	if (bestMatchDiff > 3)
	{
		//cout << "Can't find close match" << endl;
		// This doesn't seem to change things much
		return;
	}

	HSPlayer &p2 = Population[bestMatchPos];

	// Work out expected outcome
	double expP1 = 1.0 / (1.0 + pow(10, ((p2.Rating - p1.Rating) / 400.0)));

	// Fudge factor - Hearthstone involves a lot of luck.
	// As an experiment I'm going to cap probabilities so players have a minimum
	// guaranteed percent chance to win.
	const double fudgeFactor = 0.05; // Of course I have no idea what this number should be!
	if (expP1 < fudgeFactor) expP1 = fudgeFactor;
	if (expP1 > 1.0 - fudgeFactor) expP1 = 1.0 - fudgeFactor;

	// Work out actual outcome for this trial
	double matchResult = real_distribution(real_generator);
	bool p1Wins = matchResult < expP1;
	
	// Update player stats
	p1.UpdatePlayer(p1Wins, p2);
	p2.UpdatePlayer(!p1Wins, p1);

}

void PlayGames(int numGames)
{
	int playerNo = 0;
	for (int i = 0; i < numGames; ++i)
	{
		PlaySingleGame(playerNo);

		// Each player gets to play in turn.
		// NB Need to play a lot more games than our population
		// size, probably (pop.size x 5000) or so?
		++playerNo;
		playerNo %= Population.size();
		// Of course some players are keener than others and will
		// play more. Does this make a difference?

		++TotalGamesPlayed;
	}
	cout << "Played " << TotalGamesPlayed << " games" << endl;
}

void TestDump()
{
	double minRating = 1000000.0;
	double maxRating = 0.0;
	int LegendCount = 0;
	int bestLW = 10000000;
	int bestLWPos = -1;
	int i = 0;

	sort(Population.begin(), Population.end(), comparePlayerLegendAtWins);

	for_each(Population.begin(), Population.end(), [&](HSPlayer &p){
		minRating = min(minRating, p.Rating);
		maxRating = max(maxRating, p.Rating);
	
		//if (p.isLegend())
		//{
			if (p.LegendAtWins < bestLW)
			{
				bestLW = p.LegendAtWins;
				bestLWPos = i;
			}

			cout << "r=" << p.Rating << " w=" << p.Wins << " l=" << p.Losses
				<< " R=" << StarsToRank(p.Stars)
				<< " lw=" << p.LegendAtWins << " ll=" << p.LegendAtLosses << " tg=" << (p.LegendAtLosses + p.LegendAtWins)
				<< " wr=" << (p.LegendAtWins / (double)(p.LegendAtLosses + p.LegendAtWins))
				<< endl;
		//}

		if (p.isLegend()) ++LegendCount;

		++i;
	});
	cout << Population.size() << " players" << endl;
	cout << LegendCount << " hit legend" << endl;

	if (bestLWPos != -1)
	{

		cout << "bestLW=" << bestLW << " r=" << Population[bestLWPos].Rating
			<< " lw=" << Population[bestLWPos].LegendAtWins << " ll=" << Population[bestLWPos].LegendAtLosses
			<< endl;
	}
		cout << "minRating=" << minRating << endl;
	cout << "maxRating=" << maxRating << endl;
}

bool Init()
{
	// Allegedly Hearthstone has more than 10 million player accounts
	SetupPopulation(10000);
	// Calculations start to slow at 1000 players times 1000000 games.
	// This works out at roughly 1000 games each, which is probably not
	// an unreasonable upper limit for how many games a dedicated player
	// would play per month.
	// With this, I get 343 / 1000 making it to legend
	// With 5 million games played 965 / 1000 make it
	// I think this is odd - maybe because my pool of players is so small,
	// you run out of opponents at your level, so the matching stretches
	// out too much. Not sure what the implications of this are? High players
	// getting too many easy matches?
	// Also wonder if I do need to pick a player at random each time, rather
	// than let everyone play in turn?
	TotalGamesPlayed = 0;
	return true;
}

void ArenaReward(int Wins, int &Gold, int &Dust, int &Common, int &Rare, int &Epic,
				int &GCommon, int &GRare, int &GEpic, int Pack)
{
	Gold = 0; Dust = 0;
	Common = 0; Rare = 0; Epic = 0;
	GCommon = 0; GRare = 0; GEpic = 0;
	Pack = 1;

	switch (Wins) {
	case 0:
		Gold = 30;
		break;
	case 1:
		Gold = 40;
		break;
	case 2:
		Gold = 45;
		break;
	case 3:
		Gold = 50;
		break;
	case 4:
		Gold = 70;
		break;
	case 5:
		Gold = 105;
		break;
	case 6:
		Gold = 150;
		break;
	case 7:
		Gold = 190;
		break;
	case 8:
		Gold = 200;
		break;
	case 9:
		Gold = 250;
		break;
	case 10:
		Gold = 280;
		break;
	case 11:
		Gold = 330;
		break;
	case 12:
		Gold = 500;
		break;
	};
}

void ArenaRun(int &Wins, int &Losses, double WinRate)
{
	Losses = 0;
	Wins = 0;
	while (Wins < 12 && Losses < 3)
	{
		double matchResult = real_distribution(real_generator);
		bool p1Wins = matchResult < WinRate;
		if (p1Wins) ++Wins;
		else ++Losses;
	}
}

int PlayArena(int NumArenaRuns, int StartingGold, double WinRate)
{
	bool bust = false;
	int bustCount = 0;
	int gold = StartingGold;

	int Results[13][4];
	memset(&Results[0][0], 0, sizeof(Results));

	for (int i = 0; i < NumArenaRuns; ++i)
	{
		gold -= 150;
		if (gold < 0)
		{
			//cout << "Bust at " << i << " runs" << endl;
			bust = true;
			++bustCount;
			//break;
		}

		int Losses = 0;
		int Wins = 0;
		ArenaRun(Wins, Losses, WinRate);
		++Results[Wins][Losses];
		int goldPrize = 0;
		int otherPrize = 0;
		ArenaReward(Wins, goldPrize, otherPrize, otherPrize, otherPrize, otherPrize,
			otherPrize, otherPrize, otherPrize, otherPrize);
		gold += goldPrize;
	}

	
	// Dump out results histogram
	for (int i = 0; i < 12; ++i)
	{
		//cout << i << " wins, " << Results[i][3] << " times => "
		//	<< (Results[i][3] / (double)NumArenaRuns)<< endl;
	}
	for (int i = 2; i >= 0; --i)
	{
		//cout << "12  wins / " << i << " losses, "
		//	<< Results[12][i] << " times => " << (Results[12][i] / (double)NumArenaRuns) << endl;
	}
	//cout << "Gold=" << gold << endl;
	return gold;
}

void TestArena()
{
	for (int i = 0; i < 61; ++i)
	{
		double WinRate = 0.30 + (i*0.01);
		int RunCount = 1000000;
		int gold = PlayArena(RunCount, 150, WinRate);
		cout << "WinRate=" << WinRate << /*" gold=" << gold << */ " goldPerRun=" << (gold / (double)RunCount) << endl;
		
	}
}

int main()
{
	//Init();
	//PlayGames(50000000);
	//TestDump();
	
	TestArena();

	return 0;
}