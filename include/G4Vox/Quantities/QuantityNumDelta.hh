#ifndef QUANTITY_NUMDELTA_HH
#define QUANTITY_NUMDELTA_HH

#include "G4Vox/VVoxQuantity.hh"
#include "G4Vox/VVoxQuantityAccumulable.hh"

class AccumulableNumDelta : public G4Vox::VVoxQuantityAccumulable
{
    friend class QuantityNumDelta; // for data access
public:
    // Inherit constructor from base class
    using G4Vox::VVoxQuantityAccumulable::VVoxQuantityAccumulable;

    void Score(const G4Step *step) override;

    size_t FlattenVoxelIndex(const G4Vox::VoxelIndex &v) const override;
};

class QuantityNumDelta : public G4Vox::VVoxQuantity
{
public:
    QuantityNumDelta() : VVoxQuantity("NumDelta") {}

    G4Vox::VVoxQuantityAccumulable *UserCreateAccumulable(const G4String &name) const override;

    void Compute() override;
    void ReadAccumulable(const G4Vox::VVoxQuantityAccumulable &other) override;

    void Store(G4String path = ".") override;
};
#endif