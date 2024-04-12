#include <cstdlib>
#include <string>
#include "sudoku.hpp"
#include "solver.hpp"

using namespace std;

int main(int argc, char **argv)
{
    // First argument: size of the board (size*size)
    int size = atoi(argv[1]);
    // Second argument: input file name
    string input = argv[2];
    Solver solver(argc, argv, size, input);
    solver.run();

    return 0;
}