#include "G4Vox/VoxUtils.hh"

#include <filesystem>

namespace G4Vox
{
    // Printable with <<
    std::ostream &operator<<(std::ostream &os, const VoxelIndex &v)
    {
        os << "(" << v.x() << ", " << v.y() << ", " << v.z() << ")";
        return os;
    }

    namespace PathUtils
    {
        namespace fs = std::filesystem;

        /**
         * @brief Ensures a directory path ends with a trailing separator.
         *
         * Handles both Unix '/' and Windows '\\' separators.
         * Returns "./" if the input path is empty.
         *
         * @param path Input directory path
         * @return G4String Path guaranteed to end with '/'
         *
         * Examples:
         *   "/home/user/data"   --> "/home/user/data/"
         *   "/home/user/data/"  --> "/home/user/data/"
         *   "C:\\myFolder"      --> "C:\\myFolder/"
         *   ""                  --> "./"
         */
        G4String EnsureTrailingSlash(const G4String &path)
        {
            if (path.empty())
                return "./";

            const char last = path.back();

            if (last == '/' || last == '\\')
                return path;

            return path + '/';
        }

        /**
         * @brief Check if a path exists (file or directory)
         */
        G4bool Exists(const G4String &path)
        {
            return fs::exists(path.c_str());
        }

        /**
         * @brief Check if path is an existing directory
         */
        G4bool IsDirectory(const G4String &path)
        {
            return fs::is_directory(path.c_str());
        }

        /**
         * @brief Check if path is an existing regular file
         */
        G4bool IsFile(const G4String &path)
        {
            return fs::is_regular_file(path.c_str());
        }

        /**
         * @brief Create a directory only if its parent directory exists.
         *
         * Will NOT recursively create missing parents.
         *
         * Flow:
         *
         *   path already exists?
         *       │
         *   Yes─┤ IsDirectory? ──Yes──► return true
         *       │              ──No───► it's a file, return false
         *   No──┤
         *       ▼
         *   parent exists?
         *       │
         *   No──► warn + return false
         *       │
         *   Yes─┤ parent is a directory?
         *       │      │
         *       │   No─► warn + return false
         *       │      │
         *       │   Yes─► mkdir()
         *       │              │
         *       │          OK──► return true
         *       │         FAIL──► warn + return false
         */
        G4bool CreateDirectoryIfNotExists(const G4String &path, G4int verboseLevel)
        {
            const fs::path fsPath(path.c_str());

            // --- Already exists ---
            if (fs::exists(fsPath))
            {
                if (fs::is_directory(fsPath))
                {
                    if (verboseLevel >= 2)
                        G4cout << "[CreateDirectoryIfNotExists] Already exists: "
                               << fsPath << G4endl;
                    return true;
                }
                else
                {
                    G4cerr << "[CreateDirectoryIfNotExists] Path exists but is a file: "
                           << fsPath << G4endl;
                    return false;
                }
            }

            // --- Check parent ---
            const fs::path parentPath = fsPath.parent_path();

            if (!fs::exists(parentPath))
            {
                G4cerr << "[CreateDirectoryIfNotExists] Parent directory does not exist: "
                       << parentPath << G4endl;
                return false;
            }

            if (!fs::is_directory(parentPath))
            {
                G4cerr << "[CreateDirectoryIfNotExists] Parent path is not a directory: "
                       << parentPath << G4endl;
                return false;
            }

            // --- Create ---
            std::error_code ec;
            const G4bool created = fs::create_directory(fsPath, ec);

            if (created)
            {
                if (verboseLevel >= 1)
                    G4cout << "[CreateDirectoryIfNotExists] Created: "
                           << fsPath << G4endl;
                return true;
            }

            G4cerr << "[CreateDirectoryIfNotExists] Failed to create: "
                   << fsPath << " (" << ec.message() << ")" << G4endl;
            return false;
        }

    } // namespace PathUtils

} // namespace G4Vox