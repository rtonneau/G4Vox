#ifndef VoxSD_hh
#define VoxSD_hh 1

#include "G4VSensitiveDetector.hh"

#include "G4Vox/VVoxQuantity.hh"
#include "G4Vox/VVoxQuantityAccumulable.hh"

namespace G4Vox
{
    class VoxGeometry;

    struct VoxRegion
    {
        VoxGeometry *geometry = nullptr; // non-owning
        std::vector<G4String> orderedNames;
        std::vector<std::unique_ptr<VVoxQuantity>> quantities;
        std::vector<std::unique_ptr<VVoxQuantityAccumulable>> accumulables;
        std::vector<G4String> fStoredFiles;

        void Register(std::unique_ptr<VVoxQuantity> q);
        void InitializeAll()
        {
            for (auto &q : this->quantities)
            {
                q->InitializeQuantity();
                q->ClearStoredFiles();
            }
        }
        void ComputeAll()
        {
            for (auto &q : this->quantities)
                q->Compute();
        }
        void StoreAll(G4String path)
        {
            for (auto &q : this->quantities)
                q->Store(path);
        }
        void ResetAll()
        {
            for (auto &q : this->quantities)
                q->Reset();
        }

        void RegisterOutputFile(const G4String &filePath) { this->fStoredFiles.push_back(filePath); }
        const std::vector<G4String> &GetStoredFiles() const { return this->fStoredFiles; }

        void ExportToVTI(const G4String &filePath);
        size_t Size() const { return this->quantities.size(); }
        std::string Print(const std::string &indent = "") const;
    };

    class VoxSD : public G4VSensitiveDetector
    {
    public:
        VoxSD(const G4String &name, VoxRegion *region)
            : G4VSensitiveDetector(name), fRegion(region) // non-owning, lifetime guaranteed
        {
        }

        ~VoxSD() override = default;

        // Methods from base class
        G4bool ProcessHits(G4Step *aStep, G4TouchableHistory *) override;

    private:
        VoxRegion *fRegion = nullptr; // for quick access during Hits processing.
    };
}
#endif