#ifndef QUANTITY_ENERGYDEP_HH
#define QUANTITY_ENERGYDEP_HH

#include "G4Vox/VVoxQuantity.hh"
#include "G4Vox/VVoxQuantityAccumulable.hh"

namespace G4Vox
{
    namespace Quantities
    {

        class AccumulableEnergyDep : public VVoxQuantityAccumulable
        {
            friend class QuantityEnergyDep; // for data access
        public:
            // Inherit constructor from base class
            using VVoxQuantityAccumulable::VVoxQuantityAccumulable;

            void Score(const G4Step *step) override;

            size_t FlattenVoxelIndex(const VoxelIndex &v) const override;
        };

        class QuantityEnergyDep : public VVoxQuantity
        {
        public:
            QuantityEnergyDep() : VVoxQuantity("Edep") {}

            VVoxQuantityAccumulable *UserCreateAccumulable(const G4String &name) const override;

            void Compute() override;
            void ReadAccumulable(const VVoxQuantityAccumulable &other) override;

            void Store(G4String path = ".") override;
        };

    } // namespace Quantities
} // namespace G4Vox

#endif