#include "sudoku.hpp"

Sudoku::Sudoku(int size) : size(size), gridSize(int(sqrt(size))), board(size, std::vector<std::pair<int, int>>(size, std::make_pair(0, 0))) {}

Sudoku::Sudoku(const Sudoku &other) : size(other.size), gridSize(other.gridSize), board(other.board), conflict(other.conflict) {}

Sudoku::Sudoku(vector<int> flatVec)
{
    size = sqrt((flatVec.size() - 1) / 2);
    gridSize = sqrt(size);
    for (size_t i = 0; i < flatVec.size() - 1; i += 2 * size)
    {
        std::vector<std::pair<int, int>> innerVec;
        for (size_t j = 0; j < size * 2 && i + j < flatVec.size() - 1; j += 2)
        {
            innerVec.push_back(make_pair(flatVec[i + j], flatVec[i + j + 1]));
        }
        board.push_back(innerVec);
    }
    conflict = flatVec[flatVec.size() - 1];
}

void Sudoku::read_from_file(string filename)
{
    ifstream file(filename);
    if (!file.is_open())
    {
        cerr << "Error opening file ";
        cerr << filename << endl;
        exit(1);
    }
    string line;
    int row = 0;
    while (getline(file, line) && row < size)
    {
        istringstream iss(line);
        for (int col = 0; col < size; ++col)
        {
            iss >> board[row][col].first;
            if (board[row][col].first)
            {
                board[row][col].second = 1;
            }
        }
        ++row;
    }
}

vector<pair<int, int>>& Sudoku::operator[](int i) { return board[i]; }

int Sudoku::countDuplicates()
{
    int duplicates = 0;
    for (int i = 0; i < size; ++i)
    {
        unordered_set<int> rowSet;
        for (int j = 0; j < size; ++j)
        {
            if (!rowSet.insert(board[i][j].first).second)
            {
                duplicates++;
            }
        }
    }
    for (int j = 0; j < size; ++j)
    {
        unordered_set<int> colSet;
        for (int i = 0; i < size; ++i)
        {
            if (!colSet.insert(board[i][j].first).second)
            {
                duplicates++;
            }
        }
    }
    return duplicates;
}

pair<int, int> Sudoku::gridStart(int gridId)
{
    int gridRow = gridId / gridSize;
    int gridCol = gridId % gridSize;
    return make_pair(gridRow * gridSize, gridCol * gridSize);
}

void Sudoku::fillRandom()
{
    for (int gridId = 0; gridId < size; gridId++)
    {
        std::set<int> remainingNumbers;
        for (int i = 1; i <= size; ++i)
        {
            remainingNumbers.insert(i);
        }
        pair<int, int> gridStart = Sudoku::gridStart(gridId);
        int startRow = gridStart.first;
        int startCol = gridStart.second;
        for (int i = startRow; i < startRow + gridSize; i++)
        {
            for (int j = startCol; j < startCol + gridSize; j++)
            {
                if (board[i][j].first)
                {
                    remainingNumbers.erase(board[i][j].first);
                }
            }
        }
        std::vector<int> remainVec(remainingNumbers.begin(), remainingNumbers.end());
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(remainVec.begin(), remainVec.end(), gen);
        int cnt = 0;
        for (int i = startRow; i < startRow + gridSize; i++)
        {
            for (int j = startCol; j < startCol + gridSize; j++)
            {
                if (!board[i][j].first)
                {
                    board[i][j].first = remainVec[cnt];
                    cnt += 1;
                }
            }
        }
    }
}

// 扁平化board
vector<int> Sudoku::as_vector()
{
    std::vector<int> flatVec;
    for (const auto &vec1d : board)
    {
        for (const auto &p : vec1d)
        {
            flatVec.push_back(p.first);
            flatVec.push_back(p.second);
        }
    }
    flatVec.push_back(conflict);
    return flatVec;
}

int Sudoku::fitness()
{
    int totalConflicts = 0;

    // row conflicts
    for (int i = 0; i < size; ++i)
    {
        vector<int> count(size + 1, 0);
        for (int j = 0; j < size; ++j)
        {
            count[board[i][j].first] = 1;
        }
        for (int k = 1; k <= size; ++k)
        {
            if (count[k] == 0)
            {
                totalConflicts += 1;
            }
        }
    }
    // column conflicts
    for (int j = 0; j < size; ++j)
    {
        vector<int> count(size + 1, 0);
        for (int i = 0; i < size; ++i)
        {
            if (board[i][j].first > size || board[i][j].first < 0)
            {
                cerr << board[i][j].first << "!!!!!!!!!!!!!!";
                exit(1);
            }
            count[board[i][j].first] = 1;
        }
        for (int k = 1; k <= size; ++k)
        {
            if (count[k] == 0)
            {
                totalConflicts += 1;
            }
        }
    }

    return totalConflicts;
}

void Sudoku::printSudoku()
{
    for (int i = 0; i < size; ++i)
    {
        for (int j = 0; j < size; ++j)
        {
            cout << board[i][j].first << " ";
        }
        cout << endl;
    }
}

void Sudoku::setConflict(int _conflict)
{
    conflict = _conflict;
}

int Sudoku::getConflict()
{
    return conflict;
}