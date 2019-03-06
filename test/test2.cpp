#include <string>

class C {
    std::string s;
    double d;
    int * i;

public:
    C() : i(new int) {
        s = "hello";
        d = 1.23;
    }
};

int main() {
    char * str;
    int * i = new int;
    C * c = new C();
    str = new char[10];
}
