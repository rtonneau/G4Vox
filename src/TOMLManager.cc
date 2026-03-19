// TOMLManager.cc
#include "G4Vox/TOMLManager.hh"
#include <fstream>
#include <sstream>
#include "G4ios.hh"

namespace G4Vox
{
    TOMLManager *TOMLManager::fInstance = nullptr;

    TOMLManager *TOMLManager::GetInstance()
    {
        if (!fInstance)
            fInstance = new TOMLManager();
        return fInstance;
    }

    // ── resolve "a.b.c" creating sub-tables as needed ─────────────
    toml::table &TOMLManager::ResolvePath(const std::string &dotPath,
                                          std::string &lastKey)
    {
        std::istringstream ss(dotPath);
        std::string token;
        std::vector<std::string> keys;
        while (std::getline(ss, token, '.'))
            keys.push_back(token);

        lastKey = keys.back();
        keys.pop_back();

        toml::table *node = &fDocument;
        for (const auto &k : keys)
        {
            if (!node->contains(k))
                node->insert(k, toml::table{});
            node = node->at(k).as_table();
        }
        return *node;
    }

    void TOMLManager::CleanTOMLData()
    {
        this->fDocument = toml::table{};
    }

    // ── Set scalar ────────────────────────────────────────────────
    // template <typename T>
    // void TOMLManager::Set(const std::string &dotPath, T &&value)
    // {
    //     std::string lastKey;
    //     toml::table &parent = ResolvePath(dotPath, lastKey);
    //     parent.insert_or_assign(lastKey, std::forward<T>(value));
    // }

    // // Explicit instantiations for common types
    // template void TOMLManager::Set(const std::string &, int &&);
    // template void TOMLManager::Set(const std::string &, double &&);
    // template void TOMLManager::Set(const std::string &, std::string &&);
    // template void TOMLManager::Set(const std::string &, bool &&);
    // // const& overloads too if you use them
    // template void TOMLManager::Set<const bool &>(const std::string &, const bool &);
    // template void TOMLManager::Set<const int &>(const std::string &, const int &);

    // ── Append table to array ─────────────────────────────────────
    toml::table &TOMLManager::AppendTable(const std::string &arrayPath)
    {
        std::string lastKey;
        toml::table &parent = ResolvePath(arrayPath, lastKey);

        if (!parent.contains(lastKey))
            parent.insert(lastKey, toml::array{});

        auto *arr = parent.at(lastKey).as_array();
        arr->push_back(toml::table{});
        return *arr->back().as_table();
    }

    void TOMLManager::AppendTable(const std::string &arrayKey, toml::table &&table)
    {
        // create array if it doesn't exist yet
        if (!fDocument.contains(arrayKey))
            fDocument.insert(arrayKey, toml::array{});

        fDocument.at(arrayKey).as_array()->push_back(std::move(table));
    }
    void TOMLManager::AppendTable(const std::string &parentKey, const std::string &arrayKey, toml::table &&table)
    {
        // ensure [parentKey] table exists
        if (!fDocument.contains(parentKey))
            fDocument.insert(parentKey, toml::table{});

        auto *parent = fDocument.at(parentKey).as_table();

        // ensure parentKey.arrayKey array exists
        if (!parent->contains(arrayKey))
            parent->insert(arrayKey, toml::array{});

        parent->at(arrayKey).as_array()->push_back(std::move(table));
    }

    // ── Write ─────────────────────────────────────────────────────
    void TOMLManager::Write(const G4String &filename)
    {
        G4String filePath = fRootPath + "/" + filename;
        std::ofstream out(filePath);
        if (!out.is_open())
        {
            G4cerr << "[TOMLManager] Cannot open: " << filePath << G4endl;
            return;
        }
        out << fDocument;
        G4cout << "[TOMLManager] Written to: " << filePath << G4endl;
        this->CleanTOMLData();
    }

} // namespace G4Vox