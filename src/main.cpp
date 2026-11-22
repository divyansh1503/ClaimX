#include "ClaimManager.h"
#include "LostFoundSystem.h"

int main() {
    LostFoundSystem system;  // items: saves to data/items.txt
    ClaimManager claims(system);  // claims: saves to data/claims.txt, decision: saves to data/decision.txt

    system.load();              // load items
    claims.load();              // index items, load claims + decision log
    claims.registerMenus();     // insert the new modules into the menu
    system.run();
    return 0;
}

