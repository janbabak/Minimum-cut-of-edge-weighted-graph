#include <iostream>

using namespace std;

//container for test data
struct TestData {
    string filePath; //input
    int sizeOfX; //input
    long weight; //output

    TestData(string filePath, int sizeOfX, long weight)
        : filePath(filePath), sizeOfX(sizeOfX), weight(weight) {}
};