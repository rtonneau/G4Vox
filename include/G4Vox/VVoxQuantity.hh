#ifndef VVoxQuantity_H
#define VVoxQuantity_H 1

#include "G4AccumulableManager.hh"
#include "G4Step.hh"

#include "G4Vox/VoxGeometry.hh"
#include "G4Vox/VoxStepFilter.hh"
#include "G4Vox/VoxUtils.hh"

#include <memory>

class G4UImessenger;

namespace G4Vox
{
  class VVoxQuantityAccumulable;

  class VVoxQuantity
  {

  public:
    VVoxQuantity(const G4String &name) : fName(name) {}

    virtual ~VVoxQuantity() = default;

    //------------------------------------------------------------------
    // Factory
    //------------------------------------------------------------------
    /** Spawn a thread-local scorer pre-configured from this object's settings. */
    virtual VVoxQuantityAccumulable *UserCreateAccumulable(const G4String &name) const = 0;

    VVoxQuantityAccumulable *CreateAccumulable() const;

    //------------------------------------------------------------------
    // Cross-events accumulation — master thread only
    //------------------------------------------------------------------
    /** Absorb one thread's accumulable into the persistent store. */
    virtual void ReadAccumulable(const VVoxQuantityAccumulable &other);

    /** Called after all threads merged. e.g. edep → dose. */
    virtual void Compute() = 0;

    /** Write persistent store to disk. */
    virtual void Store(G4String path = ".") = 0;

    /** Zero the persistent store at BeginOfRun. */
    virtual void InitializeQuantity();
    virtual void Reset();

    virtual void StoreVTI(G4String path = ".");

    //------------------------------------------------------------------
    // Config
    //------------------------------------------------------------------
    void SetAccumulate(bool accumulate) { this->fAccumulate = accumulate; }
    bool GetAccumulate() const { return this->fAccumulate; }
    bool IsInitialized() const { return this->fInitialized; }
    void AddStepFilter(const std::shared_ptr<const VVoxStepFilter> &filter)
    {
      if (filter)
        this->fStepFilters.push_back(filter);
    }
    void ClearStepFilters() { this->fStepFilters.clear(); }
    const std::vector<std::shared_ptr<const VVoxStepFilter>> &GetStepFilters() const
    {
      return this->fStepFilters;
    }

    const G4String &GetName() const
    {
      return fName;
    }
    G4bool IsComputationEnabled() const { return fComputationEnabled; }
    G4bool HasBeenComputed() const { return fComputed; }
    const G4String &GetDetectorName() const { return fDetectorName; }

    void SetComputationEnabled(G4bool e) { fComputationEnabled = e; }
    void SetVerboseLevel(G4int l) { fVerboseLevel = l; }
    void SetDetectorName(const G4String &d) { this->fDetectorName = d; }
    void LinkGeometry(const VoxGeometry *geometry)
    {
      this->fLinkedGeometry = geometry;
      this->fInitialized = false;
    }
    std::weak_ptr<const VoxelIndex> GetMaxVoxIndex() const { return this->GetGeometry()->GetMaxVoxIndex(); }
    const VoxGeometry *GetGeometry() const { return this->fLinkedGeometry; }

    //------------------------------------------------------------------
    // Others
    //------------------------------------------------------------------
    const G4Vox::array_type &GetData() const { return this->fData; }

    size_t TotalVoxels() const
    {
      auto idx = this->GetMaxVoxIndex().lock(); // promotes to temporary shared_ptr
      if (!idx)
        return 0;
      return (idx->x() + 1) * (idx->y() + 1) * (idx->z() + 1);
    }

    size_t Size() const { return this->TotalVoxels(); }
    double Sum() const { return this->fData.sum(); }
    void RegisterOutputFile(const G4String &filePath) { this->fStoredFiles.push_back(filePath); }
    const std::vector<G4String> &GetStoredFiles() const { return this->fStoredFiles; }
    void ClearStoredFiles() { this->fStoredFiles.clear(); }

  protected:
    G4String fName;
    G4String fDetectorName;
    G4int fVerboseLevel = 0;
    G4bool fInitialized = false; // Whether InitializeQuantity has been called at least once
    G4bool fSaved = false;
    G4bool fComputed = false;
    G4bool fComputationEnabled = false;
    G4bool fAccumulate = false;                   // Whether to accumulate across runs, set by VoxQuantityManager
    G4Vox::array_type fData;                      // ← default store, subclass fills it
    const VoxGeometry *fLinkedGeometry = nullptr; // for easy access to geometry info in Compute()
    std::vector<G4String> fStoredFiles;
    std::vector<std::shared_ptr<const VVoxStepFilter>> fStepFilters;

    // Persistent data arrays live in the concrete subclass
    // e.g. std::valarray<G4double> fDose;
  };
}
#endif
