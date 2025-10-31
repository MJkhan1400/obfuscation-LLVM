
#include <iostream>

int calculate(int a, int b) {

    int result = a + b;

    result = result + 10;

    result = result + a;

    

    if (result > 50) {

        result = result + 5;

    } else {

        result = result + 3;

    }

    

    for (int i = 0; i < 3; i++) {

        result = result + i;

    }

    

    return result;

}

int main() {

    int x = 10;

    int y = 20;

    int z = x + y;

    

    std::cout << "Result: " << calculate(x, y) << std::endl;

    std::cout << "Sum: " << z << std::endl;

    

    return 0;

}



