#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <stdexcept>
#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_hash_map.h>
#include <tbb/parallel_for.h>
#include <bitset>
#include <cmath>
#include <algorithm>
#include <array>
#include <chrono>
#include <atomic>
#include <string>
#include <limits>
#include <cctype>
#include <map>

// Bit-packed state (5 robots × 8 bits = 40 bits)
using State = uint64_t;

enum class Direction : uint8_t {
    UP    = 1 << 0,
    DOWN  = 1 << 1,
    LEFT  = 1 << 2,
    RIGHT = 1 << 3
};

inline int dirToIndex(Direction dir) {
    return static_cast<int>(log2(static_cast<double>(dir)));
}

class Board {
public:
    Board(int width, int height)
        : width(width), height(height),
          walls(height, std::vector<uint8_t>(width, 0)),
          targetX(-1), targetY(-1), targetColor('\0'), targetRobot(-1) {}

    void addWall(int x, int y, Direction dir) {
        validateCoordinates(x, y);
        walls[y][x] |= static_cast<uint8_t>(dir);
        int nx = x, ny = y;
        Direction opposite_dir;
        switch (dir) {
            case Direction::UP:    ny--; opposite_dir = Direction::DOWN; break;
            case Direction::DOWN:  ny++; opposite_dir = Direction::UP;   break;
            case Direction::LEFT:  nx--; opposite_dir = Direction::RIGHT; break;
            case Direction::RIGHT: nx++; opposite_dir = Direction::LEFT;  break;
            default: return;
        }
        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
             walls[ny][nx] |= static_cast<uint8_t>(opposite_dir);
        }
    }

    void setTarget(int x, int y, char color) {
        validateCoordinates(x, y);
        if (color != 'R' && color != 'B' && color != 'G' &&
            color != 'Y' && color != 'P') {
            throw std::invalid_argument("Invalid target color");
        }
        targetX = x;
        targetY = y;
        targetColor = color;
        targetRobot = colorToIndex(color);
    }

    bool hasWall(int x, int y, Direction dir) const {
        if (x < 0 || x >= width || y < 0 || y >= height) return true;
        return walls[y][x] & static_cast<uint8_t>(dir);
    }

    std::pair<int, int> getTargetPosition() const {
        return {targetX, targetY};
    }

    char getTargetColor() const {
        return targetColor;
    }

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    // --- Integrated print method ---
    void print(const std::array<std::pair<int, int>, 5>& current_robots) const {
        // Map robot index to color character for printing
        const std::map<int, char> robotIndexToColor = {
            {0, 'R'}, {1, 'B'}, {2, 'G'}, {3, 'Y'}, {4, 'P'}
        };

        for (int y = 0; y < height; ++y) {
            // Print top walls
            for (int x = 0; x < width; ++x) {
                std::cout << "+";
                if (hasWall(x, y, Direction::UP)) {
                    std::cout << "---";
                } else {
                    std::cout << "   ";
                }
            }
            std::cout << "+" << std::endl;

            // Print cell content and side walls
            for (int x = 0; x < width; ++x) {
                // Left wall
                if (hasWall(x, y, Direction::LEFT)) {
                    std::cout << "|";
                } else {
                    std::cout << " ";
                }

                // Check if there's a robot here
                char robotChar = ' ';
                int robotIdx = -1;
                for(int i = 0; i < 5; ++i) {
                    if (current_robots[i].first == x && current_robots[i].second == y) {
                        robotIdx = i;
                        // Safely access map, handle potential missing key (though unlikely here)
                        try {
                           robotChar = robotIndexToColor.at(i);
                        } catch (const std::out_of_range& oor) {
                           robotChar = '?'; // Should not happen with indices 0-4
                        }
                        break;
                    }
                }

                // Check if this is the target cell
                bool isTarget = (x == targetX && y == targetY);

                // Build color sequence
                std::string colorSequence;

                // Add background color for target
                if (isTarget) {
                    colorSequence += std::to_string(getBackgroundColorCode(targetColor)) + ";";
                }

                // Add foreground color for robot
                if (robotChar != ' ') {
                    // Use bright foreground colors (add 60)
                    colorSequence += std::to_string(getForegroundColorCode(robotChar) + 60) + ";";
                } else if (isTarget) {
                     // Dim color for the target square itself if no robot is on it
                     colorSequence += std::to_string(getForegroundColorCode(targetColor)) + ";";
                }


                // Apply colors
                if (!colorSequence.empty()) {
                    colorSequence.pop_back(); // Remove trailing ;
                    // Use bold for robots
                    std::cout << "\033[" << (robotChar != ' ' ? "1;" : "") << colorSequence << "m";
                }

                // Print content
                if (robotChar != ' ') {
                    std::cout << " " << robotChar << " ";
                } else if (isTarget) {
                    // Use a different symbol or just colored background
                    // std::cout << " \u25A0 "; // Unicode square symbol
                     std::cout << " T "; // Simple 'T' for target
                } else {
                    std::cout << "   ";
                }

                // Reset colors
                std::cout << "\033[0m";
            }

            // Print right wall for the last cell in the row
            if (hasWall(width - 1, y, Direction::RIGHT)) {
                std::cout << "|";
            } else {
                 std::cout << " "; // Match spacing if no wall
            }
            std::cout << std::endl;
        }

        // Print bottom walls for the last row
        for (int x = 0; x < width; ++x) {
            std::cout << "+";
            if (hasWall(x, height - 1, Direction::DOWN)) {
                std::cout << "---";
            } else {
                std::cout << "   ";
            }
        }
        std::cout << "+" << std::endl;
    }

friend class Solver;

private:
    void validateCoordinates(int x, int y) const {
        if (x < 0 || x >= width || y < 0 || y >= height) {
            throw std::out_of_range("Coordinates out of bounds");
        }
    }

    int colorToIndex(char color) const {
        switch (color) {
            case 'R': return 0;
            case 'B': return 1;
            case 'G': return 2;
            case 'Y': return 3;
            case 'P': return 4;
            default: throw std::invalid_argument("Invalid color character");
        }
    }

    // --- Helper color functions for print ---
    // ANSI foreground color codes (31-37)
    int getForegroundColorCode(char color) const {
        switch (std::toupper(color)) {
            case 'R': return 31; // Red
            case 'G': return 32; // Green
            case 'Y': return 33; // Yellow
            case 'B': return 34; // Blue
            case 'P': return 35; // Magenta (for Purple)
            // Add more colors if needed
            default:  return 37; // White (default)
        }
    }

    // ANSI background color codes (41-47)
    int getBackgroundColorCode(char color) const {
         switch (std::toupper(color)) {
            case 'R': return 41; // Red
            case 'G': return 42; // Green
            case 'Y': return 43; // Yellow
            case 'B': return 44; // Blue
            case 'P': return 45; // Magenta (for Purple)
            // Add more colors if needed
            default:  return 40; // Black (default)
        }
    }

    int width;
    int height;
    std::vector<std::vector<uint8_t>> walls;
    int targetX;
    int targetY;
    char targetColor;
    int targetRobot;
};

std::array<std::pair<int, int>, 5> decode(State s) {
    std::array<std::pair<int, int>, 5> robots;
    for (int i = 0; i < 5; ++i) {
        uint8_t bits = (s >> (8*i)) & 0xFF;
        int x = bits & 0x0F;
        int y = (bits >> 4) & 0x0F;
        robots[i] = {x, y};
    }
    return robots;
}

State encode(const std::array<std::pair<int, int>, 5>& robots) {
    State s = 0;
    for (int i = 0; i < 5; ++i) {
        s |= (static_cast<State>(robots[i].first & 0x0F)) << (8*i);
        s |= (static_cast<State>(robots[i].second & 0x0F)) << (8*i + 4);
    }
    return s;
}

struct TbbStateHashCompare {
    static size_t hash(State s) {
        return std::hash<State>()(s);
    }
    static bool equal(State s1, State s2) {
        return s1 == s2;
    }
};

struct Move {
    int robot;
    Direction dir;
};

class Solver {
public:
     Solver(const Board& board, State initial)
        : board(board), initial(initial), targetRobot(board.targetRobot)
     {
        if (targetRobot < 0 || targetRobot >= 5) {
            throw std::runtime_error("Target robot not set or invalid on the board before creating Solver.");
        }
    }

    std::vector<Move> solve() {
        tbb::concurrent_queue<State> queue;
        tbb::concurrent_hash_map<State, std::pair<State, Move>, TbbStateHashCompare> visited;
        std::vector<Move> solution;
        State solution_state = 0;

        queue.push(initial);
        visited.insert({initial, {initial, {-1, Direction::UP}}});

        std::atomic<bool> solutionFound = false;

        while (!solutionFound && !queue.empty()) {
            std::vector<State> current_level;
            State s;
            while (queue.try_pop(s)) current_level.push_back(s);

            if (current_level.empty()) break;

            tbb::parallel_for(tbb::blocked_range<size_t>(0, current_level.size()),
                [&](const auto& r) {
                    for (size_t i = r.begin(); i < r.end(); ++i) {
                        if (solutionFound.load()) return;

                        const State& current = current_level[i];
                        auto robots = decode(current); // Get current robot positions

                        if (checkSolution(current)) {
                            bool expected = false;
                            if (solutionFound.compare_exchange_strong(expected, true)) {
                                solution_state = current;
                            }
                            return;
                        }

                        for (int robot_idx = 0; robot_idx < 5; ++robot_idx) { // Renamed loop variable
                            for (Direction dir : {Direction::UP, Direction::DOWN,
                                                 Direction::LEFT, Direction::RIGHT}) {

                                if (solutionFound.load()) return;

                                // Simulate the move step-by-step, checking for collisions
                                auto [start_x, start_y] = robots[robot_idx];
                                auto [nx, ny] = simulateMove(start_x, start_y, dir, robots, robot_idx);

                                // Skip if the robot didn't move
                                if (start_x == nx && start_y == ny) continue;

                                // Create the new state using the moved robot
                                State new_state = encode(robots, robot_idx, nx, ny); // Use helper encode
                                Move move{robot_idx, dir};

                                // Try inserting into the visited map
                                tbb::concurrent_hash_map<State, std::pair<State, Move>, TbbStateHashCompare>::accessor acc;
                                if (visited.insert(acc, new_state)) {
                                    acc->second = {current, move};
                                    queue.push(new_state);
                                }
                                acc.release();
                            } // End directions loop
                        } // End robots loop
                    } // End parallel_for range loop
                }); // End parallel_for

            if (solutionFound.load()) break;
        } // End while loop

        if (solutionFound) {
            solution = reconstructPath(visited, solution_state);
        }

        return solution;
    }

private:
    // Helper encode overload remains member function
    State encode(const std::array<std::pair<int, int>, 5>& current_robots,
                 int robot_to_move, int next_x, int next_y) const {
        auto temp_robots = current_robots;
        temp_robots[robot_to_move] = {next_x, next_y};
        return ::encode(temp_robots);
    }

    // --- NEW: Simulate move step-by-step with collision checking ---
    std::pair<int, int> simulateMove(int start_x, int start_y, Direction dir,
                                     const std::array<std::pair<int, int>, 5>& current_robots,
                                     int moving_robot_index) const {
        int curr_x = start_x;
        int curr_y = start_y;

        while (true) {
            // 1. Check for wall at current position blocking movement in 'dir'
            if (board.hasWall(curr_x, curr_y, dir)) {
                break; // Hit wall at source
            }

            // 2. Calculate next potential position
            int next_x = curr_x;
            int next_y = curr_y;
            Direction opposite_dir;
            switch (dir) {
                case Direction::UP:    next_y--; opposite_dir = Direction::DOWN; break;
                case Direction::DOWN:  next_y++; opposite_dir = Direction::UP;   break;
                case Direction::LEFT:  next_x--; opposite_dir = Direction::RIGHT; break;
                case Direction::RIGHT: next_x++; opposite_dir = Direction::LEFT;  break;
                default: return {curr_x, curr_y}; // Should not happen
            }

            // 3. Check board boundaries for the next step
            if (next_x < 0 || next_x >= board.getWidth() || next_y < 0 || next_y >= board.getHeight()) {
                break; // Hit board boundary
            }

            // 4. Check for wall at the destination square blocking entry
            if (board.hasWall(next_x, next_y, opposite_dir)) {
                break; // Hit wall at destination
            }

            // 5. Check for collision with ANY OTHER robot at the next square
            bool collision = false;
            for (int i = 0; i < 5; ++i) {
                if (i == moving_robot_index) continue; // Don't check self
                if (current_robots[i].first == next_x && current_robots[i].second == next_y) {
                    collision = true;
                    break;
                }
            }
            if (collision) {
                break; // Stop before hitting the other robot
            }

            // If no obstacles, update current position and continue slide
            curr_x = next_x;
            curr_y = next_y;
        }
        return {curr_x, curr_y}; // Return the final resting position
    }

    bool checkSolution(State s) const {
        auto pos = decode(s)[targetRobot];
        return pos.first == board.targetX && pos.second == board.targetY;
    }

    std::vector<Move> reconstructPath(
        const tbb::concurrent_hash_map<State, std::pair<State, Move>, TbbStateHashCompare>& visited,
        State endState) const {

        std::vector<Move> path;
        State current = endState;
        while (true) {
            tbb::concurrent_hash_map<State, std::pair<State, Move>, TbbStateHashCompare>::const_accessor acc;
            if (!visited.find(acc, current)) {
                 std::cerr << "Error: State not found during path reconstruction!" << std::endl;
                 break;
            }
            const auto& [prev_state, move] = acc->second;
            acc.release();

            if (move.robot == -1) {
                break;
            }
            path.push_back(move);
            if (current == prev_state) {
                std::cerr << "Error: Path reconstruction loop detected!" << std::endl;
                break;
            }
            current = prev_state;
        }
        std::reverse(path.begin(), path.end());
        return path;
    }

    const Board& board;
    State initial;
    int targetRobot;
};

const std::unordered_map<int, std::vector<Direction>> wallMapping = {
    {0,  {}},
    {1,  {Direction::UP}},
    {2,  {Direction::RIGHT}},
    {3,  {Direction::UP, Direction::RIGHT}},
    {4,  {Direction::DOWN}},
    {5,  {Direction::UP, Direction::DOWN}},
    {6,  {Direction::RIGHT, Direction::DOWN}},
    {7,  {Direction::UP, Direction::RIGHT, Direction::DOWN}},
    {8,  {Direction::LEFT}},
    {9,  {Direction::UP, Direction::LEFT}},
    {10, {Direction::RIGHT, Direction::LEFT}},
    {11, {Direction::UP, Direction::RIGHT, Direction::LEFT}},
    {12, {Direction::DOWN, Direction::LEFT}},
    {13, {Direction::UP, Direction::DOWN, Direction::LEFT}},
    {14, {Direction::RIGHT, Direction::DOWN, Direction::LEFT}},
    {15, {Direction::UP, Direction::RIGHT, Direction::DOWN, Direction::LEFT}}
};

void loadFromFile(Board& board, const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    std::string line;
    int y = 0;
    int boardHeight = board.getHeight();
    int boardWidth = board.getWidth();

    while (std::getline(file, line) && y < boardHeight) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        int x = 0;
        int value;

        while (iss >> value && x < boardWidth) {
            auto it = wallMapping.find(value);
            if (it == wallMapping.end()) {
                 if (value >= 0 && value <= 15) {
                     if (value & static_cast<int>(Direction::UP))    board.addWall(x, y, Direction::UP);
                     if (value & static_cast<int>(Direction::DOWN))  board.addWall(x, y, Direction::DOWN);
                     if (value & static_cast<int>(Direction::LEFT))  board.addWall(x, y, Direction::LEFT);
                     if (value & static_cast<int>(Direction::RIGHT)) board.addWall(x, y, Direction::RIGHT);
                 } else {
                    throw std::runtime_error("Invalid wall value '" + std::to_string(value) +
                                             "' at (" + std::to_string(x) + ", " +
                                             std::to_string(y) + ")");
                 }
            } else {
                for (Direction dir : it->second) {
                    board.addWall(x, y, dir);
                }
            }
            x++;
        }

        if (x != boardWidth) {
            throw std::runtime_error("Incomplete line at row " + std::to_string(y) +
                                     ". Expected " + std::to_string(boardWidth) +
                                     " values, found " + std::to_string(x));
        }
        y++;
    }

    if (y != boardHeight) {
        throw std::runtime_error("File has incomplete grid. Expected " +
                                 std::to_string(boardHeight) + " rows, found " +
                                 std::to_string(y));
    }
}

int main() {
    const int BOARD_SIZE = 16;
    Board board(BOARD_SIZE, BOARD_SIZE);

    try {
        // --- LOAD BOARD ---
        loadFromFile(board, "boardstate.txt"); // Use the correct board state file name
        std::cout << "Board loaded from boardstate.txt" << std::endl;

        // --- GET TARGET FROM USER INPUT ---
        int target_x, target_y;
        char target_color_char;
        std::string target_color_str;
        bool valid_color = false;

        while (!valid_color) {
            std::cout << "Enter target robot color (R, B, G, Y, P): ";
            std::cin >> target_color_str;
            if (target_color_str.length() == 1) {
                target_color_char = std::toupper(target_color_str[0]);
                if (target_color_char == 'R' || target_color_char == 'B' || target_color_char == 'G' ||
                    target_color_char == 'Y' || target_color_char == 'P') {
                    valid_color = true;
                } else {
                    std::cerr << "Invalid color. Please enter R, B, G, Y, or P." << std::endl;
                }
            } else {
                std::cerr << "Invalid input. Please enter a single character." << std::endl;
            }
        }

        while (true) {
            std::cout << "Enter target X coordinate (0-" << BOARD_SIZE - 1 << "): ";
            if (!(std::cin >> target_x) || target_x < 0 || target_x >= BOARD_SIZE) {
                std::cerr << "Invalid X coordinate. Please enter a number between 0 and " << BOARD_SIZE - 1 << "." << std::endl;
                std::cin.clear(); // Clear error flags
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Discard bad input
            } else {
                break; // Valid input
            }
        }

        while (true) {
            std::cout << "Enter target Y coordinate (0-" << BOARD_SIZE - 1 << "): ";
            if (!(std::cin >> target_y) || target_y < 0 || target_y >= BOARD_SIZE) {
                std::cerr << "Invalid Y coordinate. Please enter a number between 0 and " << BOARD_SIZE - 1 << "." << std::endl;
                std::cin.clear(); // Clear error flags
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Discard bad input
            } else {
                break; // Valid input
            }
        }

        // Set the target using user input
        board.setTarget(target_x, target_y, target_color_char);
        std::cout << "Target set: Robot " << board.getTargetColor()
                  << " to (" << board.getTargetPosition().first << ", "
                  << board.getTargetPosition().second << ")" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error initializing board: " << e.what() << std::endl;
        return 1;
    }

    std::array<std::pair<int, int>, 5> initial_positions = {{
        {0, 1}, {15, 1}, {14, 14}, {0, 0}, {7, 8}
    }};
     std::cout << "Initial positions set." << std::endl;

    // --- PRINT INITIAL BOARD STATE --- 
    std::cout << "\nInitial Board State:" << std::endl;
    board.print(initial_positions); // Call print with initial positions
    std::cout << std::endl;

    State initial_state = encode(initial_positions);

    try {
        Solver solver(board, initial_state);
        std::cout << "Solver created. Starting search..." << std::endl;

        auto start_time = std::chrono::high_resolution_clock::now();
        std::vector<Move> solution = solver.solve();
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end_time - start_time;

        if (solution.empty()) {
            std::cout << "No solution found." << std::endl;
        } else {
            std::cout << "Solution found in " << solution.size() << " moves ("
                      << elapsed.count() << " seconds):\n";
            char robot_chars[] = {'R', 'B', 'G', 'Y', 'P'};
            for (const auto& move : solution) {
                std::string dir_str;
                switch(move.dir) {
                    case Direction::UP:    dir_str = "UP"; break;
                    case Direction::DOWN:  dir_str = "DOWN"; break;
                    case Direction::LEFT:  dir_str = "LEFT"; break;
                    case Direction::RIGHT: dir_str = "RIGHT"; break;
                    default:               dir_str = "?"; break;
                }
                if (move.robot >= 0 && move.robot < 5) {
                   std::cout << "  Robot " << robot_chars[move.robot] << " (" << move.robot << ") -> " << dir_str << "\n";
                } else {
                   std::cout << "  Invalid robot index in move: " << move.robot << "\n";
                }
            }
        }
    } catch (const std::exception& e) {
         std::cerr << "Error during solving: " << e.what() << std::endl;
         return 1;
    }

    return 0;
}
