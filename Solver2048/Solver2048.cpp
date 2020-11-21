#include <cstdint>
#include <iostream>
#include <tuple>
#include <algorithm>
#include <unordered_map>
#include <random>
#include <chrono>
#include <array>

namespace {
	class Board {
	private:
		uint64_t mGrid;

	public:
		Board() : mGrid(0) {}

		int get(int x, int y) const {
			const int offset = x * 4 + y * 16;
			return ((mGrid & (15ull << offset)) >> offset) & 15;
		}

		void set(int x, int y, uint64_t value) {
			const int offset = x * 4 + y * 16;
			const uint64_t mask = 15ull << offset;
			mGrid = ((~mask) & mGrid) | (value << offset);
		}

		friend std::ostream& operator<<(std::ostream& os, Board board) {
			for (auto y = 0; y < 4; y++) {
				for (auto x = 0; x < 4; x++) {
					auto curr = board.get(x, y);
					os << (curr ? (1 << curr) : 0) << "\t";
				}
				os << std::endl;
			}
			return os;
		}

		void transpose() {
			for (auto y = 0; y < 4; y++)
				for (auto x = 0; x < y; x++) {
					auto first = get(x, y);
					auto second = get(y, x);
					set(x, y, second);
					set(y, x, first);
				}
		}

		void flip() {
			for (auto y = 0; y < 4; y++)
				for (auto x = 0; x < 2; x++) {
					const auto first = get(x, y);
					const auto second = get(3 - x, y);
					set(x, y, second);
					set(3 - x, y, first);
				}
		}

		void rotate() {
			transpose();
			flip();
		}

		int move(int dir) {
			int moved = 0;

			for (auto i = 0; i < (4 - dir) % 4; i++)
				rotate();

			for (auto x = 0; x < 4; x++) {
				for (auto y = 0; y < 4; y++) {
					if (get(x, y) == 0) {
						// We have some empty cell so move the next tile into it.
						for (auto i = y + 1; i < 4; i++) {
							if (get(x, i) != 0) {
								set(x, y, get(x, i));
								set(x, i, 0);
								moved = 1;
								break;
							}
						}
					}

					// Check if we can merge the next non-empty tile.
					for (auto i = y + 1; i < 4; i++) {
						// If the next cell is zero, just continue to the next one.
						if (get(x, i) == 0) {
							continue;
						}

						if (get(x, y) == get(x, i)) {
							set(x, y, get(x, y) + 1);
							set(x, i, 0);
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

		bool operator==(const Board& other) const {
			return this->mGrid == other.mGrid;
		}

		auto getEmptyCells() const {
			char emptySize = 0;
			std::array<std::pair<int, int>, 16> empty;

			for (char y = 0; y < 4; y++)
				for (char x = 0; x < 4; x++)
					if (get(x, y) == 0) {
						empty[emptySize++] = std::make_pair(x, y);
					}

			return std::make_tuple(empty, emptySize);
		}

		auto getMax() const {
			int res = 0;
			for (int y = 0; y < 4; y++)
				for (int x = 0; x < 4; x++)
					res = std::max(res, get(x, y));
			return res;
		}

		friend struct HashFunc;
	};

	class GameManager {
	private:
		Board mBoard;
		std::mt19937 mGen;

	public:
		GameManager() {
			unsigned seed = (unsigned)std::chrono::system_clock::now().time_since_epoch().count();
			mGen.seed(seed);

			putRandomTile();
			putRandomTile();
		}

		void putRandomTile() {
			// Gather all empty cells.
			auto[emptyCells, size] = mBoard.getEmptyCells();
			if (size == 0)
				return;

			// Get a random cell.
			std::uniform_int_distribution<int> distCell(0, size - 1);
			auto cell = emptyCells[distCell(mGen)];

			// Put random tiles (4 if 0, else 2)
			std::uniform_int_distribution<int> dist4(0, 9);
			bool put4 = dist4(mGen) == 0;
			if (mBoard.get(cell.first, cell.second)) exit(-1);
			mBoard.set(cell.first, cell.second, put4 ? 2 : 1);
		}

		bool movesAvailable() const {
			for (auto i = 0; i < 4; i++)
				for (auto j = 0; j < 4; j++) {
					if (mBoard.get(j, i) == 0)
						return true;
					if (i > 0 && mBoard.get(j, i) == mBoard.get(j, i - 1))
						return true;
					if (j > 0 && mBoard.get(j, i) == mBoard.get(j - 1, i))
						return true;
				}
			return false;
		}

		int move(int dir) {
			int res = mBoard.move(dir);
			if (res)
				putRandomTile();
			return res;
		}

		auto getBoard() const {
			return mBoard;
		}
	};

	double calcBoardScoreInternal(const Board& board) {
		int res = 0;
		// res += static_cast<double>(board.mScore);
		auto max = board.getMax();
		res += 1 << max;

		if (max == board.get(0, 0))
			res += 1024;

		res += (1 << board.get(0, 0)) * 40;
		res += (1 << board.get(1, 0)) * 29;
		res += (1 << board.get(2, 0)) * 27;
		res += (1 << board.get(3, 0)) * 25;
		res += (1 << board.get(0, 1)) * 20;
		res += (1 << board.get(1, 1)) * 9;
		res += (1 << board.get(2, 1)) * 7;
		res += (1 << board.get(3, 1)) * 5;

		/*
		int smooth = 0;
		int empty = 0;
		
		for (auto y = 0; y < 4; y++) {
			for (auto x = 0; x < 4; x++) {
				auto curr = board.get(x, y);
				if (y > 0 && curr == board.get(x, y - 1)) smooth++;
				if (x > 0 && curr == board.get(x - 1, y)) smooth++;
				if (curr == 0) empty++;
			}
		}

		res += smooth * 8 + empty * 8;
		*/

		return static_cast<double>(res);
	}

	struct HashFunc {
		size_t operator()(const std::pair<Board, int>& p) const {
			return std::hash<uint64_t>()(p.first.mGrid) ^ std::hash<int>()(p.second);
		}
	};

	double calcBoardScore(const Board& board) {
		double res = 0;
		auto boardCopy = board;
		for (auto i = 0; i < 4; i++) {
			boardCopy.transpose();
			res = std::max(res, calcBoardScoreInternal(boardCopy));
			boardCopy.flip();
			res = std::max(res, calcBoardScoreInternal(boardCopy));
		}
		return res;
	};

	typedef std::unordered_map<std::pair<Board, int>, std::pair<double, int>, HashFunc> Cache;

	double calcScore(const Board& board, int depth, int& bestMoveOut, int& stats,
		Cache& cache, int numOfFour = 0) {
		auto p = std::make_pair(board, depth);

		if (cache.count(p) == 1) {
			auto cached = cache[p];
			bestMoveOut = cached.second;
			return cached.first;
		}

		double bestScore = 0;
		int bestMove = -1;

		if (depth == 0) {
			// stats++;
			bestScore = calcBoardScore(board);
			cache[p] = std::make_pair(bestScore, bestMove);
			return bestScore;
		}

		for (int dir = 0; dir < 4; dir++) {
			Board boardCopy = board;
			if (boardCopy.move(dir) == 0)
				continue;

			auto[emptyCells, emptyCellsSize] = boardCopy.getEmptyCells();
			double currScore = 0;

			for (auto cellIndex = 0; cellIndex < emptyCellsSize; cellIndex++) {
				boardCopy.set(emptyCells[cellIndex].first, emptyCells[cellIndex].second, 1);
				currScore += calcScore(boardCopy, depth - 1, bestMoveOut, stats, cache, numOfFour) / emptyCellsSize * 0.9;
				if (numOfFour < 2) {
					boardCopy.set(emptyCells[cellIndex].first, emptyCells[cellIndex].second, 2);
					currScore += calcScore(boardCopy, depth - 1, bestMoveOut, stats, cache, numOfFour + 1) / emptyCellsSize * 0.1;
				}
				boardCopy.set(emptyCells[cellIndex].first, emptyCells[cellIndex].second, 0);
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
		GameManager mGM;

		int startGame() {
			while (mGM.movesAvailable()) {
				int bestMove = -1;
				auto board = mGM.getBoard();
				int stats = 0;
				int depth = 4;
				Cache cache;
				double currScore = calcScore(board, depth, bestMove, stats, cache);
				std::cout << mGM.getBoard();
				// std::cout << "Stats: " << stats << "\n";
				std::cout << "Move: " << bestMove << "\n\n";

				mGM.move(bestMove);
			}

			return 1 << mGM.getBoard().getMax();
		}
	};
}


int main() {
	for (int i = 0; i < 1; i++) {
		Player player;
		std::cout << player.startGame() << std::endl;
	}
}