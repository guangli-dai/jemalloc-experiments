#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

#define MAX_SIZE 1000000000
void no_opt_ptr(void* ptr) {
    // Dummy function to prevent optimization
    asm volatile("" : : "g"(ptr) : "memory");
}

struct AllocationRange {
    size_t minSize;
    size_t maxSize;
    double cumulativeProbability;
};

std::vector<AllocationRange> readAllocationRanges(const char* inputFilename) {
    std::vector<AllocationRange> ranges;
    std::ifstream infile(inputFilename);
    std::string line;
    double cumulativeProbability = 0;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        double minSize, probability;
        if (!(iss >> minSize >> probability)) {
            std::cerr << "Error reading line: " << line << std::endl;
            continue;
        }
        // The minSize in this line is the maxSize of the last line
        if (ranges.size() > 0) {
            ranges[ranges.size()-1].maxSize = minSize;
        }
	cumulativeProbability += probability;
        ranges.push_back({static_cast<size_t>(minSize), 0, cumulativeProbability});
    }
    ranges[ranges.size()-1].maxSize = MAX_SIZE;
    return ranges;
}

size_t getRandomSize(const std::vector<AllocationRange>& ranges) {
    double randValue = static_cast<double>(rand()) / RAND_MAX;
    auto it = std::lower_bound(ranges.begin(), ranges.end(), randValue, 
        [](const AllocationRange& range, double value) {
            return range.cumulativeProbability < value;
        });
    if (it != ranges.end()) {
        return it->minSize + (rand() % (it->maxSize - it->minSize));
    }
    std::cout << "Should be unreachable!" << std::endl;
    return ranges.back().minSize; // Fallback in case of rounding errors
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <numOperations> <outputFilename> <inputFilename>\n", argv[0]);
        return 1;
    }

    int numOperations = atoi(argv[1]);
    const char* outputFilename = argv[2];
    const char* inputFilename = argv[3];

    FILE* file = fopen(outputFilename, "w");
    if (!file) {
        perror("Failed to open output file");
        return 1;
    }

    std::vector<AllocationRange> allocationRanges = readAllocationRanges(inputFilename);
    if (allocationRanges.empty()) {
        fprintf(stderr, "No valid allocation ranges found in input file\n");
        return 1;
    }

    std::vector<size_t> allocations;
    srand((unsigned int)time(NULL));

    for (int i = 0; i < numOperations; ++i) {
        if (rand() % 2) {
            size_t size = getRandomSize(allocationRanges);
            allocations.push_back(size);
            fprintf(file, "alloc %zu\n", size);
        } else {
            if (!allocations.empty()) {
                size_t index = rand() % allocations.size();
                allocations.erase(allocations.begin() + index);
                fprintf(file, "dalloc %zu\n", index);
            }
        }
    }

/*
    for (int i=allocations.size()-1; i >= 0; i--) {
        fprintf(file, "dalloc %zu\n", i);
    }
*/

    fclose(file);

    return 0;
}
