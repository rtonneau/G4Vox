#ifndef VoxQuantityManager_h
#define VoxQuantityManager_h 1

#include "G4VSensitiveDetector.hh"
#include <map>
#include "G4Step.hh"

class VoxQuantityManagerMessenger;

namespace G4Vox
{
  class VVoxQuantity;
  class VVoxQuantityAccumulable;
  class VoxGeometry;
  struct VoxRegion;
  class VoxSD;

  class VoxQuantityManager
  {
  private:
    VoxQuantityManager() = default;
    ~VoxQuantityManager();

  public:
    // Singleton structure
    static VoxQuantityManager *GetInstance();
    static void DeleteInstance();

    // called from DetectorConstruction
    void RegisterRegion(const G4String &regionName, VoxGeometry *geo);
    // called from user setup
    void Register(const G4String &regionName,
                  std::unique_ptr<VVoxQuantity> q);

    // Defining Sensitive Detector:
    void ConstructSDs();

    // Accessors for run lifecycle and SD processing

    // Interactions with G4AccumulableManager
    void RegisterAllAccumulables();
    void CallResetG4Accumulables();
    void CallMergeG4Accumulables();

    // run lifecycle
    void InitializeAll(bool accumulate = true);
    void ComputeAll();
    void ResetAll();
    void StoreAll();
    void ReadAccumulables();

    VoxRegion *GetRegion(const G4String &regionName) const;
    VVoxQuantity *GetQuantity(const G4String &regionName,
                              const G4String &quantityName) const;
    void SetRootPath(const G4String &path);
    const G4String &GetRootPath() const { return this->fRootPath; }
    const std::vector<G4String> &GetOrderedRegions() const { return this->fOrderedRegions; }
    std::string Print() const;
    void WriteManifest() const;
    void SetPrefix(const G4String &prefix) { this->fPrefix = prefix; }
    const G4String &GetPrefix() const { return this->fPrefix; }

  private:
    std::vector<G4String> fOrderedRegions;
    std::map<G4String, std::unique_ptr<VoxRegion>> fRegions;
    G4String fRootPath = ".";
    G4String fPrefix = "";
    G4bool fisInitialized = false;

    inline static VoxQuantityManager *fInstance = nullptr;
  };
}

#endif // VoxQuantityManager_h
