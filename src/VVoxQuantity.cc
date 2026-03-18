#include "G4Vox/VVoxQuantity.hh"

#include "G4Vox/VVoxQuantityAccumulable.hh"

namespace G4Vox
{
    void VVoxQuantity::ReadAccumulable(const VVoxQuantityAccumulable &other)
    {
        this->fData += other.GetData(); // valarray operator+= is element-wise ✓
    }

    void VVoxQuantity::Reset()
    {
        if (this->fInitialized)
        {
            this->fData = 0.; // valarray broadcast assign ✓
            this->fComputed = false;
        }
        else
        {
            this->InitializeQuantity();
        }
    }

    void VVoxQuantity::InitializeQuantity()
    {
        this->fComputed = false;
        this->fData = array_type(0.0, this->TotalVoxels());
        this->fInitialized = true;
    }

    void VVoxQuantity::StoreVTI(G4String path)
    {
        G4String file_path = path + this->GetDetectorName() + "_" + this->GetName() + ".vti";
        std::ofstream ofs(file_path);

        auto nVox = this->GetGeometry()->GetMaxVoxIndex().lock();
        const G4int nX = nVox->x() + 1;
        const G4int nY = nVox->y() + 1;
        const G4int nZ = nVox->z() + 1;

        auto voxSize = this->GetGeometry()->GetVoxSize();
        // Physical voxel spacing in mm (ParaView default world unit)
        const G4double dX = voxSize.x() / CLHEP::mm;
        const G4double dY = voxSize.y() / CLHEP::mm;
        const G4double dZ = voxSize.z() / CLHEP::mm;

        if (!ofs.is_open())
        {
            G4cerr << "Cannot open " << file_path << G4endl;
            return;
        }
        G4ThreeVector origin = this->GetGeometry()->GetOrigin();
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
            << "      <CellData Scalars=\"" << this->GetName() << "\">\n"
            << "        <DataArray type=\"Float64\" Name=\"" << this->GetName() << "\"\n"
            << "                   format=\"ascii\" NumberOfComponents=\"1\">\n";

        // ── Data loop — x fastest (VTK/ParaView convention) ───────────────────
        for (G4int k = 0; k < nZ; k++)
            for (G4int j = 0; j < nY; j++)
            {
                for (G4int i = 0; i < nX; i++)
                {
                    size_t v = G4Vox::CartesianVoxelIndex::FlattenIndexes(i, j, k, nX, nY);
                    ofs << std::scientific << std::setprecision(8)
                        << this->fData[v];
                    if (i < nX - 1)
                        ofs << " ";
                }
                ofs << "\n";
            }

        ofs << "        </DataArray>\n"
            << "      </CellData>\n"
            << "    </Piece>\n"
            << "  </ImageData>\n"
            << "</VTKFile>\n";

        this->RegisterOutputFile(file_path);
        if (this->fVerboseLevel > 0)
            G4cout << "[Store] VTI written → " << file_path
                   << "  (" << nX << "×" << nY << "×" << nZ << " voxels)" << G4endl;
    }
}