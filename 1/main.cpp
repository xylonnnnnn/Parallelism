#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>

#ifdef USE_FLOAT
    using value_type = float;
    #define SIN_FUNC(x) sinf(x)
#else
    using value_type = double;
    #define SIN_FUNC(x) sin(x)
#endif

using namespace std;

int main() {
    size_t N = 10000000;
    vector<value_type> arr(N);
    value_type sum = 0;
    for (size_t i = 0; i < N; i++) {
        value_type arg = static_cast<value_type>(2.0 * M_PI * i / N);
        arr[i] = SIN_FUNC(arg);
        sum += arr[i];
    }
    string type_name;
    int precision;
    if (sizeof(value_type) == 4) {
        type_name = "float";
        precision = 6;
    } else {
        type_name = "double";
        precision = 15;
    }
    cout << fixed << setprecision(precision) << "Сумма (" << type_name << "): " << sum << std::endl;
    return 0;
}
