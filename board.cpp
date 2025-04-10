#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <stdexcept>

enum class Direction : uint8_t {
    UP    = 1 << 0,
    DOWN  = 1 << 1,
    LEFT  = 1 << 2,
    RIGHT = 1 << 3
};

class Board {
public:
    Board(int width, int height) 
        : width(width), height(height), 
          walls(height, std::vector<uint8_t>(width, 0)),
          targetX(-1), targetY(-1), targetColor('\0') {}

    void addWall(int x, int y, Direction dir) {
        validateCoordinates(x, y);
        walls[y][x] |= static_cast<uint8_t>(dir);
    }

    void setTarget(int x, int y, char color) {
        validateCoordinates(x, y);
        targetX = x;
        targetY = y;
        targetColor = color;
    }

    bool hasWall(int x, int y, Direction dir) const {
        if (x < 0 || x >= width || y < 0 || y >= height) return false;
        return walls[y][x] & static_cast<uint8_t>(dir);
    }

    std::pair<int, int> getTargetPosition() const {
        return {targetX, targetY};
    }

    char getTargetColor() const {
        return targetColor;
    }

private:
    void validateCoordinates(int x, int y) const {
        if (x < 0 || x >= width || y < 0 || y >= height) {
            throw std::out_of_range("Coordinates out of bounds");
        }
    }

    int width;
    int height;
    std::vector<std::vector<uint8_t>> walls;
    int targetX;
    int targetY;
    char targetColor;
};

// Wall configuration mapping
const std::unordered_map<int, std::vector<Direction>> wallMapping = {
    {1,  {Direction::LEFT}},
    {2,  {Direction::UP}},
    {3,  {Direction::RIGHT}},
    {4,  {Direction::DOWN}},
    {5,  {Direction::LEFT, Direction::UP}},
    {6,  {Direction::UP, Direction::RIGHT}},
    {7,  {Direction::RIGHT, Direction::DOWN}},
    {8,  {Direction::DOWN, Direction::LEFT}},
    {9,  {Direction::LEFT, Direction::UP, Direction::RIGHT}},
    {10, {Direction::UP, Direction::RIGHT, Direction::DOWN}},
    {11, {Direction::RIGHT, Direction::DOWN, Direction::LEFT}}
};

void loadFromFile(Board& board, const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file");
    }

    std::string line;
    int y = 0;
    while (std::getline(file, line) && y < 16) {
        std::istringstream iss(line);
        int x = 0;
        int value;
        
        while (iss >> value && x < 16) {
            auto it = wallMapping.find(value);
            if (it == wallMapping.end()) {
                throw std::runtime_error("Invalid wall value at (" + 
                                         std::to_string(x) + ", " + 
                                         std::to_string(y) + ")");
            }
            
            for (Direction dir : it->second) {
                board.addWall(x, y, dir);
            }
            x++;
        }
        
        if (x != 16) {
            throw std::runtime_error("Incomplete line at row " + std::to_string(y));
        }
        y++;
    }
    
    if (y != 16) {
        throw std::runtime_error("File has incomplete grid");
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    try {
        Board board(16, 16);
        loadFromFile(board, argv[1]);
        
        // Example target setting (replace with actual target configuration)
        board.setTarget(12, 8, 'R');
        
        std::cout << "Board loaded successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
