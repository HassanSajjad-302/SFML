#include <filesystem>
#include <format>
#include <iostream>

using std::string, std::filesystem::path, std::chrono::high_resolution_clock, std::filesystem::current_path;

double calculateAverageTime(std::chrono::duration<double, std::milli> totalTime, int iterations)
{
    return totalTime.count() / iterations;
}

static void touchFile(const path &filePath)
{
    string command;
#ifdef _WIN32
    command = std::format("powershell (ls {}).LastWriteTime = Get-Date", "\"" + filePath.string() + "\"");
#else
    command = fmt::format("touch {}", addQuotes(filePath.string()));
#endif
    if (system(command.c_str()) == EXIT_FAILURE)
    {
        std::cerr << "Could not touch file " << filePath.string() << " \n";
        exit(EXIT_FAILURE);
    }
}

auto executeHBuild(path &executionPath)
{
    path p = current_path();
    current_path(current_path() / executionPath);
    auto start = high_resolution_clock::now();
    if (system("hbuild") != EXIT_SUCCESS)
    {
        std::cerr << "Failed Executing hbuild\n";
        exit(EXIT_FAILURE);
    }
    auto end = high_resolution_clock::now();
    current_path(p);
    return end - start;
}

int main()
{
    const int iterations = 10;

    path win32cppFilePath = "../../examples/win32/Win32.cpp";

    path conventional = "../conventional/Example-cpp";
    path dropIn = "../drop-in/Example-cpp";
    path hu = "../hu/Example-cpp";

    std::chrono::nanoseconds conventionalTime(0), huTime(0), dropInTime(0);

    for (int i = 0; i < iterations; i++)
    {
        touchFile(win32cppFilePath);
        conventionalTime += executeHBuild(conventional);
        huTime += executeHBuild(hu);
        dropInTime += executeHBuild(dropIn);
    }

    // Calculate the average time taken for each function
    double avgTimeConventional = calculateAverageTime(conventionalTime, iterations);
    double avgTimeHU = calculateAverageTime(huTime, iterations);
    double avgTimeDropIn = calculateAverageTime(dropInTime, iterations);

    // Print the average times
    std::cout << "Average time for conventional: " << avgTimeConventional << " milliseconds" << std::endl;
    std::cout << "Average time for drop-in: " << avgTimeDropIn << " milliseconds" << std::endl;
    std::cout << "Average time for hu: " << avgTimeHU << " milliseconds" << std::endl;

    return 0;
}