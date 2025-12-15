# Ricochet Robot Solver

A C++ program that finds solutions to the Ricochet Robot board game. Given a board configuration, the solver computes the shortest sequence of moves to reach the target.

## Features

- Efficiently solves Ricochet Robot puzzles using search algorithms
- Supports custom board configurations
- Outputs the optimal move sequence

## Tech Stack

- **Language:** C++
- **Main files:** `board.cpp`, `solver.cpp`

## Getting Started

### Prerequisites

- C++ compiler (e.g., g++, clang++)
- Make (optional, if you use a Makefile)

### Setup

1. **Clone the repository:**
   ```bash
   git clone https://github.com/kcome26/ricochet-robot-solver.git
   cd ricochet-robot-solver
   ```

2. **Compile the program:**
   ```bash
   g++ board.cpp solver.cpp -o ricochet_solver
   ```

3. **Run the solver:**
   ```bash
   ./ricochet_solver
   ```

   (You may need to provide input files or parameters depending on your implementation.)

## Usage

- Prepare a board configuration as required by the program (see code or comments for format).
- Run the solver and follow prompts or pass the board file as an argument.
- The program will output the shortest solution or indicate if no solution exists.

## Example

```
$ ./ricochet_solver board.txt
Solution found in 12 moves:
Up, Right, Down, ...
```
