#include "solver.hpp"

Solver::Solver(int argc, char **argv, int size, string filename) : start_sudoku(size), size(size), box_size((int)sqrt(size))
{
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    if (mpi_rank == 0)
    {
        std::cout << "Sudoku MPI started with " << mpi_size << " processes" << std::endl
                  << std::endl;
        cout << filename << endl;
    }
    start_sudoku.read_from_file(filename);
}

bool Solver::compareSudoku(Sudoku &s1, Sudoku &s2)
{
    return s1.getConflict() > s2.getConflict();
}

void Solver::run()
{
    // 初始时间记录
    double startTime = MPI_Wtime();

    // 生成初始随机
    generate_init();
    MPI_Barrier(MPI_COMM_WORLD);
    if (mpi_rank == 0) {
        cout << "初始数量：" << population.size() << endl;
    }
    // if (mpi_rank == 0)
    // {
    //     unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    //     std::default_random_engine engine(seed);
    //     shuffle(population.begin(), population.end(), engine);
    //     for (int i = 0; i < 20; i++)
    //     {
    //         population[i].printSudoku();
    //         cout << population[i].getConflict() << endl;
    //     }
    // }
    //start_inherit();
    // 循环迭代
    for (int i = 0; i < max_generation; i++)
    {
        start_inherit();
        MPI_Barrier(MPI_COMM_WORLD);
        //  主进程进行筛选
        if (mpi_rank == 0)
        {
            cout << "目前种群数量：" << population.size() << endl;
            // 先排序，此处可以优化为MPI并行，懒了
            vector<Sudoku> sudokuVector(population.begin(), population.end());
            // for (int i = 0; i < sudokuVector.size(); i++)
            // {
            //     cout << sudokuVector[i].getConflict() << endl;
            // }
            sort(sudokuVector.begin(), sudokuVector.end(), compareSudoku);
            cout << sudokuVector.back().getConflict() << endl;
            // 找到了！
            if (sudokuVector.back().getConflict() == 0)
            {
                sudokuVector.back().printSudoku();
                // 结束时间
                double endTime = MPI_Wtime();
                cout << "Elapsed time: " << (endTime - startTime) << " seconds." << endl;
                cout << "Finded!!!!!!!!!!!!" << endl;
                exit(0);
            }
            // 没找到，进行筛选
            population.clear();
            int pop_size = sudokuVector.size();
            // 先选出一部分最好的
            for (int s = 0; s < pop_size * select_rate; ++s)
            {
                srand((unsigned)time(NULL));
                double randomValue = static_cast<double>(std::rand()) / RAND_MAX;
                if (randomValue < good_drop_rate) {
                    sudokuVector.pop_back();
                    continue;
                }
                population.push_back(sudokuVector.back());
                sudokuVector.pop_back();
            }
            // 再随机选一部分
            unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
            std::default_random_engine engine(seed);
            shuffle(sudokuVector.begin(), sudokuVector.end(), engine);
            for (int s = 0; s < pop_size * random_select_rate; ++s)
            {
                population.push_back(sudokuVector.back());
                sudokuVector.pop_back();
            }
            seed = std::chrono::system_clock::now().time_since_epoch().count();
            engine = std::default_random_engine(seed);
            shuffle(population.begin(), population.end(), engine);
        }
    }
    if (mpi_rank == 0) {
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::default_random_engine engine(seed);
        shuffle(population.begin(), population.end(), engine);
        for (int i = 0; i < 20; i++) {
            population[i].printSudoku();
            cout << population[i].getConflict()<< endl;
        }
    }
}

// 初始随机生成
void Solver::generate_init()
{
    // 每个进程需要生成的数据量
    int dataPerProcess = init_population / mpi_size + 1;

    if (mpi_rank != 0)
    {
        for (int i = 0; i < dataPerProcess; i++)
        {
            Sudoku new_gen = Sudoku(start_sudoku);
            new_gen.fillRandom();
            MPI_Send(new_gen.as_vector().data(), dataSize, MPI_INT, 0, DATA_TAG, MPI_COMM_WORLD);
        }
    }

    if (mpi_rank == 0)
    {
        for (int i = 0; i < dataPerProcess; i++)
        {
            for (int source = 1; source < mpi_size; source++)
            {
                std::vector<int> receivedData(dataSize);
                MPI_Recv(receivedData.data(), dataSize, MPI_INT, source, DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                population.push_back(Sudoku(receivedData));
                population.back().setConflict(population.back().fitness());
                // if (population.back().getConflict() == 0) {
                //     cout << "answer" << endl;
                // }
                //cout << population.back().getConflict()<< endl;
                //Sudoku(receivedData).printSudoku();
            }
        }
    }
}

void Solver::start_inherit()
{
    // 主进程
    if (mpi_rank == 0)
    {

        // 初始化非阻塞接收
        vector<MPI_Request> send_requests;
        vector<MPI_Request> recv_requests(mpi_size - 1);
        vector<int> flags(mpi_size - 1, 0);
        vector<vector<int>> recv_buffers(mpi_size - 1, vector<int>(dataSize));
        // for (int i = 1; i < mpi_size; i++)
        // {
        //     MPI_Irecv(&(recv_buffers[i - 1][0]), dataSize, MPI_INT, i, DATA_TAG, MPI_COMM_WORLD, &recv_requests[i - 1]);
        // }

        while (true)
        {
            // 检查population并发送停止信号
            //cout << "目前种群数量："<<population.size() << endl;
            if (population.size() > population_size)
            {
                for (int i = 1; i < mpi_size; i++)
                {
                    MPI_Send(NULL, 0, MPI_INT, i, STOP_TAG, MPI_COMM_WORLD);
                }
                //cout << "超了" << endl;
                break;
            }

            // 检查接收数据
            // for (int i = 0; i < recv_requests.size(); i++)
            // {
            //     MPI_Test(&recv_requests[i], &(flags[i]), MPI_STATUS_IGNORE);
            //     if (flags[i])
            //     {
            //         // 接收数据到population
            //         // Sudoku(recv_buffers[i]).printSudoku();
            //         // for (int j = 0; j < dataSize; j++) {
            //         //     if (recv_buffers[i][j] > size) {
            //         //         cerr << recv_buffers[i][j] << "????" << i+1 << endl;
            //         //         Sudoku(recv_buffers[i]).printSudoku();
            //         //         exit(1);
            //         //     }
            //         // }
            //         population.push_back(Sudoku(recv_buffers[i]));
            //         // 重新启动接收
            //         MPI_Irecv(&(recv_buffers[i - 1][0]), dataSize, MPI_INT, i + 1, DATA_TAG, MPI_COMM_WORLD, &recv_requests[i]);
            //         flags[i] = 0;
            //     }
            // }

            if (population.size() >= 2)
            {
                // 发送两个父母给子进程（非阻塞）
                for (int i = 1; i < mpi_size && population.size() >= 2; i++)
                {
                    vector<int> parentA = population.front().as_vector();
                    population.pop_front();
                    vector<int> parentB = population.front().as_vector();
                    population.pop_front();
                    MPI_Request send_request1, send_request2;
                    MPI_Send(parentA.data(), dataSize, MPI_INT, i, DATA_TAG, MPI_COMM_WORLD);
                    MPI_Send(parentB.data(), dataSize, MPI_INT, i, DATA_TAG, MPI_COMM_WORLD);
                    //send_requests.push_back(send_request1);
                    //send_requests.push_back(send_request2);
                }
            }

            for (int i = 1; i < mpi_size; i++) {
                for (int j = 0; j < n_children + n_random_children + 2; j++) {
                    vector<int> data(dataSize);
                    MPI_Status status;
                    MPI_Recv(data.data(), dataSize, MPI_INT, i, DATA_TAG, MPI_COMM_WORLD, &status);
                    population.push_back(Sudoku(data));
                    population.back().setConflict(population.back().fitness());
                    // if (population.back().getConflict() == 0)
                    // {
                    //     cout << "answer" << endl;
                    // }
                    //cout << population.back().getConflict() << endl;
                }
            }
        }
        // 等待所有发送完成
        //MPI_Waitall(send_requests.size(), send_requests.data(), MPI_STATUS_IGNORE);
    }

    // 子进程
    if (mpi_rank != 0)
    {
        vector<MPI_Request> send_requests;
        while (true)
        {
            // Setup一个接收器
            MPI_Status status;
            MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            // 检查接受的是否是停止信号
            if (status.MPI_TAG == STOP_TAG)
            {
                MPI_Recv(NULL, 0, MPI_INT, 0, STOP_TAG, MPI_COMM_WORLD, &status);
                //cout << "子进程" << mpi_rank << endl;
                break;
            }
            // 接受两个元素
            else if (status.MPI_TAG == DATA_TAG)
            {
                vector<int> parentA(dataSize);
                vector<int> parentB(dataSize);
                MPI_Recv(parentA.data(), dataSize, MPI_INT, 0, DATA_TAG, MPI_COMM_WORLD, &status);
                MPI_Recv(parentB.data(), dataSize, MPI_INT, 0, DATA_TAG, MPI_COMM_WORLD, &status);
                // GIVE BIRTH
                deque<Sudoku> children = give_birth(Sudoku(parentA), Sudoku(parentB));
                // 发回给主进程
                for (int i = 0; i < children.size(); i++)
                {   //cout << "haizishuliang:" <<children.size() << endl;
                    //children[i].printSudoku();
                    //cout << mpi_rank << endl;
                    //MPI_Request send_request;
                    MPI_Send(&(children[i].as_vector()[0]), dataSize, MPI_INT, 0, DATA_TAG, MPI_COMM_WORLD);
                    //send_requests.push_back(send_request);
                }
            }
        }
        //MPI_Waitall(send_requests.size(), send_requests.data(), MPI_STATUS_IGNORE);
    }
}

deque<Sudoku> Solver::give_birth(Sudoku A, Sudoku B)
{
    std::deque<Sudoku> children;
    // 父母也要放回
    //A.setConflict(A.fitness());
    //B.setConflict(B.fitness());
    children.push_back(A);
    children.push_back(B);
    std::random_device rd;
    for (int c = 0; c < n_children; c++)
    {
        // 随机1到size-1，来自B的数量
        std::default_random_engine generator(rd());
        std::uniform_int_distribution<int> distribution(1, size - 1);
        int n_from_B = distribution(generator);
        // 随机取n_from_b个grid
        std::vector<int> numbers(size);
        for (int i = 0; i < size; ++i)
        {
            numbers[i] = i + 1;
        }
        // 创建用于随机选择的输出向量
        std::unordered_set<int> selected_grid(n_from_B);
        // 创建随机数生成器
        std::mt19937 g(rd());
        // 使用std::sample从numbers中随机选择n个元素
        std::shuffle(numbers.begin(), numbers.end(), g);
        for (int i = 0; i < n_from_B; i++) 
        {
            selected_grid.insert(numbers.back());
            numbers.pop_back();
        }

        // crossing
        Sudoku child = Sudoku(size);
        for (int i = 0; i < size; i++)
        {
            for (int j = 0; j < size; j++)
            {
                int grid_id = (i / box_size) * box_size + (j / box_size) + 1;
                if (selected_grid.count(grid_id) > 0) 
                {
                    child[i][j].first = B[i][j].first;
                    child[i][j].second = B[i][j].second;
                }
                else 
                {
                    child[i][j].first = A[i][j].first;
                    child[i][j].second = A[i][j].second;
                }
            }
        }

        // 随机突变
        // 随机选一个格子
        int mut_grid = rand() % size + 1;
        // 存放没有fixed的cell
        vector<pair<int, int>> unfixed;
        for (int i = ((mut_grid - 1) / box_size) * box_size; i < box_size; i++)
        {
            for (int j = ((mut_grid - 1) % box_size) * box_size; j < box_size; j++)
            {
                if (child[i][j].second == 0)
                {
                    unfixed.push_back(make_pair(i, j));
                }
            }
        }

        // 有一定概率突变
        srand((unsigned)time(NULL));
        double randomValue = static_cast<double>(std::rand()) / RAND_MAX;

        if (/*child.getConflict() <= 5 and randomValue < 0.9 or*/ randomValue < mutation_rate)
        {
            // 随机选两个进行突变
            if (unfixed.size() >= 2)
            {
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dis1(0, unfixed.size() - 1);
                int index1 = dis1(gen);
                // 交换第一个元素和 vector 的最后一个元素
                std::swap(unfixed[index1], unfixed.back());
                // 从剩余的元素中随机选择第二个元素
                std::uniform_int_distribution<> dis2(0, unfixed.size() - 2);
                int index2 = dis2(gen);
                int i1 = unfixed[index1].first;
                int j1 = unfixed[index1].second;
                int i2 = unfixed[index2].first;
                int j2 = unfixed[index2].second;
                int tmp = child[i1][j1].first;
                child[i1][j1].first = child[i2][j2].first;
                child[i2][j2].first = tmp;
            }
        }
        //child.setConflict(child.fitness());
        children.push_back(child);
    }
    
    for (int c = 0; c < n_random_children; c++) {
        Sudoku child = Sudoku(start_sudoku);
        child.fillRandom();
        children.push_back(child);
    }
    return children;
}