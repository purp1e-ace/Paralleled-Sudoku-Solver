#ifndef SOLVER_H
#define SOLVER_H

#include <mpi.h>
#include <deque>
#include "sudoku.hpp"

class Solver
{
public:
    Solver(int argc, char **argv, int size, string filename);
    ~Solver()
    {
        MPI_Finalize();
    }
    void run();

protected:
    // generate random solutions to begin with
    void generate_init();
    void start_inherit();

    deque<Sudoku> give_birth(Sudoku A, Sudoku B);
    static bool compareSudoku(Sudoku &s1, Sudoku &s2);

    int mpi_rank;
    int mpi_size;
    int generation = 0;
    MPI_Status mpi_status;
    static const int DATA_TAG = 0;
    static const int STOP_TAG = 1;

    int max_generation = 100;
    int population_size = 5000;
    int init_population = 500;
    int n_children = 4;
    int n_random_children = 1;
    double select_rate = 0.1;
    double random_select_rate = 0.3;
    double mutation_rate = 0.5;
    double good_drop_rate = 0.1;

    int size;
    int box_size;
    int dataSize = size * size * 2 + 1;
    Sudoku start_sudoku;
    deque<Sudoku> population;
};

#endif