#ifndef QUANTITY_TRACK_LENGTH_HH
#define QUANTITY_TRACK_LENGTH_HH

#include "G4Vox/VVoxQuantity.hh"
#include "G4Vox/VVoxQuantityAccumulable.hh"

namespace G4Vox
{
    namespace Quantities
    {

        class QuantityTrackLength;

        class AccumulableTrackLength : public VVoxQuantityAccumulable
        {
            friend class QuantityTrackLength; // for data access

        public:
            // Inherit constructor from base class
            using VVoxQuantityAccumulable::VVoxQuantityAccumulable;

            void Score(const G4Step *step) override;

            size_t FlattenVoxelIndex(const VoxelIndex &v) const override;

            void Merge(const G4VAccumulable &other) override;

        private:
            array_type fTotLength; // Total track length in the voxel, used for track length calculation
        };

        class QuantityTrackLength : public VVoxQuantity
        {
        public:
            QuantityTrackLength() : VVoxQuantity("TrackLength") {}

            VVoxQuantityAccumulable *UserCreateAccumulable(const G4String &name) const override;

            void ReadAccumulable(const VVoxQuantityAccumulable &other) override;

            void Compute() override;

            void Store(G4String path = ".") override;
        };

    } // namespace Quantities
} // namespace G4Vox

#endif