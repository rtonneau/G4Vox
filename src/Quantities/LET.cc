#include "G4Vox/Quantities/LET.hh"

#include "G4Vox/VoxUtils.hh"
#include "G4TouchableHandle.hh"
#include "G4Step.hh"

#define width 15L

namespace G4Vox
{
    namespace Quantities
    {

        void AccumulableLET::Merge(const G4VAccumulable &other)
        {
            const auto &o = static_cast<const AccumulableLET &>(other);
            this->fData += o.fData;           // Merge energy deposits
            this->fTotLength += o.fTotLength; // Merge track lengths
        }

        void AccumulableLET::Initialize()
        {
            VVoxQuantityAccumulable::Initialize();
            this->fTotLength = array_type(0.0, this->TotalVoxels());
        }

        void AccumulableLET::Score(const G4Step *aStep)
        {
            // ALL electrons contribute — no primary/secondary distinction
            auto *track = aStep->GetTrack();
            if (std::abs(track->GetDefinition()->GetPDGEncoding()) != 11)
                return;

            G4double edep = aStep->GetTotalEnergyDeposit();
            if (edep <= 0.)
                return;
            G4double stepLen = aStep->GetStepLength();
            if (stepLen <= 0.)
                return;

            // Get pre-step point and its position
            // Determine voxel index from position
            // GetTouchable() gives a raw pointer with no reference counting overhead
            const G4VTouchable *th = aStep->GetPreStepPoint()->GetTouchable();
            std::size_t i = G4Vox::CartesianVoxelIndex::FlattenTouchable(th, fNx, fNy);
            if (i < this->TotalVoxels())
            {
                this->fData[i] += edep * edep / stepLen; // Accumulate energy deposit in the voxel
                this->fTotLength[i] += stepLen;          // Accumulate track length for LET calculation
            }
        }

        size_t AccumulableLET::FlattenVoxelIndex(const VoxelIndex &v) const
        {
            // Compute the flat voxel index from the 3D index
            return v.Flatten(this->fNx, this->fNy);
        }

        VVoxQuantityAccumulable *LET::UserCreateAccumulable(const G4String &name) const
        {
            return new AccumulableLET(name, this->GetMaxVoxIndex()); // pass weak_ptr
        }

        void LET::ReadAccumulable(const VVoxQuantityAccumulable &other)
        {
            const auto &o = static_cast<const AccumulableLET &>(other);
            constexpr G4double minStepLen = 1.0 * CLHEP::nm; // tune as needed
            std::valarray<bool> mask = (o.fTotLength > minStepLen);
            // std::valarray<bool> mask = (o.fTotLength > 0.);
            //  where track length > 0, compute LET
            // this->fData[mask] += (o.fData[mask] / G4::keV) / (o.fTotLength[mask] / G4::micrometer);
            this->fData[mask] += o.fData[mask]; // Merge energy deposits
        }

        void LET::Compute()
        {
            // this->fData *= (G4::keV / G4::micrometer); // Convert to keV/um
            this->fComputed = true;
        }

        void LET::Store(G4String path)
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
                    << std::setw(width) << "LET (keV/um)" << G4endl;

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
            // this->StoreVTI(path);
        }

    } // namespace Quantities
} // namespace G4Vox
