#include "G4Vox/VoxQuantityMessenger.hh"
#include "G4Vox/VoxQuantityManager.hh"
#include "G4Vox/VoxUtils.hh"

#include "G4UImanager.hh"

namespace G4Vox
{
    // ── constructor ──────────────────────────────────────────────────────────────
    VoxQuantityMessenger::VoxQuantityMessenger(VoxQuantityManager *manager)
        : fManager(manager)
    {
        // ── directory ────────────────────────────────────────────────────────────
        fDirectory = std::make_unique<G4UIdirectory>("/voxmgr/");
        fDirectory->SetGuidance("VoxQuantityManager controls.");

        // ── /voxmgr/setRootPath <string> ─────────────────────────────────────────
        fSetRootPathCmd = std::make_unique<G4UIcmdWithAString>(
            "/voxmgr/setRootPath", this);
        fSetRootPathCmd->SetGuidance("Set the root output path for voxel scoring.");
        fSetRootPathCmd->SetParameterName("path", /*omittable=*/false);
        fSetRootPathCmd->SetDefaultValue(".");
        // Allow change only between runs (safe moment for file-system paths)
        fSetRootPathCmd->AvailableForStates(G4State_PreInit,
                                            G4State_Idle);
        // ── /voxmgr/setPrefix <string> ───────────────────────────────────────────
        fSetPrefixCmd = std::make_unique<G4UIcmdWithAString>(
            "/voxmgr/setPrefix", this);
        fSetPrefixCmd->SetGuidance("Set the prefix for voxel related files.");
        fSetPrefixCmd->SetParameterName("prefix", /*omittable=*/false);
        fSetPrefixCmd->SetDefaultValue("");
        fSetPrefixCmd->AvailableForStates(G4State_PreInit,
                                          G4State_Idle);

        // ── /voxmgr/reset ────────────────────────────────────────────────────────
        fResetManagerCmd = std::make_unique<G4UIcmdWithoutParameter>(
            "/voxmgr/reset", this);
        fResetManagerCmd->SetGuidance("Call VoxQuantityManager::ResetManager().");
        fResetManagerCmd->AvailableForStates(G4State_Idle);

        // ── /voxmgr/setVerboseLevel <int> ───────────────────────────────────────
        fSetVerboseLevelCmd = std::make_unique<G4UIcmdWithAnInteger>(
            "/voxmgr/setVerboseLevel", this);
        fSetVerboseLevelCmd->SetGuidance("Set the verbose level for VoxQuantityManager.");
        fSetVerboseLevelCmd->SetParameterName("VerboseLevel", /*omittable=*/false);
        fSetVerboseLevelCmd->SetDefaultValue(0);
        fSetVerboseLevelCmd->AvailableForStates(G4State_PreInit,
                                                G4State_Idle);

        // ── /voxmgr/setNewSubFolder <string> ─────────────────────────────────
        fNewSubFolderCmd = std::make_unique<G4UIcmdWithAString>(
            "/voxmgr/setNewSubFolder", this);
        fNewSubFolderCmd->SetGuidance("Set a new subfolder for voxel related files.");
        fNewSubFolderCmd->SetParameterName("subFolder", /*omittable=*/false);
        fNewSubFolderCmd->SetDefaultValue("");
        fNewSubFolderCmd->AvailableForStates(G4State_PreInit,
                                             G4State_Idle);

        // ── /voxmgr/leaveSubFolder ───────────────────────────────────────────
        fLeaveSubFolderCmd = std::make_unique<G4UIcmdWithoutParameter>(
            "/voxmgr/leaveSubFolder", this);
        fLeaveSubFolderCmd->SetGuidance("Leave the current subfolder for voxel related files.");
        fLeaveSubFolderCmd->AvailableForStates(G4State_PreInit,
                                               G4State_Idle);
    }

    // ── destructor ───────────────────────────────────────────────────────────────
    VoxQuantityMessenger::~VoxQuantityMessenger() = default;

    // ── SetNewValue ──────────────────────────────────────────────────────────────
    void VoxQuantityMessenger::SetNewValue(G4UIcommand *command,
                                           G4String newValue)
    {
        if (command == fSetRootPathCmd.get())
        {
            fManager->SetRootPath(newValue);
            if (fManager->GetVerboseLevel() > 0)
                G4cout << "[VoxQuantityMessenger] RootPath set to: " << newValue << G4endl;
        }
        else if (command == fResetManagerCmd.get())
        {
            if (fManager->GetVerboseLevel() > 0)
                G4cout << "[VoxQuantityMessenger] Calling ResetManager()..." << G4endl;
            fManager->ResetManager();
        }
        else if (command == fSetPrefixCmd.get())
        {
            fManager->SetPrefix(newValue);
            if (fManager->GetVerboseLevel() > 0)
                G4cout << "[VoxQuantityMessenger] Prefix set to: " << newValue << G4endl;
        }
        else if (command == fSetVerboseLevelCmd.get())
        {
            G4int level = G4UIcmdWithAnInteger::GetNewIntValue(newValue);
            fManager->SetVerboseLevel(level);
            G4cout << "[VoxQuantityMessenger] VerboseLevel set to: " << level << G4endl;
        }
        else if (command == fNewSubFolderCmd.get())
        {
            fManager->SetSubFolder(newValue);
            if (fManager->GetVerboseLevel() > 0)
                G4cout << "[VoxQuantityMessenger] New subfolder set. Local path is now: "
                       << fManager->GetLocalPath() << G4endl;
        }
        else if (command == fLeaveSubFolderCmd.get())
        {
            fManager->SetSubFolder("");
            if (fManager->GetVerboseLevel() > 0)
                G4cout << "[VoxQuantityMessenger] Left subfolder. Local path is now: "
                       << fManager->GetLocalPath() << G4endl;
        }
        else
        {
            G4Exception("VoxQuantityMessenger::SetNewValue",
                        "VOXMGR_003",
                        JustWarning,
                        ("Unknown command: " + command->GetCommandPath()).c_str());
        }
    }
} // namespace G4Vox