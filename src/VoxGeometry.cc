#include "G4Vox/VoxGeometry.hh"

#include "G4VPhysicalVolume.hh"
#include "G4LogicalVolume.hh"
#include "G4Box.hh"
#include "G4Material.hh"
#include "G4Exception.hh"
#include "G4PVReplica.hh"
#include "G4SDManager.hh"
#include "G4VisAttributes.hh"

namespace G4Vox
{
    VoxGeometry::VoxGeometry(G4VPhysicalVolume *motherVol, G4ThreeVector voxSize)
    {
        this->SetMotherVolume(motherVol);
        this->InitFromVoxSize(voxSize);
    }

    VoxGeometry::VoxGeometry(G4VPhysicalVolume *motherVol, G4int nx, G4int ny, G4int nz)
    {
        this->SetMotherVolume(motherVol);
        this->InitFromVoxNumber(nx, ny, nz);
    }

    VoxGeometry::~VoxGeometry() {}

    void VoxGeometry::SetMotherVolume(G4VPhysicalVolume *motherVol)
    {
        G4LogicalVolume *logicalVol = motherVol->GetLogicalVolume();
        G4Box *box = dynamic_cast<G4Box *>(logicalVol->GetSolid());
        if (!box)
        {
            G4Exception("VoxGeometry::SetMotherVolume", "NotG4Box", FatalException, "Mother volume is not a G4Box!");
        }
        this->fMotherSize = G4ThreeVector(box->GetXHalfLength() * 2, box->GetYHalfLength() * 2, box->GetZHalfLength() * 2);
        this->fVoxOrigin = motherVol->GetTranslation() - this->fMotherSize / 2.0;
        this->fMotherVolume = motherVol;
    }

    void VoxGeometry::AdjustVoxNumberToOdd()
    {
        if (this->verboseLevel > 0)
            G4cout << "**** VoxGeometry::AdjustVoxNumberToOdd -> Start" << G4endl;

        if ((this->fMaxVoxIndex->y() + 1) % 2 == 0)
        {
            G4Exception("VoxGeometry::AdjustVoxNumberToOdd", "VoxNumberYEven", JustWarning,
                        "Trying to voxelize with an even number of voxels along the Y axis."
                        "Adjusting the number of voxels along Y axis to be odd to prevent "
                        "from warnings due to tracking (+1 voxel).");
            if (this->fMaxVoxIndex->x() == this->fMaxVoxIndex->y())
            {
                G4Exception("VoxGeometry::AdjustVoxNumberToOdd", "VoxNumberXEqualY", JustWarning,
                            "Vox number along X updated to match the number of voxels along Y axis");
                this->fMaxVoxIndex->fIndex[0] += 1;
            }
            this->fMaxVoxIndex->fIndex[1] += 1;
        }
        if ((this->fMaxVoxIndex->z() + 1) % 2 == 0)
        {
            G4Exception("VoxGeometry::AdjustVoxNumberToOdd", "VoxNumberZEven", JustWarning,
                        "Trying to voxelize with an even number of voxels along the Z axis."
                        "Adjusting the number of voxels along Z axis to be odd to prevent "
                        "from warnings due to tracking (+1 voxel).");
            this->fMaxVoxIndex->fIndex[2] += 1;
        }
        if (this->verboseLevel > 0)
            G4cout << "**** VoxGeometry::AdjustVoxNumberToOdd -> End" << G4endl;
    }

    void VoxGeometry::InitFromVoxNumber(G4int nx, G4int ny, G4int nz)
    {
        if (this->verboseLevel > 0)
            G4cout << "**** VoxGeometry::InitFromVoxNumber -> Start" << G4endl;

        this->fMaxVoxIndex = std::make_unique<CartesianVoxelIndex>(nx - 1, ny - 1, nz - 1);
        this->AdjustVoxNumberToOdd();
        this->fVoxSize = G4ThreeVector(this->fMotherSize.x() / (this->fMaxVoxIndex->x() + 1),
                                       this->fMotherSize.y() / (this->fMaxVoxIndex->y() + 1),
                                       this->fMotherSize.z() / (this->fMaxVoxIndex->z() + 1));
        this->UpdateVoxVolume();

        if (this->verboseLevel > 0)
        {
            G4cout << "**** VoxGeometry::InitFromVoxNumber -> End" << G4endl;
            G4cout << "**** VoxGeometry::InitFromVoxNumber -> Vox size: " << this->fVoxSize << G4endl;
        }
    }

    void VoxGeometry::InitFromVoxSize(G4ThreeVector voxSize)
    {
        if (this->verboseLevel > 0)
            G4cout << "**** VoxGeometry::InitFromVoxSize -> Start" << G4endl;

        // guard: mother must be initialised
        if (this->fMotherSize.x() <= 0)
            G4Exception("VoxGeometry::InitFromVoxSize", "MotherSizeNotSet",
                        FatalException, "Mother size not initialised before calling InitFromVoxSize.");

        // force odd: bitwise OR ("|") with 1 sets last bit, even→even+1, odd unchanged
        G4int nx = G4int(std::round(this->fMotherSize.x() / voxSize.x())) | 1;
        G4int ny = G4int(std::round(this->fMotherSize.y() / voxSize.y())) | 1;
        G4int nz = G4int(std::round(this->fMotherSize.z() / voxSize.z())) | 1;
        this->fMaxVoxIndex = std::make_unique<CartesianVoxelIndex>(nx - 1, ny - 1, nz - 1);
        // ✅ actual vox size: recompute from mother / odd n (not the requested one)
        this->fVoxSize = G4ThreeVector(this->fMotherSize.x() / nx,
                                       this->fMotherSize.y() / ny,
                                       this->fMotherSize.z() / nz);
        this->UpdateVoxVolume();

        if (this->verboseLevel > 0)
        {
            G4cout << "**** VoxGeometry::InitFromVoxSize -> Computed max voxel index: " << *(this->fMaxVoxIndex) << G4endl;
            G4cout << "**** VoxGeometry::InitFromVoxSize -> End" << G4endl;
        }
    }

    void VoxGeometry::UpdateVoxVolume()
    {
        if (this->verboseLevel > 0)
            G4cout << "**** VoxGeometry::UpdateVoxVolume -> Start" << G4endl;

        this->fVoxVolume = this->fVoxSize.x() * this->fVoxSize.y() * this->fVoxSize.z();
        this->fVoxDensity = this->GetMaterial()->GetDensity();
        this->fVoxMass = this->fVoxVolume * this->fVoxDensity;

        if (this->verboseLevel > 0)
        {
            G4cout << "**** VoxGeometry::UpdateVoxVolume -> End" << G4endl;
            G4cout << "**** VoxGeometry::UpdateVoxVolume -> Vox volume: " << this->fVoxVolume << " mm^3" << G4endl;
            G4cout << "**** VoxGeometry::UpdateVoxVolume -> Vox density: " << this->fVoxDensity / (g / cm3) << " g/cm^3" << G4endl;
            G4cout << "**** VoxGeometry::UpdateVoxVolume -> Vox mass: " << this->fVoxMass / g << " g" << G4endl;
        }
    }

    /*
       Mother
        └── PVReplica X  (nx slabs along X)
            └── PVReplica Y  (ny slabs along Y)
                └── PVReplica Z  (nz slabs along Z)  ← leaf voxel
    */
    void VoxGeometry::ConstructVoxels()
    {
        if (this->verboseLevel > 0)
            G4cout << "**** VoxGeometry::ConstructVoxels -> Start" << G4endl;
        // Along X axis
        auto &x = this->fVoxDivisions[0];
        x.solid = new G4Box("VoxBoxX", this->fVoxSize.x() / 2, this->fMotherSize.y() / 2, this->fMotherSize.z() / 2);
        x.logical = new G4LogicalVolume(x.solid, this->GetMaterial(), "VoxLogicalX", 0, 0, 0);
        x.physical = new G4PVReplica("VoxPhysicalX", x.logical, this->fMotherVolume, kXAxis, this->fMaxVoxIndex->x() + 1, this->fVoxSize.x());
        // Along Y axis
        auto &y = this->fVoxDivisions[1];
        y.solid = new G4Box("VoxBoxY", this->fVoxSize.x() / 2, this->fVoxSize.y() / 2, this->fMotherSize.z() / 2);
        y.logical = new G4LogicalVolume(y.solid, this->GetMaterial(), "VoxLogicalY", 0, 0, 0);
        y.physical = new G4PVReplica("VoxPhysicalY", y.logical, x.logical, kYAxis, this->fMaxVoxIndex->y() + 1, this->fVoxSize.y());
        // Along Z axis
        auto &z = this->fVoxDivisions[2];
        z.solid = new G4Box("VoxBoxZ", this->fVoxSize.x() / 2, this->fVoxSize.y() / 2, this->fVoxSize.z() / 2);
        z.logical = new G4LogicalVolume(z.solid, this->GetMaterial(), "VoxLogicalZ", 0, 0, 0);
        z.physical = new G4PVReplica("VoxPhysicalZ", z.logical, y.logical, kZAxis, this->fMaxVoxIndex->z() + 1, this->fVoxSize.z());

        // intermediate replica slabs: hidden (just navigation helpers)
        auto *invisible = new G4VisAttributes(false);
        fVoxDivisions[0].logical->SetVisAttributes(invisible);
        fVoxDivisions[1].logical->SetVisAttributes(invisible);

        // leaf voxels: cyan, wireframe, semi-transparent
        auto *voxVis = new G4VisAttributes(G4Colour(0.0, 0.8, 1.0, 0.3));
        voxVis->SetVisibility(true);
        voxVis->SetForceWireframe(true);
        fVoxDivisions[2].logical->SetVisAttributes(voxVis);

        // mother (outer box): orange solid, very transparent → visible envelope
        auto *motherVis = new G4VisAttributes(G4Colour(1.0, 0.4, 0.0, 0.15));
        motherVis->SetVisibility(true);
        motherVis->SetForceSolid(true);
        this->fMotherVolume->GetLogicalVolume()->SetVisAttributes(motherVis);

        if (this->verboseLevel > 0)
            G4cout << "**** VoxGeometry::ConstructVoxels -> End" << G4endl;
    }

    G4int VoxGeometry::FlattenThisVox(VoxelIndex *voxel) const
    {
        return voxel->Flatten(this->fMaxVoxIndex->x() + 1, this->fMaxVoxIndex->y() + 1);
    }

    void VoxGeometry::RegisterSD(G4VSensitiveDetector *sd)
    {
        if (this->verboseLevel > 0)
            G4cout << "**** VoxGeometry::RegisterSD -> Start" << G4endl;
        // Guard
        if (!sd)
        {
            G4Exception("VoxGeometry::RegisterSD", "NullSD", FatalException,
                        "Trying to register a sensitive detector with a null pointer.");
            G4cerr << "VoxGeometry::RegisterSD: null SD passed" << G4endl;
            return;
        }

        // Register with Geant4 SD manager
        G4SDManager::GetSDMpointer()->AddNewDetector(sd);

        // Attach to the LEAF logical volume only (Z division = finest granularity)
        this->fVoxDivisions[2].logical->SetSensitiveDetector(sd);
        if (this->verboseLevel > 0)
            G4cout << "**** VoxGeometry::RegisterSD -> End" << G4endl;
    }

    Voxel VoxGeometry::GetVoxel(G4int i, G4int j, G4int k) const
    {
        Voxel voxel;
        voxel.index = std::make_unique<CartesianVoxelIndex>(i, j, k);
        voxel.volume = this->fVoxVolume;
        voxel.density = this->fVoxDensity;
        voxel.mass = this->fVoxMass;
        voxel.flatIndex = this->FlattenThisVox(voxel.index.get());
        voxel.size = this->fVoxSize;
        return voxel;
    }

}