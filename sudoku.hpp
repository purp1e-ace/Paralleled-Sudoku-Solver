#ifndef SUDOKU_H
#define SUDOKU_H

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_set>
#include <utility>
#include <cmath>
#include <vector>
#include <algorithm>
#include <random>
#include <set>
#include <chrono>

using namespace std;

class Sudoku
{
public:
    Sudoku(int size);
    Sudoku(const Sudoku &other);
    Sudoku(vector<int> flatVec);
    void read_from_file(string filename);
    vector<pair<int, int>>& operator[](int i);
    int countDuplicates();
    void fillRandom();
    pair<int, int> gridStart(int gridId);
    vector<int> as_vector();
    int fitness();
    void printSudoku();
    void setConflict(int _conflict);
    int getConflict();

protected:
    int size;
    int gridSize;
    int conflict;
    vector<vector<pair<int, int>>> board;
};

#endif