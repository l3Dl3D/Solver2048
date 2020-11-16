#include <cstdint>
#include <iostream>
#include <tuple>
#include <algorithm>
#include <unordered_map>

class Board {
private:
	uint64_t mGrid;

public:
	int get(int x, int y) {
		const int offset = x * 4 + y * 16;
		return ((mGrid & (15ull << offset)) >> offset) & 15;
	}

	void set(int x, int y, uint64_t value) {
		const int offset = x * 4 + y * 16;
		const uint64_t mask = 15ull << offset;
		mGrid = ((~mask) & mGrid) | (value << offset);
	}

	friend std::ostream& operator<<(std::ostream& os, Board& board) {
		for (auto y = 0; y < 4; y++) {
			for (auto x = 0; x < 4; x++) {
				auto curr = board.get(x, y);
				os << (curr ? (1 << curr) : 0) << " ";
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
		for(auto y = 0; y < 4; y++)
			for (auto x = 0; x < 2; x++) {
				auto first = get(x, y);
				auto second = get(3 - x, y);
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

	bool operator==(const Board& other) {
		return this->mGrid == other.mGrid;
	}

	auto getEmptyCells() {
		uint16_t empty = 0, emptySize = 0;

		for(int y = 0; y < 4; y++)
			for(int x = 0; x < 4; x++)
				if (get(x, y) == 0) {
					emptySize++;
					empty |= 1 << (x + y * 4);
				}
		return std::make_tuple(empty, emptySize);
	}

	auto getMax() {
		int res = 0;
		for (int y = 0; y < 4; y++)
			for (int x = 0; x < 4; x++)
				res = std::max(res, get(x, y));
		return res;
	}

	friend class std::hash<Board>;
};

namespace std {
	template<>
	struct hash<Board> {
		size_t operator()(const Board& board) const {
			return std::hash<uint64_t>()(board.mGrid);
		}
	};
}


int main() {
	Board board;
	Board board2;
	// std::cout << board;

	board.set(1, 0, 2);
	board.set(3, 0, 3);

	board.move(3);
	std::cout << board;
	std::cout << (board == board2) << std::endl;

	auto[empty, emptyCells] = board.getEmptyCells();

	std::cout << empty << std::endl;
	std::cout << emptyCells << std::endl;
	std::cout << board.getMax() << std::endl;
}