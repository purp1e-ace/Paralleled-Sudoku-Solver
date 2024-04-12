#!/bin/bash
# (See https://arc-ts.umich.edu/greatlakes/user-guide/ for command details)
# Set up batch job settings

#SBATCH --job-name=submit
#SBATCH --mail-type=BEGIN,END
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=36
#SBATCH --exclusive
#SBATCH --mem-per-cpu=4g
#SBATCH --time=00:05:00
#SBATCH --account=eecs587f23_class
#SBATCH --partition=standard

# Run your program
# (">" redirects the print output of your pfogram,
# in this case to "output.txt")

mpic++ main.cpp solver.cpp sudoku.cpp -o main
mpirun -np 16 --bind-to core:overload-allowed ./main 9 "./data/difficulty4/sudoku38.txt"

