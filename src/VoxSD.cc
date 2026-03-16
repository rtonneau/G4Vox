#include "G4Vox/VoxSD.hh"

#include "G4Vox/VoxGeometry.hh"

#include "G4Step.hh"
#include "G4TouchableHistory.hh"

namespace G4Vox
{
    void VoxRegion::Register(std::unique_ptr<VVoxQuantity> q)
    {
        this->orderedNames.push_back(q->GetName());
        this->accumulables.push_back(std::unique_ptr<VVoxQuantityAccumulable>(q->CreateAccumulable()));
        this->quantities.push_back(std::move(q));
    }

    std::string VoxRegion::Print(const std::string &indent) const
    {
        std::ostringstream oss;

        oss << indent << "geometry ptr : "
            << static_cast<void *>(geometry) << "\n";

        // --- voxel count ---
        if (geometry)
        {
            auto maxIdx = geometry->GetMaxVoxIndex().lock();
            G4int total = geometry->TotalVoxels();
            auto voxSize = geometry->GetVoxSize();

            if (maxIdx && total > 0)
            {
                oss << indent
                    << "voxels      : "
                    << (maxIdx->x() + 1) << " x "
                    << (maxIdx->y() + 1) << " x "
                    << (maxIdx->z() + 1)
                    << "  =  " << total << " total\n";

                oss << indent
                    << "voxel size  : ("
                    << std::fixed << std::setprecision(3)
                    << voxSize.x() / CLHEP::um << ", "
                    << voxSize.y() / CLHEP::um << ", "
                    << voxSize.z() / CLHEP::um
                    << ") um\n";
            }
        }
        else
            oss << indent << "voxels      : <no geometry>\n";

        oss << indent << "orderedNames : [ ";
        for (const auto &n : orderedNames)
            oss << n << " ";
        oss << "]\n";

        oss << indent << "quantities (" << quantities.size() << ") :\n";
        for (size_t i = 0; i < quantities.size(); ++i)
        {
            const auto &q = quantities[i];
            const bool qLast = (i == quantities.size() - 1);
            oss << indent
                << (qLast ? "  \\-- " : "  |-- ")
                << "[" << i << "] "
                << (q ? q->GetName().c_str() : "<null>")
                << " Sum is "
                //<< (q ? q->Sum() : 0.)
                << "\n";
        }
        return oss.str();
    }

    G4bool VoxSD::ProcessHits(G4Step *aStep, G4TouchableHistory *)
    {
        for (auto &accum : this->fRegion->accumulables)
            accum->Score(aStep);
        return true;
    }
}