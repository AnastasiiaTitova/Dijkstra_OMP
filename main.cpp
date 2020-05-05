#include <iostream>
#include <fstream>
#include <omp.h>
#include <string>
#include <chrono>
#include "graph.h"
#include "my_exception.h"

using namespace std;

void generate(int number)
{
	ofstream out;
	out.open("input" + to_string(number) + ".txt");
	if (!out)
		throw MyException("Can not open file to generate");

	out << number << endl << rand() % number << endl;

	int** generated = new int* [number];
	for (auto i = 0; i < number; i++)
		generated[i] = new int[number];

	cout << "Started generating matrix for size " << number << endl;
	for (auto i = 0; i < number; i++)
	{
		generated[i][i] = 0;
		for (auto j = i + 1; j < number; j++)
			generated[i][j] = generated[j][i] = rand() % 100;
	}
	cout << "Finished generating matrix" << endl;

	cout << "Started filling in file" << endl;
	for (auto i = 0; i < number; i++)
	{
		for (auto j = 0; j < number; j++)
			out << generated[i][j] << " ";
		out << endl;
		delete[] generated[i];
	}
	delete[] generated;
	cout << "Finished filling in file" << endl;
	out.close();
}

int main()
{
	try
	{
		//const int size = 20000;
		//generate(size);
		//string name = "input" + to_string(size) + ".txt";
		string name;
		cout << "Enter input file name" << endl;
		cin >> name;
		Graph* graph = Graph::readFromFile(name);
		cout << "Enter output file name" << endl;
		cin >> name;
		//name = "output" + to_string(size) + ".txt";

		const auto max_threads = omp_get_max_threads();

		ofstream out;
		//out.open("result" + to_string(size) + ".csv");
		out.open("result.csv");
		out << "Type,Duration,Threads,Launch,Chunk" << endl;

		for (auto launch = 1; launch <= 5; launch++)
		{
			cout << "Launch number " << launch << endl;
			for (auto num_threads = 1; num_threads <= max_threads; num_threads++)
			{
				cout << "Number of threads " << num_threads << endl;
				
				auto start = std::chrono::steady_clock::now();
				graph->Dijkstra();
				auto finish = std::chrono::steady_clock::now();
				auto duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start);
				cout << "No OMP duration: " << duration.count() << endl;
				out << "None," << duration.count() << ",," << launch << "," << endl;

				omp_set_num_threads(num_threads);

				start = std::chrono::steady_clock::now();
				graph->DijkstraOMP(num_threads);
				finish = std::chrono::steady_clock::now();
				duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start);
				cout << "OMP duration dynamic: " << duration.count() << " \tThreads: " << num_threads << " \tLaunch: " << launch  
					<< "\t Chunk: none" << "\t Correct: " << graph->compareDistances() << endl;
				out << "Dynamic," << duration.count() << "," << num_threads << "," << launch  << ",none"<< endl;


				int chunk = graph->size / num_threads;
				start = std::chrono::steady_clock::now();
				graph->DijkstraOMPWithChunks(num_threads, chunk);
				finish = std::chrono::steady_clock::now();
				duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start);
				cout << "OMP duration dynamic: " << duration.count() << " \tThreads: " << num_threads << " \tLaunch: " << launch
					<< "\t Chunk: " << chunk << "\t Correct: " << graph->compareDistances() << endl;
				out << "Dynamic," << duration.count() << "," << num_threads << "," << launch  << "," << chunk << endl;

				start = std::chrono::steady_clock::now();
				graph->DijkstraOMPCritical();
				finish = std::chrono::steady_clock::now();
				duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start);
				cout << "OMP duration dynamic critical: " << duration.count() << " \tThreads: " << num_threads << " \tLaunch: " << launch
					<< "\t Chunk: none" << "\t Correct: " << graph->compareDistances() << endl;
				out << "Dynamic critical," << duration.count() << "," << num_threads << "," << launch << ",none" << endl;

				start = std::chrono::steady_clock::now();
				graph->DijkstraOMPCriticalWithChunks(chunk);
				finish = std::chrono::steady_clock::now();
				duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start);
				cout << "OMP duration dynamic critical: " << duration.count() << " \tThreads: " << num_threads << " \tLaunch: " << launch
					<< "\t Chunk: " << chunk << "\t Correct: " << graph->compareDistances() << endl;
				out << "Dynamic critical," << duration.count() << "," << num_threads << "," << launch << "," << chunk << endl;
			}
		}

		out.close();
		graph->writeToFile(name);
		delete graph;
	}
	catch (exception &ex)
	{
		cout << ex.what();
		return -1;
	}
		
	return 0;
}
