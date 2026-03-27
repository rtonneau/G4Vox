#ifndef QUANTITY_LET_HH
#define QUANTITY_LET_HH

#include "G4Vox/VVoxQuantity.hh"
#include "G4Vox/VVoxQuantityAccumulable.hh"

class QuantityLET;

class AccumulableLET : public G4Vox::VVoxQuantityAccumulable
{
    friend class QuantityLET; // for data access

public:
    // Inherit constructor from base class
    using G4Vox::VVoxQuantityAccumulable::VVoxQuantityAccumulable;

    void Score(const G4Step *step) override;

    size_t FlattenVoxelIndex(const G4Vox::VoxelIndex &v) const override;

    void Initialize() override;

    void Merge(const G4VAccumulable &other) override;

private:
    G4Vox::array_type fTotLength; // Total track length in the voxel, used for LET calculation
};

class QuantityLET : public G4Vox::VVoxQuantity
{
public:
    QuantityLET() : VVoxQuantity("LET") {}

    G4Vox::VVoxQuantityAccumulable *UserCreateAccumulable(const G4String &name) const override;

    void ReadAccumulable(const G4Vox::VVoxQuantityAccumulable &other) override;

    void Compute() override;

    void Store(G4String path = ".") override;
};
#endif