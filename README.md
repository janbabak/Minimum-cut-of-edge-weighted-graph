# Minimum cut of an edge-weighted graph

[![Open MP](https://img.shields.io/badge/open-MP-pgreen.svg)](https://www.openmp.org)
[![Open MPI](https://img.shields.io/badge/open-MPI-pgreen.svg)](https://www.open-mpi.org)
[![C++](https://img.shields.io/badge/C%2B%2B-00599C?style=flat&logo=c%2B%2B&logoColor=white)](https://en.cppreference.com/w/c/language)

## Description

This repository contains a program that can solve the optimization problem of finding the minimum cut of an edge-weighted graph. I developed it during my studies in the Parallel and Distributed Programming course at the [Faculty of Information Technology of Czech Technical University](https://fit.cvut.cz/cs).

The problem involves taking a graph with $n$ vertices and a number $a$, and finding a cut of the vertices into two sets, $X$ and $Y$. The set $X$ contains $a$ vertices, while the set $Y$ contains $n-a$ vertices. The program then calculates the sum of edges $\{u, v\}$, where $u$ is in $X$z and $v$ is in $Y$, and finds the minimum value for this sum.

## Solution

Finding the minimal cut of an edge-weighted graph is a problem that is NP complete, which means it cannot be solved in polynomial time. The purpose of this course was to create a sequential solution using "brute force" methods, while also taking advantage of parallelization to increase speed. **Each solution is located in a separate git branch.**

This task involved four iterations. The first iteration was focused on developing a sequential solution. The second iteration involved parallelizing the sequential solution by implementing task parallelism with the [Open MP library](https://www.openmp.org). The third iteration aimed at developing a parallel solution by implementing data parallelism and utilizing the [Open MP library](https://www.openmp.org). Finally, the fourth iteration included using data or task parallelization (on multiple threads) from previous steps and adding process parallelization using the [MPI library](https://www.open-mpi.org) and master-slave architecture.

### Sequential Solution

This is the simplest and slowest solution, which is a recursive algorithm based on depth-first search. I have implemented several optimizations, such as branch and bound, which allow the program to avoid backtracking every possible solution. Here are the optimizations:

-   If the size of set $X$ is greater than $a$ or the size of set $Y$ is greater than $n-a$, the requirements are not met, and the recursion ends.
-   If some vertices are not assigned to $X$ or $Y$, but the weight of this configuration is already greater than the best minimal cut weight found, stop this recursion branch.
-   Next, I have implemented lower bound calculation. If the lower bound is greater than or equal to the best minimal weight found, it stops. The lower bound is computed as the weight of the cut of assigned vertices plus, for each unassigned vertex, the minimum of assigning it to $X$ or $Y$.

### Task parallelism

Task parallelism is a method that executes functions (tasks) in separate threads. This solution is based on the sequential. The sequential solution computes all recursive call in one thread, while this one computes recursive call in different threads. This algorithm utilizes the optimizations from sequential solution and adds a few more.

-   When there remain only the last three layers of tree of recursive calls, algorithm executes them sequentially, because it is faster than creating new threads for them.
-   Next, if the solution isn't the best, the algorithm doesn't entry the critical section.
    ```c++
    // if best, save it
    if (weight < minimalSplitWeight) {
        #pragma omp critical
        {
            if (weight < minimalSplitWeight)
                minimalSplitWeight = weight;
                for (int i = 0; i < configLength; i++) {
                    minimalSplitConfig[i] = config[i];
                }
            }
        }
    }
    ```

### Data parallelism

This approach uses a parallel for loop. Firstly, a task pool is created using DFS or BFS algorithms. The second step involves parallel iteration over the loop. Since iterations are data independent, each can be handled by a different thread. This method employs the same optimizations as the previous ones.

```c++
void consumeTaskPool() {
    int indexOfFirstUndecided = min(maxPregeneratedLevel, configLength);

    #pragma omp parallel for schedule(dynamic)
    for (auto &task: taskPool) {
        searchAux(task, indexOfFirstUndecided);
    }

    for (auto &task: taskPool) {
        delete[] task;
    }
}
```

### Master-Slave Parallelization

Master-slave parallelization is a more complex approach than the previous ones. This program can run on supercomputers that contain multiple CPUs. It is based on the data parallelism solution. There is one master process that generates the task pool and distributes it to slave processes. The slave processes create their separate task pools and execute them using data parallelism. This solution uses multiple processes, and each process uses multiple threads. The [MPI library](https://www.open-mpi.org) handles communication between processes by sending messages.

## Software requirements

-   [g++ compiler](https://gcc.gnu.org) that can compile code to c++ 14 or newer.
-   [OpenMP](https://www.openmp.org)
-   [Open MPI](https://www.open-mpi.org)
-   [mpic++](https://www.open-mpi.org/doc/v3.0/man1/mpic++.1.php)
-   [Make](https://www.gnu.org/software/make/manual/make.html)
-   or just [Docker](https://www.docker.com) (best way for apple sillicon)

## How to run

To run the program, do the following:

-   Clone the repository:
    ```bash
    git clone https://github.com/janbabak/Minimum-cut-of-edge-weighted-graph.git
    ```
-   Navigate to the cloned folder:
    ```bash
    cd Minimum-cut-of-edge-weighted-graph
    ```
-   If you don't wan to install the dependencies on your machine, you can use the provided Docker image.
-   Build the Docker image using the command below:
    ```bash
    docker build -t minimum-cut-img .
    ```
-   Run the Docker image in interactive mode using the following command:

    ```bash
    docker run -it -v `pwd`:/work --name minimum-cut minimum-cut-img
    ```

-   **Sequential solution**

    -   Checkout to the right branch using the command:
        ```bash
        git checkout sequential_solution
        ```
    -   Compile the code using the command:
        ```bash
        make
        ```
    -   Run the program. The first argument is the input file containing the graph, followed by the size of $X$. An optional argument is the solution for testing purposes.
        ```bash
        ./pdp graf_mro/graf_30_10.txt 10 4636
        ```

-   **Task parallelism**
    -   Checkout to the right branch using the command:
        ```bash
        git checkout task_parallelism
        ```
    -   Compile the code using the command:
        ```bash
        make
        ```
    -   Run the program. The first argument is input file containing the graph followed by the size of $X$ and number of threads. Than is and optional argument - solution for testing purposes.
        ```bash
        ./pdp graf_mro/graf_30_10.txt 10 5 4636
        ```
-   **Data parallelism**
    -   Checkout to the right branch using the command:
        ```bash
        git checkout data_parallelism
        ```
    -   Compile the code using the command:
        ```bash
        make
        ```
    -   Run the program. The first argument is input file containing the graph followed by the size of $X$ and number of threads. Than is and optional argument - solution for testing purposes.
        ```bash
        ./pdp graf_mro/graf_30_10.txt 10 5 4636
        ```
-   **Master slave (multi process) parallelism**

    -   Checkout to the right branch using the command:
        ```bash
        git checkout process_parallelism
        ```
    -   Compile the code using the command:
        ```bash
        make
        ```
    -   Run the program. The np means number of processes. The first argument of my program is input file containing the graph followed by the size of $X$ and number of threads. Than is and optional argument - solution for testing purposes.
        ```bash
        mpirun -np 4 ./pdp graf_mro/graf_10_5.txt 5 10 974
        ```

-   Generated files can be removed by running
    ```bash
    make clean
    ```

## Output Format

The output consists of four lines:

-   The first line contains the name of the file.
-   The second line contains the minimum weight of the cut.
-   The third line contains an array where ones represent vertices in the set X, and zeros represent the remaining vertices in Y.
-   The last line provides the time.

```
graf_mro/graf_10_5.txt
Minimal weight: 974
[0, 0, 0, 1, 1, 1, 1, 1, 0, 0]
time: 0.025s
________________________________
```
