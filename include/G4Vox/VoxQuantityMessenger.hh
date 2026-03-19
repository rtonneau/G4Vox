#pragma once

#include "G4UImessenger.hh"
#include "G4UIdirectory.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithoutParameter.hh"

namespace G4Vox
{
    class VoxQuantityManager;

    class VoxQuantityMessenger : public G4UImessenger
    {
    public:
        explicit VoxQuantityMessenger(VoxQuantityManager *manager);
        ~VoxQuantityMessenger() override;

        // Called by Geant4 UI machinery when a command is issued
        void SetNewValue(G4UIcommand *command, G4String newValue) override;

    private:
        VoxQuantityManager *fManager = nullptr; // non-owning

        // UI tree:  /voxmgr/
        //               setRootPath  <string>
        //               setPrefix   <string>
        //               resetManager
        std::unique_ptr<G4UIdirectory> fDirectory;
        std::unique_ptr<G4UIcmdWithAString> fSetRootPathCmd;
        std::unique_ptr<G4UIcmdWithAString> fSetPrefixCmd;
        std::unique_ptr<G4UIcmdWithoutParameter> fResetManagerCmd;
    };
} // namespace G4Vox