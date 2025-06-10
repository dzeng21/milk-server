#include "milk_matrix.h"
#include "milk_server.h"

using namespace std;

ThreadPool pool;

int main() {
    pool.init();

    Matrix a = Matrix();
    Matrix b = Matrix();
    Matrix c = Matrix();

    a.construct("[1 2; 3 4]");
    b.construct("[5 6; 7 8]");

    c.multiply(a, b, c, pool);

    cout << c << "\n";

    return 0;
}