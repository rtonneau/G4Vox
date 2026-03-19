#ifndef VoxQuantityManager_h
#define VoxQuantityManager_h 1

#include "G4VSensitiveDetector.hh"
#include <map>
#include "G4Step.hh"

namespace G4Vox
{
  class VVoxQuantity;
  class VVoxQuantityAccumulable;
  class VoxGeometry;
  struct VoxRegion;
  class VoxSD;

  class VoxQuantityMessenger;

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

    // run lifecycle
    void InitializeAll();
    void ComputeAll();
    void ResetAll();
    void StoreAll();
    void StoreAllVTI();
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
    void SetPostfix(const G4String &postfix) { this->fPostfix = postfix; }
    const G4String &GetPostfix() const { return this->fPostfix; }
    const G4String GetFullRootPath() const { return this->fRootPath + this->fPrefix; }

    void RegisterOutputFile(const G4String &filePath) { this->fStoredFiles.push_back(filePath); }
    const std::vector<G4String> &GetStoredFiles() const { return this->fStoredFiles; }

    void ResetManager();

    // Iterate
    std::vector<std::pair<G4String, VoxRegion *>> GetAllRegionsOrdered() const;
    std::vector<std::pair<G4String, VVoxQuantity *>> GetAllQuantitiesOrdered() const;

  private:
    std::vector<G4String> fOrderedRegions;
    std::map<G4String, std::unique_ptr<VoxRegion>> fRegions;
    G4String fRootPath = ".";
    G4String fPrefix = "";
    G4String fPostfix = "";
    std::vector<G4String> fStoredFiles;

    inline static VoxQuantityManager *fInstance = nullptr;

    std::unique_ptr<VoxQuantityMessenger> fMessenger;
  };
}

#endif // VoxQuantityManager_h
