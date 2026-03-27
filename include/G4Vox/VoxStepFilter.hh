#ifndef VOXSTEPFILTER_H
#define VOXSTEPFILTER_H 1

#include "G4Step.hh"
#include "G4String.hh"

namespace G4Vox
{
    class VVoxStepFilter
    {
    public:
        virtual ~VVoxStepFilter() = default;
        virtual G4bool Accept(const G4Step *step) const = 0;
        virtual G4String GetName() const = 0;
    };

    class SecondaryOnlyStepFilter : public VVoxStepFilter
    {
    public:
        G4bool Accept(const G4Step *step) const override
        {
            if (step == nullptr || step->GetTrack() == nullptr)
                return false;
            return step->GetTrack()->GetParentID() > 0;
        }

        G4String GetName() const override
        {
            return "secondary-only";
        }
    };
}

#endif
