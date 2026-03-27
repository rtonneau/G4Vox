#ifndef QUANTITY_LET_HH
#define QUANTITY_LET_HH

#include "G4Vox/VVoxQuantity.hh"
#include "G4Vox/VVoxQuantityAccumulable.hh"

namespace G4Vox
{
    namespace Quantities
    {

        class LET;

        class AccumulableLET : public VVoxQuantityAccumulable
        {
            friend class LET; // for data access

        public:
            // Inherit constructor from base class
            using VVoxQuantityAccumulable::VVoxQuantityAccumulable;

            void Score(const G4Step *step) override;

            size_t FlattenVoxelIndex(const VoxelIndex &v) const override;

            void Initialize() override;

            void Merge(const G4VAccumulable &other) override;

        private:
            array_type fTotLength; // Total track length in the voxel, used for LET calculation
        };

        class LET : public VVoxQuantity
        {
        public:
            LET() : VVoxQuantity("LET") {}

            VVoxQuantityAccumulable *UserCreateAccumulable(const G4String &name) const override;

            void ReadAccumulable(const VVoxQuantityAccumulable &other) override;

            void Compute() override;

            void Store(G4String path = ".") override;
        };

    } // namespace Quantities
} // namespace G4Vox

#endif