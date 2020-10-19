// Solver2048.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <array>
#include <sstream>
#include <algorithm>
#include <random>
#include <chrono>
#include <vector>

namespace {
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
		Cell(int _x, int _y) : x(_x), y(_y) {}
	};

	template<int SIZE>
	struct Board {
		std::array<std::array<unsigned int, SIZE>, SIZE> mGrid;
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
			for(auto i = 0; i < SIZE; i++)
				for (auto j = 0; j < i; j++)
					std::swap(mGrid[i][j], mGrid[j][i]);
		}

		void flip() {
			for (auto row = mGrid.begin(); row < mGrid.end(); row++)
				std::reverse(row->begin(), row->end());
		}

		std::vector<Cell> getEmptyCells() const {
			std::vector<Cell> res;
			for (auto i = 0; i < SIZE; i++)
				for (auto j = 0; j < SIZE; j++) {
					if (mGrid[i][j] == 0)
						res.emplace_back(j, i);
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
	};

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
			auto emptyCells = mBoard.getEmptyCells();
			auto size = emptyCells.size();
			if (size == 0)
				return;

			// Get a random cell.
			std::uniform_int_distribution<int> distCell(0, size - 1);
			auto cell = emptyCells[distCell(mGen)];

			// Put random tiles (4 if 0, else 2)
			std::uniform_int_distribution<int> dist4(0, 9);
			bool put4 = dist4(mGen) == 0;
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
	double calcBoardScore(const Board<SIZE>& board) {
		double res = static_cast<double>(board.mScore);

		unsigned int max = 0;
		for (auto row : board.mGrid)
			for(auto cell : row)
				max = std::max(max, cell);

		/*
		if (board.mGrid[0][0] == max)
			res += max * 64;
		*/

		res += board.mGrid[0][0] * std::pow(3, 4);
		res += board.mGrid[0][1] * std::pow(3, 3);
		res += board.mGrid[0][2] * std::pow(3, 2);
		res += board.mGrid[0][3] * std::pow(3, 1);
		return res;
	}

	template<int SIZE>
	double calcScore(const Board<SIZE>& board, int depth) {
		if(depth == 0)
			return calcBoardScore<SIZE>(board);

		double res = 0;
		for (int dir = 0; dir < max_dir; dir++) {
			Board<SIZE> boardCopy = board;
			if (boardCopy.move((directrion_t)dir) == 0)
				continue;

			auto emptyCells = boardCopy.getEmptyCells();

			double currRes = 0;
			for (auto cell : emptyCells) {
				boardCopy.mGrid[cell.y][cell.x] = 2;
				res += calcScore(boardCopy, depth - 1) / emptyCells.size() * 0.9;
				boardCopy.mGrid[cell.y][cell.x] = 4;
				res += calcScore(boardCopy, depth - 1) / emptyCells.size() * 0.1;
			}

			res = std::max(res, currRes);
		}

		return res;
	}

	class Player {
	public:
		static const int SIZE = 4;
		GameManager<SIZE> mGM;

		void startGame() {
			while (mGM.movesAvailable()) {
				int bestMove = -1;
				double bestScore = -1;
				int dir = 0;
				for (; dir < 4; dir++) {
					auto board = mGM.getBoard();
					if(board.move((directrion_t)dir) == 0)
						continue;
					double currScore = calcScore<SIZE>(board, 2);
					if (currScore > bestScore) {
						bestScore = currScore;
						bestMove = dir;
					}
				}

				mGM.move((directrion_t)bestMove);
			}
		}
	};
}

int main()
{
	int avg = 0;
	for (auto i = 0; i < 100; i++) {
		Player player;
		player.startGame();
		player.mGM.dump();
		avg += player.mGM.getBoard().mScore;
	}
	avg /= 100;
	std::cout << avg << "\n";

	return 0;
}
