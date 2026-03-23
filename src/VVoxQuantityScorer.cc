#include "G4Vox/VVoxQuantityScorer.hh"

#include "G4SDManager.hh"

namespace G4Vox
{
    void VVoxQuantityScorer::Initialize(G4HCofThisEvent *hce)
    {
        auto *mfd = this->GetMultiFunctionalDetector();
        auto *sdm = G4SDManager::GetSDMpointer();

        for (auto &[name, ptr] : fEvtMaps)
        {
            ptr = new G4THitsMap<G4double>(mfd->GetName(), name);

            sdm->AddNewCollection(mfd->GetName(), name); // declare ✅

            G4int id = sdm->GetCollectionID(mfd->GetName() + "/" + name);

            hce->AddHitsCollection(id, ptr);
        }
    }

    std::valarray<G4double> VVoxQuantityScorer::HitMapToValarray(const G4String &name) const
    {
        auto it = this->fEvtMaps.find(name);
        if (it == this->fEvtMaps.end() || !it->second)
        {
            G4Exception("VVoxQuantityScorer::HitsMapToValarray",
                        "VOXMGR_002",
                        JustWarning,
                        ("Quantity not found: " + name).c_str());
            return std::valarray<G4double>(); // empty
        }
        std::valarray<G4double> result(0.0, this->TotalVoxels()); // zero-initialized

        for (const auto &[copyNo, valuePtr] : *it->second->GetMap())
        {
            if (copyNo < this->TotalVoxels() && valuePtr)
                result[copyNo] = *valuePtr;
        }

        return result;
    }

    std::map<G4String, std::valarray<G4double>> VVoxQuantityScorer::AllHitsMapsToValarrays() const
    {
        std::map<G4String, std::valarray<G4double>> result;
        for (const auto &[name, _] : fEvtMaps)
            result[name] = this->HitMapToValarray(name);
        return result;
    }
} // namespace G4Vox