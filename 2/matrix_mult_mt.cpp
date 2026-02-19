#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <functional>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <map>
#include <limits>
#include <cstdlib>
#include <atomic>
#include <string>


using namespace std;
using hrclock = chrono::high_resolution_clock;

vector<size_t> parse_sizes(const string &s) {
    vector<size_t> out;
    string cur;
    for (char c : s) {
        if (c==',') { if(!cur.empty()) { out.push_back(stoull(cur)); cur.clear(); } }
        else if (!isspace((unsigned char)c)) cur.push_back(c);
    }
    if (!cur.empty()) out.push_back(stoull(cur));
    return out;
}

void *aligned_alloc64(size_t bytes) {
    void *ptr = nullptr;
    int r = posix_memalign(&ptr, 64, bytes);
    if (r != 0) return nullptr;
    return ptr;
}

void parallel_init(double* M, size_t N, int num_threads, function<double(size_t,size_t)> init_fn) {
    auto worker = [&](int tid){
        size_t rows_per = N / num_threads;
        size_t start = tid * rows_per;
        size_t end = (tid==num_threads-1) ? N : start + rows_per;
        for (size_t i = start; i < end; ++i) {
            size_t base = i * N;
            for (size_t j = 0; j < N; ++j) {
                M[base + j] = init_fn(i,j);
            }
        }
    };
    vector<thread> th; th.reserve(num_threads);
    for (int t=0;t<num_threads;++t) th.emplace_back(worker,t);
    for (auto &t: th) t.join();
}

void parallel_transpose(const double* B, double* Bt, size_t N, int num_threads) {
    auto worker = [&](int tid){
        size_t rows_per = N / num_threads;
        size_t i_start = tid * rows_per;
        size_t i_end = (tid==num_threads-1) ? N : i_start + rows_per;
        for (size_t i = i_start; i < i_end; ++i) {
            size_t base_i = i * N;
            for (size_t j = 0; j < N; ++j) {
                Bt[j*N + i] = B[base_i + j];
            }
        }
    };
    vector<thread> th; th.reserve(num_threads);
    for (int t=0;t<num_threads;++t) th.emplace_back(worker,t);
    for (auto &t: th) t.join();
}

void multiply_parallel(const double* A, const double* Bt, double* C, size_t N, int num_threads) {
    auto worker = [&](int tid){
        size_t rows_per = N / num_threads;
        size_t i_start = tid * rows_per;
        size_t i_end = (tid==num_threads-1) ? N : i_start + rows_per;
        for (size_t i = i_start; i < i_end; ++i) {
            const double* Ai = A + i*N;
            double* Ci = C + i*N;
            for (size_t j = 0; j < N; ++j) {
                const double* Btj = Bt + j*N;
                double sum = 0.0;
                for (size_t k = 0; k < N; ++k) sum += Ai[k] * Btj[k];
                Ci[j] = sum;
            }
        }
    };
    vector<thread> th; th.reserve(num_threads);
    for (int t=0;t<num_threads;++t) th.emplace_back(worker,t);
    for (auto &t: th) t.join();
}

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    vector<size_t> sizes = {20000, 40000};
    vector<int> threads = {1,2,4,7,8,16,20,40};
    int runs = 3;
    string outcsv = "results.csv";
    int init_threads = thread::hardware_concurrency() > 0 ? (int)thread::hardware_concurrency() : 4;

    for (int i=1;i<argc;++i) {
        string s = argv[i];
        if (s=="--sizes" && i+1<argc) { sizes = parse_sizes(argv[++i]); }
        else if (s=="--threads" && i+1<argc) {
            auto vs = parse_sizes(argv[++i]); threads.clear(); for (auto v: vs) threads.push_back((int)v);
        }
        else if (s=="--runs" && i+1<argc) { runs = stoi(argv[++i]); }
        else if (s=="--out" && i+1<argc) { outcsv = argv[++i]; }
        else if (s=="--init-threads" && i+1<argc) { init_threads = stoi(argv[++i]); }
        else if (s=="--help") {
            cout << "Usage: " << argv[0] << " [--sizes 2000,4000] [--threads 1,2,4] [--runs 3] [--out file.csv]\n";
            return 0;
        }
    }

    ofstream fout(outcsv);
    if (!fout) {
        cerr << "Cannot open output file: " << outcsv << "\n";
        return 1;
    }
    fout << "size,threads,run,time_seconds\n";

    for (size_t N : sizes) {
        cout << "\\n==== SIZE N=" << N << " ====" << endl;
        long double bytes_per = (long double)N * (long double)N * sizeof(double);
        auto bytes_to_gib = [&](long double b){ return b / (1024.0L*1024.0L*1024.0L); };
        cout << "Approx memory per matrix: " << fixed << setprecision(3) << bytes_to_gib(bytes_per) << " GiB\n";
        cout << "Total approx for A,B,Bt,C: " << setprecision(3) << bytes_to_gib(bytes_per*4.0L) << " GiB\n";

        size_t elems = N * (size_t)N;
        size_t bytes = elems * sizeof(double);
        cout << "Allocating matrices (this may fail if insufficient RAM)...\n";
        double* A = (double*)aligned_alloc64(bytes);
        double* B = (double*)aligned_alloc64(bytes);
        double* Bt = (double*)aligned_alloc64(bytes);
        double* C = (double*)aligned_alloc64(bytes);
        if (!A || !B || !Bt || !C) {
            cerr << "Allocation failed for N=" << N << ". Try smaller size or run on a machine with more RAM.\n";
            free(A); free(B); free(Bt); free(C);
            continue;
        }

        cout << "Initializing A and B with " << init_threads << " threads...\n";
        auto init_fnA = [&](size_t i, size_t j)->double { return 1.0; };
        auto init_fnB = [&](size_t i, size_t j)->double { return 1.0; };

        auto t0 = hrclock::now();
        parallel_init(A, N, init_threads, init_fnA);
        parallel_init(B, N, init_threads, init_fnB);
        auto t1 = hrclock::now();
        double init_seconds = chrono::duration<double>(t1 - t0).count();
        cout << "Initialization took " << init_seconds << " s\n";

        cout << "Transposing B into Bt using " << init_threads << " threads...\n";
        auto tt0 = hrclock::now();
        parallel_transpose(B, Bt, N, init_threads);
        auto tt1 = hrclock::now();
        cout << "Transpose took " << chrono::duration<double>(tt1-tt0).count() << " s\n";

        double time_one_thread = -1.0;
        map<int,double> best_time; 
        for (int tcount : threads) {
            cout << "Running multiplication with " << tcount << " threads (" << runs << " runs)...\n";
            double best = numeric_limits<double>::infinity();
            for (int r=0;r<runs;++r) {
                auto zero_worker = [&](int tid){
                    size_t rows_per = N / tcount;
                    size_t start = tid * rows_per;
                    size_t end = (tid==tcount-1) ? N : start + rows_per;
                    for (size_t i=start;i<end;++i) {
                        memset(C + i*N, 0, N * sizeof(double));
                    }
                };
                vector<thread> zth; zth.reserve(tcount);
                for (int tt=0; tt<tcount; ++tt) zth.emplace_back(zero_worker, tt);
                for (auto &thd : zth) thd.join();

                auto tstart = hrclock::now();
                multiply_parallel(A, Bt, C, N, tcount);
                auto tend = hrclock::now();
                double sec = chrono::duration<double>(tend - tstart).count();
                cout << "  run " << r+1 << ": " << sec << " s\n";
                fout << N << "," << tcount << "," << (r+1) << "," << setprecision(6) << sec << "\n";
                if (sec < best) best = sec;
            }
            best_time[tcount] = best;
            cout << "  best time for " << tcount << " threads: " << best << " s\n";
            if (tcount==1) time_one_thread = best;
        }

        cout << "Speedups (relative to 1 thread):\n";
        if (time_one_thread > 0) {
            for (auto &p : best_time) {
                double sp = time_one_thread / p.second;
                cout << "  threads=" << p.first << " speedup=" << fixed << setprecision(3) << sp << "\n";
            }
        } else cout << "  No 1-thread baseline recorded.\n";

        free(A); free(B); free(Bt); free(C);
        cout << "Finished size " << N << "\n";
    }

    cout << "Results written to " << outcsv << "\n";
    fout.close();
    return 0;
}
