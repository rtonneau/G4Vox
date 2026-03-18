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

    void VoxRegion::ExportToVTI(const G4String &filePath)
    {
        if (this->Size() == 0)
            return;

        G4String file_path = filePath;
        std::ofstream ofs(file_path);
        if (!ofs.is_open())
        {
            G4cerr << "Cannot open " << file_path << G4endl;
            return;
        }

        auto nVox = this->geometry->GetMaxVoxIndex().lock();
        const G4int nX = nVox->x() + 1;
        const G4int nY = nVox->y() + 1;
        const G4int nZ = nVox->z() + 1;

        auto voxSize = this->geometry->GetVoxSize();
        // Physical voxel spacing in mm (ParaView default world unit)
        const G4double dX = voxSize.x() / CLHEP::mm;
        const G4double dY = voxSize.y() / CLHEP::mm;
        const G4double dZ = voxSize.z() / CLHEP::mm;

        G4ThreeVector origin = this->geometry->GetOrigin();
        // WholeExtent: point indices  → 0..nX, 0..nY, 0..nZ
        // Piece Extent same as WholeExtent for single-piece file
        ofs << "<?xml version=\"1.0\"?>\n"
            << "<VTKFile type=\"ImageData\" version=\"0.1\" byte_order=\"LittleEndian\">\n"
            << "  <ImageData WholeExtent=\""
            << "0 " << nX << " 0 " << nY << " 0 " << nZ << "\"\n"
            << " Origin=\""
            << origin.x() << " "
            << origin.y() << " "
            << origin.z() << "\""
            << "             Spacing=\"" << dX << " " << dY << " " << dZ << "\">\n"
            << "    <Piece Extent=\"0 " << nX << " 0 " << nY << " 0 " << nZ << "\">\n"
            << "      <CellData>\n";

        for (const auto &qty : this->quantities)
        {
            ofs << "        <DataArray type=\"Float64\" Name=\"" << qty->GetName() << "\"\n"
                << "                   format=\"ascii\" NumberOfComponents=\"1\">\n";

            // ── Data loop — x fastest (VTK/ParaView convention) ───────────────────
            auto data = qty->GetData(); // get const reference to quantity data
            for (G4int k = 0; k < nZ; k++)
                for (G4int j = 0; j < nY; j++)
                {
                    for (G4int i = 0; i < nX; i++)
                    {
                        size_t v = G4Vox::CartesianVoxelIndex::FlattenIndexes(i, j, k, nX, nY);
                        ofs << std::scientific << std::setprecision(8)
                            << data[v];
                        if (i < nX - 1)
                            ofs << " ";
                    }
                    ofs << "\n";
                }
            ofs << "        </DataArray>\n";
        }

        ofs << "      </CellData>\n"
            << "    </Piece>\n"
            << "  </ImageData>\n"
            << "</VTKFile>\n";
        this->RegisterOutputFile(file_path);
    }

    G4bool VoxSD::ProcessHits(G4Step *aStep, G4TouchableHistory *)
    {
        for (auto &accum : this->fRegion->accumulables)
            accum->Score(aStep);
        return true;
    }
}