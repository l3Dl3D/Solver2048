#include <cstdint>
#include <iostream>
#include <tuple>
#include <algorithm>
#include <unordered_map>
#include <random>
#include <chrono>
#include <array>
#include <immintrin.h>

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
			mGrid =\
				((mGrid & 0xf0000f0000f0000full) >> 0) |
				((mGrid & 0x0f0000f0000f0000ull) >> 12) |
				((mGrid & 0x00f0000f00000000ull) >> 24) |
				((mGrid & 0x000f000000000000ull) >> 36) |
				((mGrid & 0x0000f0000f0000f0ull) << 12) |
				((mGrid & 0x00000000f0000f00ull) << 24) |
				((mGrid & 0x000000000000f000ull) << 36);
		}

		void flip() {
			mGrid = \
				((mGrid & 0xf000f000f000f000ull) >> 12) |
				((mGrid & 0x0f000f000f000f00ull) >> 4) |
				((mGrid & 0x00f000f000f000f0ull) << 4) |
				((mGrid & 0x000f000f000f000full) << 12);
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

		/*For sorting*/
		bool operator<(const Board& other) const {
			return this->mGrid < other.mGrid;
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

		auto getRelevantCells(bool isRow) const {
			int relevantSize = 0;
			std::array<std::pair<int, int>, 16> relevant;
			auto board = *this;

			if (!isRow)
				board.transpose();

			auto[emptyCells, emptySize] = board.getEmptyCells();
			int nextY = -1, nextX = -1;
			for (auto it = emptyCells.cbegin(); it != emptyCells.cbegin() + emptySize; it++) {
				if (nextX != it->first) {
					if (isRow)
						relevant[relevantSize++] = *it;
					else
						relevant[relevantSize++] = std::make_pair(it->second, it->first);
					nextX = it->first;
				}
				nextX++;
			}

			return std::make_pair(relevant, relevantSize);
		}

		auto getMax() const {
			unsigned long long res = 0;
			auto grid = mGrid;
			for (int i = 0; i < 16; i++) {
				res = std::max(res, grid & 0xf);
				grid >>= 4;
			}
			return res;
		}

		friend struct HashFunc;

		bool movesAvailable() const {
			for (auto i = 0; i < 4; i++)
				for (auto j = 0; j < 4; j++) {
					if (get(j, i) == 0)
						return true;
					if (i > 0 && get(j, i) == get(j, i - 1))
						return true;
					if (j > 0 && get(j, i) == get(j - 1, i))
						return true;
				}
			return false;
		}

		auto countEmptyTiles() const {
			auto grid = mGrid;
			grid = \
				((grid & 0x1111111111111111ull) >> 0) |
				((grid & 0x2222222222222222ull) >> 1) |
				((grid & 0x4444444444444444ull) >> 2) |
				((grid & 0x8888888888888888ull) >> 3);
			return int(16 - _mm_popcnt_u64(grid));
		}

		auto calcVariance() const {
			double res = 0;
			double average = 0;
			std::array<double, 16> table;
			int tableSize = 0;
			for(int x = 0; x < 4; x++)
				for (int y = 0; y < 4; y++) {
					const auto temp = get(x, y);
					table[tableSize++] = temp;
					average += temp;
				}
			average /= tableSize;
			
			for (auto it = table.begin(); it < table.begin() + tableSize; it++) {
				auto temp = (*it - average);
				temp *= temp;
				res += temp;
			}
			res /= tableSize;
			return res;
		}
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

	auto calcBoardScoreInternal(const Board& board, unsigned long long max) {
		double res = 0;

		if (max == board.get(0, 0))
			res += 1024;

		res += (1 << board.get(0, 0)) * 60;
		res += (1 << board.get(1, 0)) * 49;
		res += (1 << board.get(2, 0)) * 47;
		res += (1 << board.get(3, 0)) * 45;
		res += (1 << board.get(0, 1)) * 40;
		res += (1 << board.get(1, 1)) * 29;
		res += (1 << board.get(2, 1)) * 27;
		res += (1 << board.get(3, 1)) * 25;
		res += (1 << board.get(0, 2)) * 20;
		res += (1 << board.get(1, 2)) * 9;
		res += (1 << board.get(2, 2)) * 7;
		res += (1 << board.get(3, 2)) * 5;
		res += (1 << board.get(0, 3)) * 4;
		res += (1 << board.get(1, 3)) * 3;
		res += (1 << board.get(2, 3)) * 2;
		res += (1 << board.get(3, 3)) * 1;

		return res;
	}

	auto calcBoardScore(const Board& board) {
		double res = 0;

		if (!board.movesAvailable())
			return res;

		auto max = board.getMax();
		res += 1 << max;

		auto boardCopy = board;
		decltype(res) resMax = 0;
		for (auto i = 0; i < 4; i++) {
			boardCopy.transpose();
			resMax = std::max(resMax, calcBoardScoreInternal(boardCopy, max));
			boardCopy.flip();
			resMax = std::max(resMax, calcBoardScoreInternal(boardCopy, max));
		}
		res += resMax;

		// res += board.calcVariance() * 2;

		int empty = board.countEmptyTiles();
		res += empty * 2;

		return res;
	};

	auto getAllPossibleMoves(const Board& board) {
		std::array<std::tuple<double, Board, int>, 4> res;
		int size = 0;
		for (int dir = 0; dir < 4; dir++) {
			Board currBoard = board;
			if (currBoard.move(dir) == 0)
				continue;
			auto const score = calcBoardScore(currBoard);
			auto const t = std::make_tuple(score, currBoard, dir);
			auto scoreIt = std::lower_bound(res.begin(), res.begin() + size, t, [](auto a, auto b) {
				return std::get<0>(b) < std::get<0>(a); // Put lower scores in the end.
			});
			std::copy_backward(scoreIt, res.begin() + size, res.begin() + size + 1);
			*scoreIt = t;
			size++;
		}

		if (size > 2) {
			double prevDiff = std::get<0>(res[0]) - std::get<0>(res[1]);
			for (int i = 2; i < size; i++) {
				if (prevDiff * 50 < std::get<0>(res[i - 1]) - std::get<0>(res[i])) {
					size = i;
					break;
				}
			}
		}

		return std::make_tuple(res, std::min(size, 3));
	}

	struct HashFunc {
		size_t operator()(const std::pair<Board, int>& p) const {
			return std::hash<uint64_t>()(p.first.mGrid) ^ std::hash<int>()(p.second);
		}
	};

	typedef std::unordered_map<std::pair<Board, int>, std::pair<double, int>, HashFunc> Cache;

	auto calcScore(const Board& board, int depth, int& bestMoveOut, int& stats,
		Cache& cache, int& cacheHits, int numOfFours = 0)
	{
		auto p = std::make_pair(board, depth);

		if (cache.count(p) == 1) {
			auto cached = cache[p];
			bestMoveOut = cached.second;
			cacheHits++;
			return cached.first;
		}

		double bestScore = 0;
		int bestMove = -1;

		if (depth == 0) {
			bestScore = calcBoardScore(board);
			cache.emplace(p, std::make_pair(bestScore, bestMove));
			stats++;
			return bestScore;
		}

		auto[possibleMoves, numOfPossibleMoves] = getAllPossibleMoves(board);

		for (int i = 0; i < numOfPossibleMoves; i++) {
			auto[someScore, boardCopy, dir] = possibleMoves[i];

			auto[emptyCells, emptyCellsSize] = boardCopy.getRelevantCells(i & 1);
			auto numOfEmptyCells = boardCopy.countEmptyTiles();
			double currScore = 0;

			for (auto it = emptyCells.cbegin(); it != emptyCells.cbegin() + emptyCellsSize; it++) {
				if (numOfEmptyCells > 3) {
					boardCopy.set(it->first, it->second, 1);
					currScore += calcScore(boardCopy, depth - 1, bestMoveOut, stats, cache, cacheHits, numOfFours);
					boardCopy.set(it->first, it->second, 0);
				}
				else {
					boardCopy.set(it->first, it->second, 1);
					currScore += calcScore(boardCopy, depth - 1, bestMoveOut, stats, cache, cacheHits, numOfFours) * 0.9;
					boardCopy.set(it->first, it->second, 2);
					currScore += calcScore(boardCopy, depth - 1, bestMoveOut, stats, cache, cacheHits, numOfFours) * 0.1;
					boardCopy.set(it->first, it->second, 0);
				}
			}
			currScore /= emptyCellsSize;

			if (currScore > bestScore) {
				bestScore = currScore;
				bestMove = dir;
			}
		}

		// No moves - choose random move
		if (bestMove == -1) {
			std::mt19937 gen((unsigned)std::chrono::system_clock::now().time_since_epoch().count());
			std::uniform_int_distribution<int> dist(0, 3);
			bestMove = dist(gen);
		}

		bestMoveOut = bestMove;
		cache.emplace(p, std::make_pair(bestScore, bestMove));
		stats++;
		return bestScore;
	}

	class Player {
	public:
		static const int SIZE = 4;
		GameManager mGM;

		int startGame() {
			int maxStats = 0;
			Cache cache;
			while (mGM.getBoard().movesAvailable()) {
				int bestMove = -1;
				auto board = mGM.getBoard();
				int stats = 0, cacheHits = 0;
				int depth = 6;
				if(cache.bucket_count() < maxStats)
					cache.reserve(maxStats);
				double currScore = calcScore(board, depth, bestMove, stats, cache, cacheHits);
				cache.clear();
				maxStats = std::max(maxStats, stats);
				std::cout << mGM.getBoard();
				std::cout << "Stats: " << maxStats << " (%" << (double(cacheHits) * 100 / (cacheHits + stats)) << ")\n";
				std::cout << "Variance: " << mGM.getBoard().calcVariance() << "\n";
				std::cout << "Move: " << bestMove << "\n\n";

				mGM.move(bestMove);
			}

			return 1 << mGM.getBoard().getMax();
		}
	};
}


int main() {
	/*
	Board board;
	board.set(1, 2, 5);
	board.set(1, 0, 5);

	std::cout << board << std::endl;
	auto[relevant, relevantSize] = board.getRelevantCells(1);
	for (auto it = relevant.cbegin(); it < relevant.cbegin() + relevantSize; it++)
		std::cout << it->first << " " << it->second << std::endl;
	return 0;
	*/
	for (int i = 0; i < 1; i++) {
		Player player;
		std::cout << player.startGame() << std::endl;
	}

	return 0;
}