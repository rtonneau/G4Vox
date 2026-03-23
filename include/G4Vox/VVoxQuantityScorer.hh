#ifndef VVoxQuantityScorer_HH
#define VVoxQuantityScorer_HH 1

#include "G4Vox/VoxGeometry.hh"
#include "G4Vox/VoxUtils.hh"

#include "G4THitsMap.hh"
#include "G4VPrimitiveScorer.hh"

#include <set>

namespace G4Vox
{
    /** Base class for voxel scorers. */
    class VVoxQuantityScorer : public G4VPrimitiveScorer
    {
    public:
        explicit VVoxQuantityScorer(const G4String &name) : G4VPrimitiveScorer(name) {}

        virtual ~VVoxQuantityScorer() = default;

    protected:
        virtual G4bool ProcessHits(G4Step *aStep, G4TouchableHistory *) = 0;

    public:
        virtual void Initialize(G4HCofThisEvent *);
        virtual void EndOfEvent(G4HCofThisEvent *);
        virtual void DrawAll();
        virtual void PrintAll();
        /** Method used in multithreading mode in order to merge
            the results*/
        virtual void AbsorbResultsFromWorkerScorer(G4VPrimitiveScorer *);
        virtual void OutputAndClear();

        // Voxel Related Methods
    public:
        /** Return the set of quantities this scorer can fill. */
        std::set<G4String> GetQuantities() const
        {
            std::set<G4String> q;
            for (const auto &[name, _] : fEvtMaps)
                q.insert(name);
            return q;
        }

        void LinkGeometry(const VoxGeometry *geometry)
        {
            this->fLinkedGeometry = geometry;
        }
        const VoxGeometry *GetGeometry() const { return this->fLinkedGeometry; }
        size_t TotalVoxels() const
        {
            return this->fLinkedGeometry ? this->fLinkedGeometry->TotalVoxels() : 0;
        }
        const G4int GetNumberOfQuantities() const { return this->fEvtMaps.size(); }
        void RegisterOutputFile(const G4String &filePath) { this->fStoredFiles.push_back(filePath); }
        const std::vector<G4String> &GetStoredFiles() const { return this->fStoredFiles; }
        void ClearStoredFiles() { this->fStoredFiles.clear(); }

        virtual void StoreVTI(G4String path = ".");

        std::valarray<G4double> HitMapToValarray(const G4String &name) const;
        std::map<G4String, std::valarray<G4double>> AllHitsMapsToValarrays() const;

    protected:
        void RegisterQuantity(const G4String &name)
        {
            this->fEvtMaps[name] = nullptr; // ptr allocated later in Initialize()
        }

    protected:
        G4int fHCID;
        std::map<G4String, G4THitsMap<G4double> *> fEvtMaps;

        const VoxGeometry *fLinkedGeometry = nullptr; // for easy access to geometry info in Compute()
        std::vector<G4String> fStoredFiles;
    };
} // namespace G4Vox

#endif // VVoxQuantityScorer_HH