#include <iostream>
#include <cmath>
#include <chrono>
#include <cstring>

using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        return 1;
    }
    const size_t N = 10000000;
    const double two_pi = 2.0 * M_PI;
    
    if (strcmp(argv[1], "float") == 0) {
        auto start = chrono::high_resolution_clock::now();
        
        float* array = new float[N];
        float sum = 0.0f;
        
        for (size_t i = 0; i < N; ++i) {
            float phase = static_cast<float>(i) / static_cast<float>(N) * static_cast<float>(two_pi);
            array[i] = sinf(phase); 
            sum += array[i];
        }
        
        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double> elapsed = end - start;
        
        cout << "Sum: " << sum << endl;
        cout << "Time: " << elapsed.count() << endl;
        delete[] array;

    } else if (strcmp(argv[1], "double") == 0) {
        auto start = chrono::high_resolution_clock::now();

        double* array = new double[N];
        double sum = 0.0;
        
        for (size_t i = 0; i < N; ++i) {
            double phase = static_cast<double>(i) / static_cast<double>(N) * two_pi;
            array[i] = sin(phase);
            sum += array[i];
        }
        
        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double> elapsed = end - start;
        
        cout << "Sum: " << sum << endl;
        cout << "Time: " << elapsed.count() << endl;
        delete[] array;

    } else {
        return 1;
    }
    return 0;
}