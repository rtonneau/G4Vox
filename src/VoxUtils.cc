#include "G4Vox/VoxUtils.hh"

namespace G4Vox
{
    // Printable with <<
    std::ostream &operator<<(std::ostream &os, const VoxelIndex &v)
    {
        os << "(" << v.x() << ", " << v.y() << ", " << v.z() << ")";
        return os;
    }

}