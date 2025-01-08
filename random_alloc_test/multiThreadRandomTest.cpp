#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <mutex>
#include <fstream>
#include <jemalloc/jemalloc.h>

std::mutex mtx;

void dumpMallocStats() {
    const char* opts = "g";  // General information
    malloc_stats_print(NULL, NULL, opts);
}

void probeMemoryStats(size_t time) {
    uint64_t epoch = 1;
    size_t sz = sizeof(epoch);
    mallctl("epoch", &epoch, &sz, &epoch, sz);

    size_t allocated, active, resident, pdirty, purged, metadata;
    sz = sizeof(size_t);
    mallctl("stats.allocated", &allocated, &sz, NULL, 0);
    mallctl("stats.active", &active, &sz, NULL, 0);
    mallctl("stats.resident", &resident, &sz, NULL, 0);
    mallctl("stats.arenas.4096.pdirty", &pdirty, &sz, NULL, 0);
    mallctl("stats.arenas.4096.dirty_purged", &purged, &sz, NULL, 0);
    mallctl("stats.metadata", &metadata, &sz, NULL, 0);
    std::cout << allocated << ";" << active << ";" << resident << ";"<< pdirty << ";"<<purged<<";"<<metadata<<";" << time << std::endl;
}

static inline void *no_opt_ptr(void *ptr) {
    asm volatile("" : "+r"(ptr));
    return ptr;
}

void performOperations(int id, int runTimeSeconds, std::string file_name) {
    auto startTime = std::chrono::steady_clock::now();
    std::vector<void*> allocations;
    std::ifstream inputFile;
    long long numOps = 0;

    inputFile.open(file_name);
    if (!inputFile.is_open()) {
        std::cerr << "Failed to open file: " << file_name << std::endl;
        return;
    }

    while (true) {
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count();

        if (elapsed >= runTimeSeconds) {
            break;
        }
        std::string operation;
        size_t size_or_index;
        if (inputFile >> operation >> size_or_index) {
            if (operation == "alloc") {
                void* ptr = malloc(size_or_index);
                if (ptr) {
                    no_opt_ptr(ptr);
                    allocations.push_back(ptr);
                }
            } else if (operation == "dalloc") {
                if (size_or_index < allocations.size()) {
                    free(allocations[size_or_index]);
                    allocations.erase(allocations.begin() + size_or_index);
                }
            }
            numOps ++;
        } else {
            // End of file or invalid input
            break;
        }
    }

    for (void* ptr : allocations) {
        free(ptr);
    }
    allocations.clear();
    std::cout << "Total numOps: " << numOps <<std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 5) {
        std::cerr << "Usage: " << argv[0] << " <num_threads> <runTimeSeconds> <probeInterval> <file>" << std::endl;
        return 1;
    }

    int num_threads = std::stoi(argv[1]);
    int runTimeSeconds = std::stoi(argv[2]);
    int probeInterval = std::stoi(argv[3]);
    std::string file_name = argv[4];

    std::vector<std::thread> threads;

    // Launch threads
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(performOperations, i, runTimeSeconds, file_name);
    }

    // Main thread probes memory stats
    auto startTime = std::chrono::steady_clock::now();
    while (true) {
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count();

        if (elapsed >= runTimeSeconds) {
            break;
        }

        probeMemoryStats(elapsed);
        std::this_thread::sleep_for(std::chrono::seconds(probeInterval));
    }

    // Join all threads
    for (auto& th : threads) {
        th.join();
    }

    // Final 30 seconds wait with probing every 10 seconds
    for (int i = 0; i < 3; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        probeMemoryStats(1000000);
    }

    std::cout << "Program ended." << std::endl;
    return 0;
}
