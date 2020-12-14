#include <cstdint>
#include <iostream>
#include <tuple>
#include <algorithm>
#include <unordered_map>
#include <random>
#include <chrono>
#include <numeric>
#include <array>
#include <immintrin.h>

namespace {
	template <typename T>
	auto standard_deviation(T first, T last) {
		const double n = double(std::distance(first, last));
		const double average = std::accumulate(first, last, 0.0) / n;
		return std::sqrt(std::accumulate(first, last, 0.0, [](const double a, const double b) {
			return a + b * b;
		}) / n - average * average);
	}

	class Board {
	private:
		uint64_t mGrid;

	public:
		Board() : mGrid(0) {}

		uint64_t get(unsigned x, unsigned y) const {
			const int offset = x * 4 + y * 16;
			return ((mGrid & (15ull << offset)) >> offset) & 15ull;
		}

		void set(unsigned x, unsigned y, uint64_t value) {
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
			unsigned emptySize = 0;
			std::array<std::pair<int, int>, 16> empty;

			for (unsigned y = 0; y < 4; y++)
				for (unsigned x = 0; x < 4; x++)
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

		uint64_t getEmptyCellsBits() const {
			auto grid = mGrid;
			grid = \
				((grid & 0x1111111111111111ull) >> 0) |
				((grid & 0x2222222222222222ull) >> 1) |
				((grid & 0x4444444444444444ull) >> 2) |
				((grid & 0x8888888888888888ull) >> 3);
			return grid;
		}

		auto countEmptyCells() const {
			return int(16 - _mm_popcnt_u64(getEmptyCellsBits()));
		}

		template<typename T>
		size_t getDeltasInRows(T arr) const {
			size_t size = 0;
			auto bits = getEmptyCellsBits() * 0xfull;
			auto grid = mGrid;

			for (int i = 0; i < 4; i++) {
				const auto row = uint16_t(grid);
				const auto mask = uint16_t(bits);
				auto relevant = _pext_u64(row, mask);
				while (relevant & 0xf0) { // while next tile is relevant
					arr[size++] = std::abs((1 << (relevant & 0xf)) - (1 << ((relevant >> 4) & 0xf)));
					relevant >>= 4;
				}

				grid >>= 16;
				bits >>= 16;
			}

			return size;
		}

		auto getDeltas() const {
			auto boardCopy = *this;
			std::array<unsigned, 24> arr;
			auto size = boardCopy.getDeltasInRows(arr.begin());
			boardCopy.transpose();
			size += boardCopy.getDeltasInRows(arr.begin() + size);
			return std::make_pair(arr, size);
		}

		auto getDeltasAverage() const {
			auto[arr, size] = getDeltas();
			return std::accumulate(arr.begin(), arr.begin() + size, 0.0) / size;
		}

		auto getAllFilledCells() const {
			std::array<unsigned, 16> arr;
			size_t size = 0;
			uint64_t bits = getEmptyCellsBits() * 0xfull;
			uint64_t relevant = _pext_u64(mGrid, bits);
			while (relevant) {
				arr[size++] = 1 << (relevant & 0xf);
				relevant >>= 4;
			}

			return std::make_pair(arr, size);
		}

		auto getNonMaxAverage() const {
			auto grid = mGrid;
			double res = 0;
			unsigned n = 0;
			auto max = getMax();
			while (grid) {
				auto curr = grid & 0xfull;
				res += 1 << curr;
				n++;
				grid >>= 4;
			}
			res -= 1 << max;
			n--;
			return res / n;
		}

		auto sumSquares() const {
			double res = 0;
			uint64_t grid = mGrid;
			while (grid) {
				const auto curr = 1 << (grid & 0xfull);
				res += curr * curr;
				grid >>= 4;
			}
			return std::sqrt(res) / 4;
		}

		auto calcDeltasStandardDeviation() const {
			auto[arr, size] = getDeltas();
			// auto[arr, size] = getAllFilledCells();
			auto newEnd = std::remove(arr.begin(), arr.begin() + size, 1 << getMax());
			return standard_deviation(arr.begin(), newEnd);
		}

		auto calcSmoothnessInRows() const {
			unsigned res = 0;
			auto grid = mGrid;
			auto bits = getEmptyCellsBits() * 0xfull;
			for (int i = 0; i < 4; i++) {
				auto row = uint16_t(grid);
				auto relevant = _pext_u64(row, bits);
				while (relevant) {
					auto pair = uint8_t(relevant);
					if (pair % 0x11 == 0) {
						res += 1 << (pair / 0x11);
					}
					relevant >>= 4;
				}
				grid >>= 16;
				bits >>= 16;
			}

			return res;
		}

		auto calcSmoothness() const {
			auto boardCopy = *this;
			auto res = boardCopy.calcSmoothnessInRows();
			boardCopy.transpose();
			res += boardCopy.calcSmoothnessInRows();
			return res;
		}

		auto getGrid() const {
			return mGrid;
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

	auto calcBoardScoreInternal(const Board& board, const unsigned long long max) {
		double res = 0;

		if (max == board.get(0, 0))
			res += 1024;

		auto grid = board.getGrid();
		static const unsigned weights[4][4] = {
		{ 70, 59, 57, 55 },
		// { 40, 29, 27, 25 },
		{ 25, 27, 29, 40 },
		{ 20, 9, 7, 5 },
		// { 4, 3, 2, 1 }
		{ 1, 2, 3, 4 }
		};

		for(int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++) {
				unsigned r = grid & 0xf;
				grid >>= 4;
				res += (1 << r) * weights[i][j];
			}
		
		return res;
	}

	auto calcBoardScore(const Board& board) {
		double res = 0;

		if (!board.movesAvailable())
			return res;

		auto max = board.getMax();
		res += (1 << max) * 2;

		auto boardCopy = board;
		decltype(res) resMax = 0;
		for (auto i = 0; i < 4; i++) {
			boardCopy.transpose();
			resMax = std::max(resMax, calcBoardScoreInternal(boardCopy, max));
			boardCopy.flip();
			resMax = std::max(resMax, calcBoardScoreInternal(boardCopy, max));
		}
		res += resMax;

		// res -= 4 * board.calcDeltasStandardDeviation();
		res -= board.getNonMaxAverage();
		// res -= board.getDeltasAverage();
		res += board.sumSquares();
		res += 2 * board.countEmptyCells();
		res += board.calcSmoothness();

		return res;
	}

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

		/*
		if (size > 2) {
			double prevDiff = std::get<0>(res[0]) - std::get<0>(res[1]);
			for (int i = 2; i < size; i++) {
				if (prevDiff * 50 < std::get<0>(res[i - 1]) - std::get<0>(res[i])) {
					size = i;
					break;
				}
			}
		}
		*/

		return std::make_tuple(res, std::min(size, 3));
	}

	struct HashFunc {
		size_t operator()(const std::pair<Board, int>& p) const {
			return std::hash<uint64_t>()(p.first.mGrid) ^ std::hash<int>()(p.second);
		}
	};

	typedef std::unordered_map<std::pair<Board, int>, std::pair<double, int>, HashFunc> Cache;

	auto calcScore(const Board& board, int depth, int& bestMoveOut, int& stats,
		Cache& cache, int& cacheHits)
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
			auto numOfEmptyCells = boardCopy.countEmptyCells();
			double currScore = 0;

			for (auto it = emptyCells.cbegin(); it != emptyCells.cbegin() + emptyCellsSize; it++) {
				if (numOfEmptyCells > 5) {
					boardCopy.set(it->first, it->second, 1);
					currScore += calcScore(boardCopy, depth - 1, bestMoveOut, stats, cache, cacheHits);
					boardCopy.set(it->first, it->second, 0);
				}
				else {
					boardCopy.set(it->first, it->second, 1);
					currScore += calcScore(boardCopy, depth - 1, bestMoveOut, stats, cache, cacheHits) * 0.9;
					boardCopy.set(it->first, it->second, 2);
					currScore += calcScore(boardCopy, depth - 1, bestMoveOut, stats, cache, cacheHits) * 0.1;
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
		GameManager mGM;

		int startGame() {
			int maxStats = 1 << 20;
			Cache cache(maxStats);
			while (mGM.getBoard().movesAvailable()) {
				int bestMove = -1;
				auto board = mGM.getBoard();
				int stats = 0, cacheHits = 0;
				int depth = 5;

				auto[moves, movesSize] = getAllPossibleMoves(mGM.getBoard());
				if (movesSize == 1) {
					std::cout << "Only one move available!\n";
					bestMove = std::get<2>(moves[0]);
				}
				else {
					if (cache.bucket_count() < maxStats)
						cache.reserve(maxStats);
					calcScore(board, depth, bestMove, stats, cache, cacheHits);
					cache.clear();
					maxStats = std::max(maxStats, stats);
				}

				std::cout << mGM.getBoard();
				std::cout << "Stats: " << maxStats << " (%" << (double(cacheHits) * 100 / (cacheHits + stats)) << ")\n";
				std::cout << "Move: " << bestMove << "\n";
				std::cout << "Standard deviation: " << board.calcDeltasStandardDeviation() << "\n";
				std::cout << "\n";
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

	return 0;
}