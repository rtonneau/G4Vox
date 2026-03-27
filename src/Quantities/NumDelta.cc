#include "G4Vox/Quantities/NumDelta.hh"

#include "G4Vox/VoxUtils.hh"
#include "G4TouchableHandle.hh"
#include "G4Step.hh"

#define width 15L

namespace G4Vox
{
    namespace Quantities
    {

        void AccumulableNumDelta::Score(const G4Step *aStep)
        {
            // Get secondaries produced in this step
            const std::vector<const G4Track *> *secondaries =
                aStep->GetSecondaryInCurrentStep();

            if (!secondaries)
                return;

            const G4VTouchable *th = aStep->GetPreStepPoint()->GetTouchable();
            std::size_t i = G4Vox::CartesianVoxelIndex::FlattenTouchable(th, fNx, fNy);
            if (i >= this->TotalVoxels())
                return;

            for (const G4Track *sec : *secondaries)
            {
                // Delta electron = secondary electron (PDG ±11)
                if (std::abs(sec->GetDefinition()->GetPDGEncoding()) == 11)
                {
                    this->fData[i] += 1.0; // count
                }
            }
        }

        size_t AccumulableNumDelta::FlattenVoxelIndex(const VoxelIndex &v) const
        {
            // Compute the flat voxel index from the 3D index
            return v.Flatten(this->fNx, this->fNy);
        }

        VVoxQuantityAccumulable *NumDelta::UserCreateAccumulable(const G4String &name) const
        {
            return new AccumulableNumDelta(name, this->GetMaxVoxIndex()); // pass weak_ptr
        }

        void NumDelta::ReadAccumulable(const VVoxQuantityAccumulable &other)
        {
            const auto &o = static_cast<const AccumulableNumDelta &>(other);
            this->fData += o.fData; // Merge number of delta electrons
        }

        void NumDelta::Compute()
        {
            // convert to keV
            // this->fData /= G4::keV;

            // For energy deposition, no post-processing is needed after merging
            this->fComputed = true;
        }

        void NumDelta::Store(G4String path)
        {
            // Implement logic to store fData to disk, e.g. as a CSV or binary file
            // This is a placeholder and should be replaced with actual file I/O code
            if (this->fVerboseLevel > 0)
            {
                G4cout << "Storing " << this->GetName() << " data for " << this->TotalVoxels() << " voxels." << G4endl;
            }

            G4String file_path = path + this->GetDetectorName() + "_" + this->GetName() + ".csv";
            std::ofstream ofs(file_path);
            auto nVox = this->GetMaxVoxIndex().lock();
            if (ofs.is_open())
            {
                ofs << "x_index" << std::setw(width) << "y_index" << std::setw(width) << "z_index"
                    << std::setw(width) << "NumDeltaElectrons" << G4endl;

                G4int nX = nVox->x() + 1;
                G4int nY = nVox->y() + 1;
                for (G4int i = 0; i < nVox->x(); i++)
                    for (G4int j = 0; j < nVox->y(); j++)
                        for (G4int k = 0; k < nVox->z(); k++)
                        {
                            size_t v = G4Vox::CartesianVoxelIndex::FlattenIndexes(i, j, k, nX, nY);
                            ofs << i << std::setw(width) << j << std::setw(width) << k << std::setw(width)
                                << this->fData[v] << G4endl;
                        }
            }
            this->RegisterOutputFile(file_path);
            this->StoreVTI(path);
        }

    } // namespace Quantities
} // namespace G4Vox
