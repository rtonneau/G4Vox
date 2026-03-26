#ifndef HDF5Writer_hh
#define HDF5Writer_hh 1

#ifdef G4ANALYSIS_USE_HDF5

#include "G4String.hh"
#include <H5Cpp.h>
#include <vector>
#include <memory>

namespace G4Vox
{
    class VoxGeometry;
    struct VoxRegion;

    struct RunLogEntry
    {
        double unix_timestamp; // seconds since epoch
        double runtime_s;      // wall-clock seconds for this run
        int64_t primaries;     // number of primaries in this run
        int32_t subrun_id;
    };

    class HDF5Writer
    {
    public:
        enum class Mode
        {
            Snapshot3D,
            Extendable4D
        };

        // ── Lifecycle ────────────────────────────────────────────────────────────
        explicit HDF5Writer(const G4String &filepath,
                            Mode mode = Mode::Snapshot3D);

        ~HDF5Writer(); // closes HDF5 file
        bool IsOpen() const;
        void Close();

        // non-copyable, movable
        HDF5Writer(const HDF5Writer &) = delete;
        HDF5Writer &operator=(const HDF5Writer &) = delete;
        HDF5Writer(HDF5Writer &&) = default;

        // ── Configuration (call before first AppendBatch) ────────────────────────
        void SetCompressionLevel(unsigned int level); // 0-9, default 6
        void SetVerboseLevel(int level);              // 0=silent, 1=info, 2=debug
        /// Must be called once before AppendBatch (4D mode: creates extendable ds)
        void Init(const VoxRegion &region);

        // ── Geometry metadata — written to /metadata once ────────────────────────
        void WriteGeometryMetadata(const VoxRegion &region);

        // ── Export per subrun ────────────────────────
        // Call after each Geant4 Run
        void Export(int subrunId, int64_t primaries, double runtimeSec, const VoxRegion &region);
        void WriteSnapshot3D(int subrunId, const VoxRegion &region);
        void WriteExtendable4D(const VoxRegion &region);

        // ── Convenience: write a single named scalar attribute on root ───────────
        void WriteRootAttribute(const std::string &name, G4double value);
        void WriteRootAttribute(const std::string &name, G4int value);
        void WriteRootAttribute(const std::string &name, const std::string &value);

        // ── Finalize: compute + write /final group (sum across all batches) ──────
        //    Optional — skip if you only want per-batch snapshots
        void Finalize();

        // ── Introspection ────────────────────────────────────────────────────────
        const G4String &GetFilePath() const { return fFilePath; }
        Mode GetMode() const { return fMode; }

    private:
        void InitRunLog();
        void AppendRunLog(const RunLogEntry &entry);
        // ── 3D helpers ────────────────────────────────────────────────────────────
        void Write3DDataset(H5::Group &group,
                            const std::string &name,
                            const G4double *data,
                            hsize_t nX, hsize_t nY, hsize_t nZ);

        // ── 4D helpers ────────────────────────────────────────────────────────────
        void Create4DDataset(const std::string &name,
                             hsize_t nX, hsize_t nY, hsize_t nZ);

        void Append4DSlice(const std::string &name,
                           const G4double *data,
                           hsize_t nX, hsize_t nY, hsize_t nZ);

        // ── shared helpers ────────────────────────────────────────────────────────
        H5::Group CreateOrOpenGroup(const std::string &name);
        H5::DSetCreatPropList MakeCompressedProps(hsize_t nX,
                                                  hsize_t nY,
                                                  hsize_t nZ) const;
        template <typename T>
        void WriteAttribute(H5::H5Object &obj,
                            const std::string &name,
                            T value,
                            const H5::DataType &dtype);

        static void WriteAttributeArray(H5::H5Object &obj,
                                        const std::string &name,
                                        const std::vector<G4double> &values);
        // ── Members ──────────────────────────────────────────────────────────────
        G4bool fInitialized = false;
        bool fMetadataWritten = false; // guard: write only once
        G4String fFilePath;
        std::unique_ptr<H5::H5File> fFile; // owns the open file
        Mode fMode;
        unsigned int fCompressionLevel = 6;
        int fVerboseLevel = 0;
        hsize_t fNX = 0, fNY = 0, fNZ = 0; // cached after Init()
    }; // End of class HDF5Writer

} // End of namespace G4Vox

#endif // G4ANALYSIS_USE_HDF5

#endif
