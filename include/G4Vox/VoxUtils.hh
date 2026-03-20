#ifndef VoxUtils_H
#define VoxUtils_H 1

#include "G4SystemOfUnits.hh"
#include "G4Types.hh"
#include "G4String.hh"
#include "G4TouchableHandle.hh"

namespace G4
{
    // --- Units: import everything from CLHEP ---
    using namespace CLHEP;

    // --- Types: explicit aliases ---
    using G4double = ::G4double;
    using G4bool = ::G4bool;
    using G4int = ::G4int;
    using G4long = ::G4long;
    using G4String = ::G4String;

    // G4double GetEnergyDeposit(const G4Step *step);

}

#include <valarray>
#include <array>

namespace G4Vox
{
    /// Returns (converted_value, unit_string) for a G4 internal-unit distance.
    /// Chooses µm below 1000 µm, mm otherwise.
    inline std::pair<double, std::string> BestLengthUnit(double valG4)
    {
        double valMm = valG4 / CLHEP::mm;
        if (valMm < 1.)
            return {valG4 / CLHEP::um, "um"};
        else
            return {valMm, "mm"};
    }

    // Alias for matrix type
    using array_type = std::valarray<G4::G4double>;

    struct VoxelIndex
    {
        std::array<int, 3> fIndex = {-1, -1, -1};

        VoxelIndex(int i, int j, int k) : fIndex{i, j, k} {}

        virtual ~VoxelIndex() = default;

        virtual int Flatten(int nI, int nJ) const = 0;

        // Default implementation — no need to override in derived classes
        virtual bool operator==(const VoxelIndex &other) const
        {
            return this->fIndex == other.fIndex; // ✅ array == array → single bool
        }

        G4int x() const { return this->fIndex[0]; }
        G4int y() const { return this->fIndex[1]; }
        G4int z() const { return this->fIndex[2]; }

        void SetX(int x) { this->fIndex[0] = x; }
        void SetY(int y) { this->fIndex[1] = y; }
        void SetZ(int z) { this->fIndex[2] = z; }
    };
    std::ostream &operator<<(std::ostream &os, const VoxelIndex &v);

    struct CartesianVoxelIndex : public VoxelIndex
    {
        using VoxelIndex::VoxelIndex; // Inherit constructor

        int Flatten(int nI, int nJ) const override
        {
            return this->fIndex[0] + nI * (this->fIndex[1] + nJ * this->fIndex[2]);
        }
        static std::unique_ptr<VoxelIndex> FromTouchableHandle(const G4TouchableHandle &th)
        {
            return std::make_unique<CartesianVoxelIndex>(
                th->GetReplicaNumber(2),
                th->GetReplicaNumber(1),
                th->GetReplicaNumber(0));
        }

        static size_t FlattenIndexes(int i, int j, int k, int nI, int nJ)
        {
            return i + nI * (j + nJ * k);
        }
        // In CartesianVoxelIndex — static, no allocation, no virtual = fast!
        static std::size_t FlattenTouchable(const G4VTouchable *th, int nX, int nY)
        {
            return th->GetReplicaNumber(2) + nX * (th->GetReplicaNumber(1) + nY * th->GetReplicaNumber(0));
        }
    };

    namespace PathUtils
    {
        G4String EnsureTrailingSlash(const G4String &path);
        G4bool CreateDirectoryIfNotExists(const G4String &path, G4int verboseLevel = 0);
        G4bool IsDirectory(const G4String &path);
        G4bool IsFile(const G4String &path);
        G4bool Exists(const G4String &path);
    }

}
#endif