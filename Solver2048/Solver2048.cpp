// Solver2048.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <array>
#include <sstream>
#include <algorithm>
#include <random>
#include <chrono>
#include <vector>
#include <unordered_map>

namespace {
	template <int A, int B>
	struct get_power
	{
		static const long long value = A * get_power<A, B - 1>::value;
	};
	template <int A>
	struct get_power<A, 0>
	{
		static const long long value = 1;
	};

	enum directrion_t {
		up = 0,
		right = 1,
		down = 2,
		left = 3,
		max_dir = 4
	};

	struct Cell {
		int x;
		int y;
		Cell() : x(-1), y(-1) {}
		Cell(int _x, int _y) : x(_x), y(_y) {}
	};

	template<int SIZE>
	struct Board {
		std::array<std::array<uint16_t, SIZE>, SIZE> mGrid;
		unsigned int mScore;

		int move(directrion_t dir) {
			int moved = 0;

			for (auto i = 0; i < (max_dir - dir) % max_dir; i++)
				rotate();

			for (auto x = 0; x < SIZE; x++) {
				for (auto y = 0; y < SIZE; y++) {
					if (mGrid[y][x] == 0) {
						// We have some empty cell so move the next tile into it.
						for (auto i = y + 1; i < SIZE; i++) {
							if (mGrid[i][x] != 0) {
								mGrid[y][x] = mGrid[i][x];
								mGrid[i][x] = 0;
								moved = 1;
								break;
							}
						}
					}

					// Check if we can merge the next non-empty tile.
					for (auto i = y + 1; i < SIZE; i++) {
						// If the next cell is zero, just continue to the next one.
						if (mGrid[i][x] == 0) {
							continue;
						}

						if (mGrid[y][x] == mGrid[i][x]) {
							mGrid[y][x] *= 2;
							mScore += mGrid[y][x];
							mGrid[i][x] = 0;
							moved = 1;
						}

						// If we merged the tile there's no need to continue (one merge per tile).
						// If we didn't that means the next tile has a different value (obstacle) so we should stop).
						break;
					}
				}
			}

			for (auto i = 0; i < dir; i++)
				rotate();

			return moved;
		}

		void rotate() {
			transpose();
			flip();
		}

		void transpose() {
			for (auto i = 0; i < SIZE; i++)
				for (auto j = 0; j < i; j++)
					std::swap(mGrid[i][j], mGrid[j][i]);
		}

		void flip() {
			for (auto row = mGrid.begin(); row < mGrid.end(); row++)
				std::reverse(row->begin(), row->end());
		}

		int getEmptyCells(Cell cells[SIZE * SIZE]) const {
			int res = 0;
			for (auto i = 0; i < SIZE; i++)
				for (auto j = 0; j < SIZE; j++) {
					if (mGrid[i][j] == 0)
						cells[res++] = { j, i };
				}
			return res;
		}

		void dump() {
			std::cout << "Score: " << mScore << "\n";
			for (auto row = mGrid.begin(); row < mGrid.end(); row++) {
				for (auto cell = row->begin(); cell < row->end(); cell++) {
					std::cout << *cell << "\t";
				}
				std::cout << "\n";
			}
		}

		bool operator==(const Board<SIZE>& other) const {
			return other.mScore == this->mScore && other.mGrid == this->mGrid;
		}

		uint16_t getMax() const {
			uint16_t max = 0;
			for (auto row : this->mGrid)
				for (auto cell : row)
					max = std::max(max, cell);
			return max;
		}
	};
}

namespace std {
	template<>
	struct hash<std::pair<Board<4>, int>> {
		size_t operator()(const std::pair<Board<4>, int>& board) const {
			size_t res = 0;
			for (auto i = 0; i < 4; i++)
				for (auto j = 0; j < 4; j++)
					res ^= std::hash<unsigned short>()(board.first.mGrid[j][i] + i + j);
			return std::hash<int>()(board.first.mScore) ^ std::hash<int>()(board.second) ^ res;
		}
	};
}

namespace {
	template<int SIZE>
	class GameManager {
	private:
		Board<SIZE> mBoard;
		std::mt19937 mGen;

	public:
		GameManager() : mBoard{ 0 } {
			unsigned seed = (unsigned)std::chrono::system_clock::now().time_since_epoch().count();
			mGen.seed(seed);

			putRandomTile();
			putRandomTile();
		}

		void putRandomTile() {
			// Gather all empty cells.
			Cell emptyCells[SIZE * SIZE];
			int emptyCellsSize = mBoard.getEmptyCells(emptyCells);
			auto size = emptyCellsSize;
			if (size == 0)
				return;

			// Get a random cell.
			std::uniform_int_distribution<int> distCell(0, size - 1);
			auto cell = emptyCells[distCell(mGen)];

			// Put random tiles (4 if 0, else 2)
			std::uniform_int_distribution<int> dist4(0, 9);
			bool put4 = dist4(mGen) == 0;
			if (mBoard.mGrid[cell.y][cell.x]) exit(-1);
			mBoard.mGrid[cell.y][cell.x] = put4 ? 2 : 4;
		}

		bool movesAvailable() {
			for(auto i = 0; i < SIZE; i++)
				for (auto j = 0; j < SIZE; j++) {
					if (mBoard.mGrid[i][j] == 0)
						return true;
					if (i > 0 && mBoard.mGrid[i][j] == mBoard.mGrid[i - 1][j])
						return true;
					if (j > 0 && mBoard.mGrid[i][j] == mBoard.mGrid[i][j - 1])
						return true;
				}
			return false;
		}

		int move(directrion_t dir) {
			int res = mBoard.move(dir);
			if(res)
				putRandomTile();
			return res;
		}

		void dump() {
			mBoard.dump();
		}

		auto getBoard() {
			return mBoard;
		}
	};

	template<int SIZE>
	double calcBoardScoreInternal(const Board<SIZE>& board) {
		double res = 0;
		// res += static_cast<double>(board.mScore);
		res += static_cast<double>(board.getMax());

		if (board.getMax() == board.mGrid[0][0])
			res += 1024;
		
		res += board.mGrid[0][0] * 40;
		res += board.mGrid[0][1] * 29;
		res += board.mGrid[0][2] * 27;
		res += board.mGrid[0][3] * 25;
		/*
		res += board.mGrid[1][0] * 29;
		res += board.mGrid[1][1] * 18;
		res += board.mGrid[1][2] * 16;
		res += board.mGrid[1][3] * 14;

		res += board.mGrid[2][0] * get_power<2, 7>::value;
		res += board.mGrid[2][1] * get_power<2, 5>::value;
		res += board.mGrid[2][2] * get_power<2, 3>::value;
		res += board.mGrid[2][3] * get_power<2, 1>::value;
		*/

		double smooth = 0;
		double empty = 0;
		for (auto y = 0; y < SIZE; y++) {
			for (auto x = 0; x < SIZE; x++) {
				if (y > 0 && board.mGrid[y][x] == board.mGrid[y - 1][x]) smooth++;
				if (x > 0 && board.mGrid[y][x] == board.mGrid[y][x - 1]) smooth++;
				if (board.mGrid[y][x] == 0) empty++;
			}
		}
		res += smooth * 8 + empty * 8;

		return res;
	}

	template<int SIZE>
	double calcBoardScore(const Board<SIZE>& board) {
		double res = 0;
		auto boardCopy = board;
		for (auto i = 0; i < 4; i++) {
			boardCopy.transpose();
			res = std::max(res, calcBoardScoreInternal<SIZE>(boardCopy));
			boardCopy.flip();
			res = std::max(res, calcBoardScoreInternal<SIZE>(boardCopy));
		}
		return res;
	}

	template<int SIZE>
	double calcScore(const Board<SIZE>& board, int depth, int& bestMoveOut, int& stats,
		std::unordered_map<std::pair<Board<SIZE>, int>, std::pair<double, int>>& cache, int numOfFour = 0) {
		if (depth == 0) {
			stats++;
			return calcBoardScore<SIZE>(board);
		}

		auto p = std::make_pair(board, depth);
		if (cache.count(p) == 1) {
			auto cached = cache[p];
			bestMoveOut = cached.second;
			return cached.first;
		}

		double bestScore = 0;
		int bestMove = -1;

		for (int dir = 0; dir < max_dir; dir++) {
			Board<SIZE> boardCopy = board;
			if (boardCopy.move((directrion_t)dir) == 0)
				continue;

			Cell emptyCells[SIZE * SIZE];
			int emptyCellsSize = boardCopy.getEmptyCells(emptyCells);
			double currScore = 0;

			for (auto cellIndex = 0; cellIndex < emptyCellsSize; cellIndex++) {
				boardCopy.mGrid[emptyCells[cellIndex].y][emptyCells[cellIndex].x] = 2;
				currScore += calcScore(boardCopy, depth - 1, bestMoveOut, stats, cache, numOfFour) / emptyCellsSize * 0.9;
				if (numOfFour < 2) {
					boardCopy.mGrid[emptyCells[cellIndex].y][emptyCells[cellIndex].x] = 4;
					currScore += calcScore(boardCopy, depth - 1, bestMoveOut, stats, cache, numOfFour + 1) / emptyCellsSize * 0.1;
				}
				boardCopy.mGrid[emptyCells[cellIndex].y][emptyCells[cellIndex].x] = 0;
			}

			if (currScore > bestScore) {
				bestScore = currScore;
				bestMove = dir;
			}
		}

		if (bestMove == -1) {
			std::mt19937 gen((unsigned)std::chrono::system_clock::now().time_since_epoch().count());
			std::uniform_int_distribution<int> dist(0, 3);
			bestMove = dist(gen);
		}
		bestMoveOut = bestMove;
		cache[p] = std::make_pair(bestScore, bestMove);
		return bestScore;
	}

	class Player {
	public:
		static const int SIZE = 4;
		GameManager<SIZE> mGM;

		int startGame() {
			while (mGM.movesAvailable()) {
				int bestMove = -1;
				auto board = mGM.getBoard();
				int stats = 0;
				int depth = 3;
				std::unordered_map<std::pair<Board<SIZE>, int>, std::pair<double, int>> cache;
				double currScore = calcScore<SIZE>(board, depth, bestMove, stats, cache);
				mGM.dump();
				std::cout << "Stats: " << stats << "\n";
				std::cout << "Move: " << bestMove << "\n\n";
				
				mGM.move((directrion_t)bestMove);
			}

			return mGM.getBoard().getMax();
			return mGM.getBoard().mScore;
		}
	};
}

int main()
{
	for (auto i = 0; i < 1; i++) {
		Player player;
		std::cout << player.startGame() << "\n";
	}

	return 0;
}
