// Compatibility stub for older CubismCore that declares but doesn't export csmHasMocConsistency
#include "Live2DCubismCore.h"

extern "C" {
    int csmHasMocConsistency(void* address, const unsigned int size) {
        (void)address;
        (void)size;
        return 1;
    }
}
