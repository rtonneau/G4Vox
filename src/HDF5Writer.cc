// VoxHDF5Writer.cc
#ifdef G4ANALYSIS_USE_HDF5

#include "G4Vox/HDF5Writer.hh"
#include "G4Vox/VoxGeometry.hh"
#include "G4Vox/VoxSD.hh"
#include "G4Vox/VoxUtils.hh"

#include <G4SystemOfUnits.hh>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <filesystem>

namespace G4Vox
{
    // ══════════════════════════════════════════════════════════════════════════════
    //  Lifecycle
    // ══════════════════════════════════════════════════════════════════════════════

    HDF5Writer::HDF5Writer(const G4String &filePath, Mode mode)
        : fFilePath(filePath), fMode(mode)
    {
        if (fVerboseLevel >= 1)
            G4cout << "[HDF5Writer] Opening " << filePath << G4endl;
        try
        {
            H5::Exception::dontPrint(); // we handle errors ourselves
            const bool fileExists = std::filesystem::exists(std::string(filePath));
            const unsigned int flag = fileExists ? H5F_ACC_RDWR : H5F_ACC_TRUNC;

            fFile = std::make_unique<H5::H5File>(
                std::string(filePath), flag);

            if (fileExists)
            {
                fMetadataWritten = fFile->nameExists("metadata");

                if (fMetadataWritten)
                {
                    H5::Group meta = fFile->openGroup("metadata");
                    if (meta.attrExists("mode"))
                    {
                        H5::Attribute modeAttr = meta.openAttribute("mode");
                        H5::StrType modeType = modeAttr.getStrType();
                        std::string storedMode;
                        modeAttr.read(modeType, storedMode);

                        const std::string requestedMode =
                            (fMode == Mode::Snapshot3D) ? "Snapshot3D" : "Extendable4D";

                        if (storedMode != requestedMode)
                        {
                            G4Exception(
                                "G4Vox::HDF5Writer::HDF5Writer",
                                "G4Vox_HDF5_ModeMismatch",
                                FatalException,
                                ("Existing HDF5 file mode mismatch. Stored mode: " + storedMode +
                                 ", requested mode: " + requestedMode)
                                    .c_str());
                        }
                    }
                }
            }
        }
        catch (const H5::FileIException &e)
        {
            G4Exception(
                "G4Vox::HDF5Writer::HDF5Writer",
                "G4Vox_HDF5_OpenFailed",
                FatalException,
                ("Cannot create/open HDF5 file: " + std::string(filePath)).c_str());
        }
    }

    HDF5Writer::~HDF5Writer()
    {
        if (fFile)
        {
            fFile->flush(H5F_SCOPE_GLOBAL); // flush pending writes
            fFile->close();                 // explicit close
            fFile.reset();                  // release unique_ptr
        }
    }

    // ══════════════════════════════════════════════════════════════════════════════
    //  Configuration
    // ══════════════════════════════════════════════════════════════════════════════

    void HDF5Writer::SetCompressionLevel(unsigned int l)
    {
        fCompressionLevel = l;
    }
    void HDF5Writer::SetVerboseLevel(int level)
    {
        fVerboseLevel = level;
    }
    bool HDF5Writer::IsOpen() const
    {
        return fFile && fFile->getId() >= 0;
    }

    // ── Init ─────────────────────────────────────────────────────────────────────
    void HDF5Writer::Init(const VoxRegion &region)
    {
        auto nVox = region.geometry->GetMaxVoxIndex().lock();
        fNX = nVox->x() + 1;
        fNY = nVox->y() + 1;
        fNZ = nVox->z() + 1;

        if (fVerboseLevel >= 2)
            G4cout << "[HDF5Writer::Init] nX=" << fNX << " nY=" << fNY << " nZ=" << fNZ << G4endl;

        if (this->fInitialized)
        {
            if (fVerboseLevel >= 1)
                G4cout << "[HDF5Writer::Init] already initialized, dimensions refreshed" << G4endl;
            return;
        }

        if (fMode == Mode::Extendable4D)
            for (const auto &qty : region.quantities)
                if (!fFile->nameExists(qty->GetName()))
                    Create4DDataset(std::string(qty->GetName()), fNX, fNY, fNZ);
        this->InitRunLog();
        this->fInitialized = true;

        // ── write geometry metadata exactly once ─────────────────────────────────
        this->WriteGeometryMetadata(region);
    }

    // ── InitRunLog ───────────────────────────────────────────────────────────────
    void HDF5Writer::InitRunLog()
    {
        if (fFile->nameExists("run_log"))
            return;

        hsize_t init[2] = {0, 4};
        hsize_t maxdims[2] = {H5S_UNLIMITED, 4};
        hsize_t chunk[2] = {1, 4};

        H5::DataSpace space(2, init, maxdims);
        H5::DSetCreatPropList pl;
        pl.setChunk(2, chunk);

        fFile->createDataSet("run_log",
                             H5Tcopy(H5T_NATIVE_DOUBLE),
                             space, pl);

        // human-readable column names as attribute
        H5::DataSet ds = fFile->openDataSet("run_log");
        H5::DataSpace attr_space(H5S_SCALAR);
        H5::StrType str(H5Tcopy(H5T_C_S1));
        str.setSize(H5T_VARIABLE);
        std::string cols = "unix_timestamp, primaries, runtime_s, subrun_id";
        H5::Attribute a = ds.createAttribute("columns", str, attr_space);
        a.write(str, cols);
    }

    // ── AppendRunLog ─────────────────────────────────────────────────────────────
    void HDF5Writer::AppendRunLog(const RunLogEntry &entry)
    {
        H5::DataSet ds = fFile->openDataSet("run_log");

        // current size
        H5::DataSpace fspace = ds.getSpace();
        hsize_t cur[2];
        fspace.getSimpleExtentDims(cur);

        // extend by one row
        hsize_t newdims[2] = {cur[0] + 1, 4};
        ds.extend(newdims);

        // select the new row as hyperslab
        fspace = ds.getSpace();
        hsize_t start[2] = {cur[0], 0};
        hsize_t count[2] = {1, 4};
        fspace.selectHyperslab(H5S_SELECT_SET, count, start);

        // pack row into a flat array (all cast to double for simplicity)
        double row[4] = {
            entry.unix_timestamp,
            static_cast<double>(entry.primaries),
            entry.runtime_s,
            static_cast<double>(entry.subrun_id)};

        H5::DataSpace mspace(2, count);
        // H5::DataType dt(H5Tcopy(H5T_NATIVE_DOUBLE));
        ds.write(row, H5Tcopy(H5T_NATIVE_DOUBLE), mspace, fspace);
    }

    // ── Write3DDataset ────────────────────────────────────────────────────────────
    void HDF5Writer::Write3DDataset(H5::Group &group,
                                    const std::string &name,
                                    const G4double *data,
                                    hsize_t nX, hsize_t nY, hsize_t nZ)
    {
        if (group.nameExists(name))
        {
            G4Exception(
                "G4Vox::HDF5Writer::Write3DDataset",
                "G4Vox_HDF5_DatasetExists",
                FatalException,
                ("Dataset already exists in group: " + name).c_str());
            return;
        }

        hsize_t dims[3] = {nZ, nY, nX};
        H5::DataSpace space(3, dims);
        H5::DSetCreatPropList props = MakeCompressedProps(nX, nY, nZ);
        H5::DataSet ds = group.createDataSet(name,
                                             H5Tcopy(H5T_NATIVE_DOUBLE),
                                             space, props);
        ds.write(data, H5Tcopy(H5T_NATIVE_DOUBLE));
    }

    // ── Create4DDataset ───────────────────────────────────────────────────────────
    void HDF5Writer::Create4DDataset(const std::string &name,
                                     hsize_t nX, hsize_t nY, hsize_t nZ)
    {
        hsize_t dims[4] = {0, nZ, nY, nX};
        hsize_t maxdims[4] = {H5S_UNLIMITED, nZ, nY, nX};
        hsize_t chunks[4] = {1, nZ, nY, nX};

        H5::DSetCreatPropList props;
        props.setChunk(4, chunks);
        props.setShuffle();
        props.setDeflate(6);

        H5::DataSpace space(4, dims, maxdims);
        H5::DataType dtype(H5Tcopy(H5T_NATIVE_DOUBLE));
        fFile->createDataSet(name,
                             dtype,
                             space, props);
    }

    // ── Append4DSlice ─────────────────────────────────────────────────────────────
    void HDF5Writer::Append4DSlice(const std::string &name,
                                   const G4double *data,
                                   hsize_t nX, hsize_t nY, hsize_t nZ)
    {
        H5::DataSet ds = fFile->openDataSet(name);

        // current number of slices
        hsize_t cur[4];
        ds.getSpace().getSimpleExtentDims(cur);

        hsize_t newDims[4] = {cur[0] + 1, nZ, nY, nX};
        ds.extend(newDims);

        hsize_t start[4] = {cur[0], 0, 0, 0};
        hsize_t count[4] = {1, nZ, nY, nX};

        H5::DataSpace fspace = ds.getSpace();
        fspace.selectHyperslab(H5S_SELECT_SET, count, start);

        H5::DataSpace mspace(4, count);
        ds.write(data, H5Tcopy(H5T_NATIVE_DOUBLE), mspace, fspace);
    }

    // ── WriteSnapshot3D ───────────────────────────────────────────────────────────
    //
    //  /subrun_0001/
    //      dose   [nZ][nY][nX]
    //      edep   [nZ][nY][nX]
    //      ...

    void HDF5Writer::WriteSnapshot3D(int subrunId, const VoxRegion &region)
    {
        std::ostringstream oss;
        oss << "subrun_" << std::setw(4) << std::setfill('0') << subrunId;
        if (fVerboseLevel >= 2)
            G4cout << "[HDF5Writer::WriteSnapshot3D] writing group " << oss.str() << G4endl;
        H5::Group g = this->CreateOrOpenGroup(oss.str());

        for (const auto &qty : region.quantities)
            Write3DDataset(g,
                           qty->GetName(),
                           &qty->GetData()[0], // valarray guarantees contiguous
                           fNX, fNY, fNZ);
    }

    // ── WriteExtendable4D ─────────────────────────────────────────────────────────
    //
    //  /dose   [subrun, nZ, nY, nX]   grows along axis 0
    //  /edep   [subrun, nZ, nY, nX]
    //  ...

    void HDF5Writer::WriteExtendable4D(const VoxRegion &region)
    {
        for (const auto &qty : region.quantities)
        {
            const std::string &name = qty->GetName();

            // Create the dataset the first time we see this quantity
            if (!fFile->nameExists(name))
                Create4DDataset(name, fNX, fNY, fNZ);

            Append4DSlice(name,
                          &qty->GetData()[0], // valarray guarantees contiguous
                          fNX, fNY, fNZ);
        }
    }

    // ══════════════════════════════════════════════════════════════════════════════
    //  Geometry metadata  →  written to /metadata group
    //
    //  /metadata
    //    attr: dims_xyz    [3]   voxel counts
    //    attr: spacing_mm  [3]   voxel size
    //    attr: origin_mm   [3]   world origin
    // ══════════════════════════════════════════════════════════════════════════════

    void HDF5Writer::WriteGeometryMetadata(const VoxRegion &region)
    {
        if (fMetadataWritten)
            return;

        H5::Group meta = this->CreateOrOpenGroup("metadata");

        auto nVox = region.geometry->GetMaxVoxIndex().lock();
        auto voxSize = region.geometry->GetVoxSize();
        auto origin = region.geometry->GetOrigin();

        WriteAttributeArray(meta, "dims_xyz",
                            {static_cast<G4double>(nVox->x() + 1),
                             static_cast<G4double>(nVox->y() + 1),
                             static_cast<G4double>(nVox->z() + 1)});

        WriteAttributeArray(meta, "spacing_mm",
                            {voxSize.x() / CLHEP::mm,
                             voxSize.y() / CLHEP::mm,
                             voxSize.z() / CLHEP::mm});

        WriteAttributeArray(meta, "origin_mm",
                            {origin.x() / CLHEP::mm,
                             origin.y() / CLHEP::mm,
                             origin.z() / CLHEP::mm});

        // ── writer mode (Snapshot3D / Extendable4D) ───────────────────────────
        {
            const std::string modeStr =
                (fMode == Mode::Snapshot3D) ? "Snapshot3D" : "Extendable4D";
            H5::DataSpace scalar(H5S_SCALAR);
            H5::StrType strType(H5::PredType::C_S1, H5T_VARIABLE);
            H5::Attribute attr = meta.createAttribute("mode", strType, scalar);
            attr.write(strType, modeStr);
        }

        // ── quantity names  (comma-separated string attribute) ───────────────────
        {
            std::string qtyList;
            for (std::size_t i = 0; i < region.Size(); ++i)
            {
                if (i > 0)
                    qtyList += ",";
                qtyList += std::string(region.quantities[i]->GetName());
            }
            H5::DataSpace scalar(H5S_SCALAR);
            H5::StrType strType(H5::PredType::C_S1, H5T_VARIABLE);
            H5::Attribute attr = meta.createAttribute("quantities", strType, scalar);
            attr.write(strType, qtyList);
        }

        this->fMetadataWritten = true;
    }

    void HDF5Writer::Export(int subrunId, int64_t primaries,
                            double runtimeSec, const VoxRegion &region)
    {
        if (fVerboseLevel >= 1)
            G4cout << "[HDF5Writer::Export] subrunId=" << subrunId
                   << " primaries=" << primaries
                   << " runtimeSec=" << runtimeSec << G4endl;

        if (!this->fInitialized)
        {
            G4Exception(
                "G4Vox::HDF5Writer::Export",
                "G4Vox_HDF5_NotInitialized",
                FatalException,
                "Export called before Init.");
            return;
        }

        // physics data
        if (this->fMode == Mode::Snapshot3D)
            this->WriteSnapshot3D(subrunId, region);
        else
            this->WriteExtendable4D(region);

        // log row
        RunLogEntry e;
        e.unix_timestamp = static_cast<double>(
            std::chrono::system_clock::to_time_t(
                std::chrono::system_clock::now()));
        e.primaries = primaries;
        e.runtime_s = runtimeSec;
        e.subrun_id = subrunId;
        this->AppendRunLog(e);

        this->fFile->flush(H5F_SCOPE_GLOBAL);
    }

    // ══════════════════════════════════════════════════════════════════════════════
    //  Finalize  →  /final  group with sum of all batches
    // ══════════════════════════════════════════════════════════════════════════════

    void HDF5Writer::Finalize()
    {
        fFile->flush(H5F_SCOPE_GLOBAL);
    }

    // ══════════════════════════════════════════════════════════════════════════════
    //  Root scalar attributes  (run-level metadata)
    // ══════════════════════════════════════════════════════════════════════════════

    void HDF5Writer::WriteRootAttribute(const std::string &name, G4double value)
    {
        H5::Group root = fFile->openGroup("/");
        WriteAttribute(root, name, value, H5Tcopy(H5T_NATIVE_DOUBLE));
    }
    void HDF5Writer::WriteRootAttribute(const std::string &name, G4int value)
    {
        H5::Group root = fFile->openGroup("/");
        WriteAttribute(root, name, value, H5Tcopy(H5T_NATIVE_INT));
    }
    void HDF5Writer::WriteRootAttribute(const std::string &name,
                                        const std::string &value)
    {
        H5::Group root = fFile->openGroup("/");
        H5::StrType stype(H5Tcopy(H5T_C_S1));
        stype.setSize(H5T_VARIABLE);
        H5::DataSpace scalar(H5S_SCALAR);
        H5::Attribute attr = root.createAttribute(name, stype, scalar);
        attr.write(stype, value);
    }

    // ══════════════════════════════════════════════════════════════════════════════
    //  Private helpers
    // ══════════════════════════════════════════════════════════════════════════════

    H5::Group HDF5Writer::CreateOrOpenGroup(const std::string &path)
    {
        if (fFile->nameExists(path))
            return fFile->openGroup(path);
        return fFile->createGroup(path);
    }

    // ── Compression properties ────────────────────────────────────────────────────
    H5::DSetCreatPropList HDF5Writer::MakeCompressedProps(hsize_t nX,
                                                          hsize_t nY,
                                                          hsize_t nZ) const
    {
        hid_t plistId = H5Pcreate(H5P_DATASET_CREATE);
        hsize_t chunk[3] = {1, nY, nX}; // one Z-slice per chunk
        H5Pset_chunk(plistId, 3, chunk);
        H5Pset_shuffle(plistId);
        H5Pset_deflate(plistId, fCompressionLevel);
        return H5::DSetCreatPropList(plistId);
    }

    // ── Scalar attribute ──────────────────────────────────────────────────────────
    template <typename T>
    void HDF5Writer::WriteAttribute(H5::H5Object &obj,
                                    const std::string &name,
                                    T value,
                                    const H5::DataType &dtype)
    {
        H5::DataSpace scalar(H5S_SCALAR);
        H5::Attribute attr = obj.createAttribute(name, dtype, scalar);
        attr.write(dtype, &value);
    }

    // ── Array attribute (e.g. dims, spacing, origin) ─────────────────────────────
    void HDF5Writer::WriteAttributeArray(H5::H5Object &obj,
                                         const std::string &name,
                                         const std::vector<G4double> &values)
    {
        hsize_t len = values.size();
        H5::DataSpace space(1, &len);
        H5::Attribute attr = obj.createAttribute(
            name, H5Tcopy(H5T_NATIVE_DOUBLE), space);
        attr.write(H5Tcopy(H5T_NATIVE_DOUBLE), values.data());
    }

} // namespace G4Vox

#endif // G4ANALYSIS_USE_HDF5
