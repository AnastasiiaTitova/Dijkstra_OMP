#include <omp.h>
#include "graph.h"
#include "my_exception.h"

Graph::Graph(int size, int start, int** array)
{
    this->size = size;
    this->start = start;
    this->graph = array;
    this->distances_OMP = new long[size];
    this->distances_sequential = new long[size];
    this->visited = new bool[size];
}

Graph::~Graph()
{
    for (auto i = 0; i < this->size; i++)
        delete[] this->graph[i];

    delete[] this->graph;
    delete[] this->distances_OMP;
    delete[] this->distances_sequential;
    delete[] this->visited;
}

Graph* Graph::readFromFile(std::string file)
{
    std::ifstream in;
    in.open(file);

    if (!in)
        throw MyException("Could not open file");

    int** array;
    int size;
    int start;

    in >> size;
    in >> start;

    array = new int* [size];
    for (int i = 0; i < size; i++)
    {
        array[i] = new int[size];
        for (int j = 0; j < size; j++)
            in >> array[i][j];
    }
    in.close();

    return new Graph(size, start, array);
}

void Graph::writeToFile(std::string file)
{
    std::ofstream out;
    out.open(file);

    if (!out)
        throw MyException("Can not write to this file");

    for (auto i = 0; i < this->size; i++)
    {
        if (this->distances_sequential[i] == LONG_MAX)
            out << "INF ";
        else
            out << this->distances_sequential[i] << " ";
    }

    out.close();
    return;
}

void Graph::prepare()
{
    for (auto i = 0; i < this->size; i++)
    {
        this->distances_sequential[i] = LONG_MAX;
        this->visited[i] = false;
    }
    return;
}

void Graph::prepareOMP()
{
#pragma omp parallel for schedule(dynamic)
    for (auto i = 0; i < this->size; i++)
    {
        this->distances_OMP[i] = LONG_MAX;
        this->visited[i] = false;
    }
    return;
}

bool Graph::compareDistances()
{
    for (auto i = 0; i < this->size; i++)
        if (distances_OMP[i] != distances_sequential[i])
            return false;

    return true;
}

void Graph::Dijkstra()
{
    this->prepare();
    this->distances_sequential[this->start] = 0;
    
    for (auto i = 0; i < this->size; i++) 
    {
        auto min = LONG_MAX;
        auto min_index = -1;

        for (auto j = 0; j < this->size; j++) 
        {
            if (!this->visited[j] && (this->distances_sequential[j] <= min))
            {
                min = this->distances_sequential[j];
                min_index = j;
            }
        }

        this->visited[min_index] = true;
        for (auto j = 0; j < this->size; j++)
        {
            if (this->graph[min_index][j] == 0)
                continue;
            if (this->distances_sequential[min_index] + this->graph[min_index][j] < this->distances_sequential[j])
                this->distances_sequential[j] = this->distances_sequential[min_index] + this->graph[min_index][j];
        }
    }

    return;
}

void Graph::DijkstraOMP(int num_threads)
{
    this->prepareOMP();
    this->distances_OMP[this->start] = 0;
    long* mins = new long[num_threads];
    int* min_indexes = new int[num_threads];

    for (auto i = 0; i < this->size; i++)
    {
        #pragma omp parallel for schedule(dynamic)
        for (auto i = 0; i < num_threads; i++)
        {
            min_indexes[i] = -1;
            mins[i] = LONG_MAX;
        }

        #pragma omp parallel for schedule(dynamic)
        for (auto j = 0; j < this->size; j++)
        {
            if (!this->visited[j] && (this->distances_OMP[j] <= mins[omp_get_thread_num()]))
            {
                mins[omp_get_thread_num()] = this->distances_OMP[j];
                min_indexes[omp_get_thread_num()] = j;
            }
        }
        
        auto min = LONG_MAX;
        auto min_index = -1;
        for (auto i = 0; i < num_threads; i++)
        {
            if (mins[i] <= min)
            {
                min = mins[i];
                min_index = min_indexes[i];
            }
        }

        this->visited[min_index] = true;
        #pragma omp parallel for schedule(dynamic)
        for (auto j = 0; j < this->size; j++)
        {
            if (this->graph[min_index][j] == 0)
                continue;
            if (this->distances_OMP[min_index] + this->graph[min_index][j] < this->distances_OMP[j])
                this->distances_OMP[j] = this->distances_OMP[min_index] + this->graph[min_index][j];
        }
    }

    delete[] mins;
    delete[] min_indexes;
    return;
}

void Graph::DijkstraOMPWithChunks(int num_threads, int chunk)
{
    this->prepareOMP();
    this->distances_OMP[this->start] = 0;
    long* mins = new long[num_threads];
    int* min_indexes = new int[num_threads];

    for (auto i = 0; i < this->size; i++)
    {
        #pragma omp parallel for schedule(dynamic)
        for (auto i = 0; i < num_threads; i++)
        {
            min_indexes[i] = -1;
            mins[i] = LONG_MAX;
        }

        #pragma omp parallel for schedule(dynamic, chunk)
        for (auto j = 0; j < this->size; j++)
        {
            if (!this->visited[j] && (this->distances_OMP[j] <= mins[omp_get_thread_num()]))
            {
                mins[omp_get_thread_num()] = this->distances_OMP[j];
                min_indexes[omp_get_thread_num()] = j;
            }
        }

        auto min = LONG_MAX;
        auto min_index = -1;
        for (auto i = 0; i < num_threads; i++)
        {
            if (mins[i] <= min)
            {
                min = mins[i];
                min_index = min_indexes[i];
            }
        }

        this->visited[min_index] = true;
        #pragma omp parallel for schedule(dynamic, chunk)
        for (auto j = 0; j < this->size; j++)
        {
            if (this->graph[min_index][j] == 0)
                continue;
            if (this->distances_OMP[min_index] + this->graph[min_index][j] < this->distances_OMP[j])
                this->distances_OMP[j] = this->distances_OMP[min_index] + this->graph[min_index][j];
        }
    }

    delete[] mins;
    delete[] min_indexes;
    return;
}

void Graph::DijkstraOMPCritical()
{
    this->prepareOMP();
    this->distances_OMP[this->start] = 0;

    for (auto i = 0; i < this->size; i++)
    {
        auto min = LONG_MAX;
        auto min_index = -1;
        #pragma omp parallel 
        {
            auto local_min = LONG_MAX;
            auto local_min_index = -1;

            #pragma omp for schedule(dynamic)
            for (auto j = 0; j < this->size; j++)
            {
                if (!this->visited[j] && (this->distances_OMP[j] <= local_min))
                {
                    local_min = this->distances_OMP[j];
                    local_min_index = j;
                }
            }

            #pragma omp critical
            {
                if (local_min < min) 
                {
                    min = local_min;
                    min_index = local_min_index;
                }
            };
        }

        this->visited[min_index] = true;
        #pragma omp parallel for schedule(dynamic)
        for (auto j = 0; j < this->size; j++)
        {
            if (this->graph[min_index][j] == 0)
                continue;
            if (this->distances_OMP[min_index] + this->graph[min_index][j] < this->distances_OMP[j])
                this->distances_OMP[j] = this->distances_OMP[min_index] + this->graph[min_index][j];
        }
        
    }
    return;
}

void Graph::DijkstraOMPCriticalWithChunks(int chunk)
{
    this->prepareOMP();
    this->distances_OMP[this->start] = 0;

    for (auto i = 0; i < this->size; i++)
    {
        auto min = LONG_MAX;
        auto min_index = -1;
        #pragma omp parallel 
        {
            auto local_min = LONG_MAX;
            auto local_min_index = -1;

            #pragma omp for schedule(dynamic, chunk)
            for (auto j = 0; j < this->size; j++)
            {
                if (!this->visited[j] && (this->distances_OMP[j] <= local_min))
                {
                    local_min = this->distances_OMP[j];
                    local_min_index = j;
                }
            }

            #pragma omp critical
            {
                if (local_min < min)
                {
                    min = local_min;
                    min_index = local_min_index;
                }
            };
        }

        this->visited[min_index] = true;
        #pragma omp parallel for schedule(dynamic, chunk)
        for (auto j = 0; j < this->size; j++)
        {
            if (this->graph[min_index][j] == 0)
                continue;
            if (this->distances_OMP[min_index] + this->graph[min_index][j] < this->distances_OMP[j])
                this->distances_OMP[j] = this->distances_OMP[min_index] + this->graph[min_index][j];
        }

    }
    return;
}
