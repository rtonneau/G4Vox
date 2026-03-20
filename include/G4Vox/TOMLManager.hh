#ifndef TOML_MANAGER_HH
#define TOML_MANAGER_HH

#include <toml++/toml.hpp>
#include <string>

#include "G4String.hh"

namespace G4Vox
{
    class TOMLManager
    {
    public:
        static TOMLManager *GetInstance();

        // ── path ──────────────────────────────────────────────────
        void SetRootPath(const G4String &path);
        G4String GetRootPath() const { return fRootPath; }

        // ── clean ─────────────────────────────────────────────────
        void CleanTOMLData();

        // ── write to tree ─────────────────────────────────────────
        // Set a scalar:   Set("run_manager.num_events", 10000)
        template <typename T>
        void Set(const std::string &dotPath, T &&value)
        {
            std::string lastKey;
            toml::table &parent = ResolvePath(dotPath, lastKey);
            parent.insert_or_assign(lastKey, std::forward<T>(value));
        }

        // Append to array of tables:  used for [[regions]]
        toml::table &AppendTable(const std::string &arrayPath);

        // Direct access for complex structures
        toml::table &GetRoot() { return fDocument; }

        void AppendTable(const std::string &arrayKey, toml::table &&table);
        void AppendTable(const std::string &parentKey, const std::string &arrayKey, toml::table &&table);
        // ── output ────────────────────────────────────────────────
        void Write(const G4String &filename = "Geant4_manifest.toml");

    private:
        TOMLManager() = default;
        static TOMLManager *fInstance;

        G4String fRootPath;
        toml::table fDocument;

        // resolve "a.b.c" → nested node
        toml::table &ResolvePath(const std::string &dotPath, std::string &lastKey);
    };
} // namespace G4Vox

#endif