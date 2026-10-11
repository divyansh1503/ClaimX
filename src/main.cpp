#include "LostFoundSystem.h"

int main() {
    LostFoundSystem system;  // saves to data/items.txt
    system.load();
    system.run();
    return 0;
}

