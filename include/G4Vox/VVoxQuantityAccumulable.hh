#ifndef VVoxQuantityAccumulable_H
#define VVoxQuantityAccumulable_H 1

#include "G4VAccumulable.hh"

#include "G4Vox/VoxStepFilter.hh"
#include "G4Vox/VoxUtils.hh"

#include <memory>
#include <vector>

class G4Step;

namespace G4Vox
{
    class VVoxQuantity;

    class VVoxQuantityAccumulable : public G4VAccumulable
    {
        friend class VVoxQuantity; // for data access

    public:
        explicit VVoxQuantityAccumulable(const G4String &name, std::weak_ptr<const VoxelIndex> maxVoxIndex)
            : G4VAccumulable(name), fMaxVoxIndex(maxVoxIndex)
        {
        }

        virtual ~VVoxQuantityAccumulable() = default;

        //------------------------------------------------------------------
        // Scoring — called from SD on worker thread
        //------------------------------------------------------------------
        virtual void Score(const G4Step *step)
        {
            if (step == nullptr || !this->PassesFilters(step))
                return;
            this->ScoreImpl(step);
        }

        void AddStepFilter(const std::shared_ptr<const VVoxStepFilter> &filter)
        {
            if (filter)
                this->fStepFilters.push_back(filter);
        }

        void SetStepFilters(const std::vector<std::shared_ptr<const VVoxStepFilter>> &filters)
        {
            this->fStepFilters.clear();
            for (const auto &filter : filters)
                this->AddStepFilter(filter);
        }

        void ClearStepFilters() { this->fStepFilters.clear(); }

        const std::vector<std::shared_ptr<const VVoxStepFilter>> &GetStepFilters() const
        {
            return this->fStepFilters;
        }

        //------------------------------------------------------------------
        // G4VAccumulable interface
        //------------------------------------------------------------------

        /** Default Merge: element-wise addition of fData arrays. */
        virtual void Merge(const G4VAccumulable &other) override
        {
            const auto &o = static_cast<const VVoxQuantityAccumulable &>(other);
            this->fData += o.fData; // valarray operator+= is element-wise ✓
        }

        /** Default Reset: zero all bins. */
        virtual void Reset() override
        {
            if (this->fInitialized)
            {
                this->fData = 0.; // valarray broadcast assign ✓
            }
            else
            {
                this->Initialize();
            }
        }

        inline size_t TotalVoxels() const { return this->fTotalVoxels; }

        virtual size_t FlattenVoxelIndex(const VoxelIndex &v) const = 0;

        //------------------------------------------------------------------
        // Data access
        //------------------------------------------------------------------
        virtual const G4Vox::array_type &GetData() const
        {
            return this->fData;
        }
        virtual G4double GetData(std::size_t i) const { return this->fData[i]; }

    protected:
        virtual void ScoreImpl(const G4Step *) = 0;

        G4bool PassesFilters(const G4Step *step) const
        {
            for (const auto &filter : this->fStepFilters)
            {
                if (filter && !filter->Accept(step))
                    return false;
            }
            return true;
        }

        virtual void Initialize()
        {
            auto maxVox = this->fMaxVoxIndex.lock();
            this->fNx = maxVox->x() + 1;
            this->fNy = maxVox->y() + 1;
            this->fNz = maxVox->z() + 1;
            this->fTotalVoxels = this->fNx * this->fNy * this->fNz;
            this->fData = array_type(0.0, this->TotalVoxels());
            this->fInitialized = true;
        }

    protected:
        G4Vox::array_type fData; // ← default store, subclass fills it
        G4bool fInitialized = false;
        G4int fNx = 0;
        G4int fNy = 0;
        G4int fNz = 0;
        G4int fTotalVoxels = 0;
        std::weak_ptr<const VoxelIndex> fMaxVoxIndex;
        std::vector<std::shared_ptr<const VVoxStepFilter>> fStepFilters;
    };
}
#endif
