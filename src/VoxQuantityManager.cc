#include "G4Vox/VoxQuantityManager.hh"
#include "G4Vox/VVoxQuantity.hh"
#include "G4Vox/VVoxQuantityAccumulable.hh"
#include "G4Vox/VoxGeometry.hh"
#include "G4Vox/VoxSD.hh"
#include "G4Vox/VoxUtils.hh"
#include "G4Vox/TOMLManager.hh"

#include "G4AccumulableManager.hh"

#include <memory>
#include <vector>
#include <string>
#include <filesystem>

namespace G4Vox
{

    VoxQuantityManager::~VoxQuantityManager() = default;

    VoxQuantityManager *VoxQuantityManager::GetInstance()
    {
        if (VoxQuantityManager::fInstance == nullptr)
        {
            VoxQuantityManager::fInstance = new VoxQuantityManager();
        }
        return VoxQuantityManager::fInstance;
    }

    void VoxQuantityManager::DeleteInstance()
    {
        delete VoxQuantityManager::fInstance;
        VoxQuantityManager::fInstance = nullptr;
    }

    void VoxQuantityManager::RegisterRegion(const G4String &regionName, VoxGeometry *geo)
    {
        if (fRegions.find(regionName) == fRegions.end())
        {
            auto region = std::make_unique<VoxRegion>();
            region->geometry = geo;
            fOrderedRegions.push_back(regionName);
            fRegions[regionName] = std::move(region);
        }
        else
        {
            G4Exception("VoxQuantityManager::RegisterRegion", "G4Vox001", FatalException,
                        ("Region " + regionName + " already exists.").c_str());
        }
    }

    void VoxQuantityManager::Register(const G4String &regionName,
                                      std::unique_ptr<VVoxQuantity> q)
    {
        auto it = this->fRegions.find(regionName);
        if (it == this->fRegions.end())
        {
            G4Exception("VoxQuantityManager::Register", "VQM002",
                        FatalException, ("Unknown region: " + regionName).c_str());
        }

        VoxRegion *region = it->second.get();

        auto &names = region->orderedNames;
        auto &quantity_name = q->GetName();
        if (std::find(names.begin(), names.end(), quantity_name) != names.end())
        {
            G4Exception("VoxQuantityManager::Register", "VQM003",
                        FatalException, ("Quantity already registered: " + quantity_name).c_str());
        }

        q->LinkGeometry(region->geometry);
        q->SetDetectorName(regionName); // set detector name for later use in SD processing
        region->Register(std::move(q)); // ownership transferred
    }

    void VoxQuantityManager::ConstructSDs()
    {
        for (auto &name : this->fOrderedRegions)
        {
            auto &region = this->fRegions.at(name);
            if (!region->geometry)
                continue;
            auto sd = new VoxSD(name + "_SD", region.get()); // SD takes non-owning raw pointer to region
            region->geometry->RegisterSD(sd);                // pass actual SD
        }
    }

    void VoxQuantityManager::RegisterAllAccumulables()
    {
        auto *accMgr = G4AccumulableManager::Instance();
        for (auto &regionName : this->GetOrderedRegions())
        {
            auto *region = this->GetRegion(regionName);
            for (auto &acc : region->accumulables)
            {
                accMgr->Register(acc.get());
            }
        }
    }
    void VoxQuantityManager::CallResetG4Accumulables()
    {
        auto *accMgr = G4AccumulableManager::Instance();
        accMgr->Reset();
    }

    void VoxQuantityManager::CallMergeG4Accumulables()
    {
        auto *accMgr = G4AccumulableManager::Instance();
        accMgr->Merge();
    }

    void VoxQuantityManager::InitializeAll(bool accumulate)
    {
        if (this->fisInitialized && accumulate)
            return;
        for (auto &name : fOrderedRegions)
            this->fRegions.at(name)->InitializeAll();
        this->fisInitialized = true;
    }

    void VoxQuantityManager::ComputeAll()
    {
        for (auto &name : fOrderedRegions)
            this->fRegions.at(name)->ComputeAll();
    }

    void VoxQuantityManager::ResetAll()
    {
        for (auto &name : fOrderedRegions)
            this->fRegions.at(name)->ResetAll();
    }

    void VoxQuantityManager::StoreAll()
    {
        for (auto &name : fOrderedRegions)
            this->fRegions.at(name)->StoreAll(this->GetRootPath() + this->GetPrefix());
    }
    void VoxQuantityManager::ReadAccumulables()
    {
        for (auto &name : fOrderedRegions)
        {
            auto &region = this->fRegions.at(name);
            for (size_t idx = 0; idx < region->quantities.size(); ++idx)
                region->quantities[idx]->ReadAccumulable(*region->accumulables[idx]);
        }
    }
    VoxRegion *VoxQuantityManager::GetRegion(const G4String &regionName) const
    {
        auto it = this->fRegions.find(regionName);
        if (it == this->fRegions.end())
            return nullptr;
        return it->second.get();
    }

    void VoxQuantityManager::SetRootPath(const G4String &path)
    {
        namespace fs = std::filesystem;

        fs::path p(path.c_str());

        if (!fs::exists(p))
        {
            G4cout << "[VoxQuantityManager] Directory not found, creating: "
                   << path << G4endl;

            if (!fs::create_directories(p))
            {
                G4Exception("VoxQuantityManager::SetRootPath",
                            "VOXMGR_001",
                            FatalException,
                            ("Failed to create directory: " + path).c_str());
            }
            G4cout << "[VoxQuantityManager] Directory created OK." << G4endl;
        }
        else if (!fs::is_directory(p))
        {
            G4Exception("VoxQuantityManager::SetRootPath",
                        "VOXMGR_002",
                        FatalException,
                        ("Path exists but is NOT a directory: " + path).c_str());
        }
        else
        {
            G4cout << "[VoxQuantityManager] Directory already exists: "
                   << path << G4endl;
        }

        this->fRootPath = path + "/";
    }

    std::string VoxQuantityManager::Print() const
    {
        std::ostringstream oss;

        oss << "\n"
            << "+------------------------------------------+\n"
            << "|         VoxQuantityManager               |\n"
            << "+------------------------------------------+\n"
            << "  Registered regions : " << fOrderedRegions.size() << "\n\n";

        for (size_t r = 0; r < fOrderedRegions.size(); ++r)
        {
            const G4String &regionName = fOrderedRegions[r];
            const bool isLast = (r == fOrderedRegions.size() - 1);

            auto it = fRegions.find(regionName);
            if (it == fRegions.end())
            {
                oss << (isLast ? "\\-- " : "|-- ")
                    << "Region \"" << regionName << "\"  <NOT FOUND IN MAP>\n";
                continue;
            }

            oss << (isLast ? "\\-- " : "|-- ")
                << "Region [" << r << "] : \"" << regionName << "\"\n";

            const std::string childIndent = isLast ? "    " : "|   ";
            oss << it->second->Print(childIndent);
            oss << "\n";
        }

        return oss.str();
    }
    void VoxQuantityManager::WriteManifest() const
    {
        auto *toml = TOMLManager::GetInstance();

        for (const auto &regionName : this->fOrderedRegions)
        {
            auto it = this->fRegions.find(regionName);
            if (it == this->fRegions.end())
                continue;

            const VoxRegion &region = *it->second;

            if (!region.geometry)
            {
                G4Exception("VoxQuantityManager::WriteManifest", "VQM004",
                            JustWarning,
                            ("Region " + regionName + " has no linked geometry. "
                                                      "Manifest may be incomplete.")
                                .c_str());
                continue;
            }

            auto nVox = region.geometry->GetMaxVoxIndex().lock();
            auto origin = region.geometry->GetOrigin();
            auto vox_size = region.geometry->GetVoxSize();

            auto [dx, ux] = G4Vox::BestLengthUnit(vox_size.x());
            auto [dy, uy] = G4Vox::BestLengthUnit(vox_size.y());
            auto [dz, uz] = G4Vox::BestLengthUnit(vox_size.z());
            auto [ox, uox] = G4Vox::BestLengthUnit(origin.x());
            auto [oy, uoy] = G4Vox::BestLengthUnit(origin.y());
            auto [oz, uoz] = G4Vox::BestLengthUnit(origin.z());

            // ── build region table ────────────────────────────────────────
            toml::table regTable;
            regTable.insert("name", regionName.c_str());
            regTable.insert("nx", (int64_t)(nVox->x() + 1));
            regTable.insert("ny", (int64_t)(nVox->y() + 1));
            regTable.insert("nz", (int64_t)(nVox->z() + 1));
            regTable.insert("vox_size", toml::array{dx, dy, dz});
            regTable.insert("vox_size_unit", ux.c_str());
            regTable.insert("origin", toml::array{ox, oy, oz});
            regTable.insert("origin_unit", uox.c_str());

            // ── quantities sub-array ──────────────────────────────────────
            toml::array qtiesArray;
            for (const auto &qty : region.quantities)
            {
                toml::table qtyTable;
                qtyTable.insert("name", qty->GetName().c_str());

                toml::array filesArray;
                for (auto f : qty->GetStoredFiles())
                {
                    std::replace(f.begin(), f.end(), '\\', '/');
                    filesArray.push_back(f.c_str());
                }
                qtyTable.insert("files", std::move(filesArray));
                qtiesArray.push_back(std::move(qtyTable));
            }
            regTable.insert("quantities", std::move(qtiesArray));

            // ── push into [[regions]] ─────────────────────────────────────
            toml->AppendTable("VoxQuantityManager", "regions", std::move(regTable));
        }
        G4cout << "[VoxQuantityManager] Manifest entries added to TOMLManager" << G4endl;
    }
} // End of namespace G4Vox