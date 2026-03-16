#ifndef VoxGeometry_h
#define VoxGeometry_h 1

#include "G4ThreeVector.hh"

#include "G4Vox/VoxUtils.hh"

class G4Box;
class G4LogicalVolume;
class G4VPhysicalVolume;

// Forward declaration of other radiobiology classes
class DetectorConstruction;
class VoxGeometryMessenger;
class G4VSensitiveDetector;

namespace G4Vox
{
  struct Voxel
  {
    std::unique_ptr<VoxelIndex> index;
    G4int flatIndex; // flat index for 1D storage
    G4double mass;
    G4double density;
    G4double volume;
    G4ThreeVector size;
  };

  struct VoxelDivision
  {
    G4Box *solid = nullptr;
    G4LogicalVolume *logical = nullptr;
    G4VPhysicalVolume *physical = nullptr;
  };

  class VoxGeometry
  {
  public:
    // Constructors and destructor
    VoxGeometry(G4VPhysicalVolume *motherVol, G4ThreeVector voxSize);
    VoxGeometry(G4VPhysicalVolume *motherVol, G4int nx, G4int ny, G4int nz);

    // ❌ Explicitly forbid default construction
    VoxGeometry() = delete;
    VoxGeometry(const VoxGeometry &) = delete;            // no copy
    VoxGeometry &operator=(const VoxGeometry &) = delete; // no copy-assign

    ~VoxGeometry();

    // public methods:
  public:
    void InitFromVoxNumber(G4int nx, G4int ny, G4int nz);
    void InitFromVoxSize(G4ThreeVector voxSize);
    void AdjustVoxNumberToOdd();

    void UpdateVoxVolume();
    void ConstructVoxels();
    G4int FlattenThisVox(VoxelIndex *voxel) const;
    void RegisterSD(G4VSensitiveDetector *sd);

    // List of Setters
  public:
    void SetVerboseLevel(G4int level) { this->verboseLevel = level; }

    // List of Getters
  public:
    G4Material *GetMaterial() const { return this->fMotherVolume ? this->fMotherVolume->GetLogicalVolume()->GetMaterial() : nullptr; }
    G4int TotalVoxels() const
    {
      if (!this->fMaxVoxIndex)
        return -1;
      return (this->fMaxVoxIndex->x() + 1) * (this->fMaxVoxIndex->y() + 1) * (this->fMaxVoxIndex->z() + 1);
    }
    std::size_t size() const { return this->TotalVoxels(); }
    G4double GetVoxVolume() const { return this->fVoxVolume; }
    G4double GetVoxDensity() const { return this->fVoxDensity; }
    G4double GetVoxMass() const { return this->fVoxMass; }
    G4ThreeVector GetVoxSize() const { return this->fVoxSize; }
    // Produces the weak handle observers need
    std::weak_ptr<const VoxelIndex> GetMaxVoxIndex() const
    {
      return this->fMaxVoxIndex; // caller can store as weak_ptr
    }
    G4ThreeVector GetOrigin() const { return this->fVoxOrigin; }
    Voxel GetVoxel(G4int i, G4int j, G4int k) const;

    // private methods
  private:
    void ReadMotherVolume();
    void SetMotherVolume(G4VPhysicalVolume *motherVol);

  private:
    // private data members
    G4VPhysicalVolume *fMotherVolume = nullptr;
    G4int verboseLevel = 0;
    std::shared_ptr<VoxelIndex> fMaxVoxIndex = nullptr;

    G4ThreeVector fVoxSize = G4ThreeVector(-1.0, -1.0, -1.0);
    G4ThreeVector fMotherSize = G4ThreeVector(-1.0, -1.0, -1.0);
    G4ThreeVector fVoxOrigin = G4ThreeVector(-1.0, -1.0, -1.0);

    G4double fVoxVolume = -1.0;
    G4double fVoxDensity = -1.0;
    G4double fVoxMass = -1.0;

    std::array<VoxelDivision, 3> fVoxDivisions{};
  };

}
#endif // VoxGeometry_h
