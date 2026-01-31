#ifndef COMPONENT_CHARACTERISTICS_DATABASE_H
#define COMPONENT_CHARACTERISTICS_DATABASE_H

#include <map>
#include <string>
#include <optional>
#include "DiodeModels.h"
#include "TransistorModels.h"

namespace Nonlinear {
namespace ComponentDB {

class DiodeDatabase {
public:
    static DiodeDatabase& getInstance() {
        static DiodeDatabase instance;
        return instance;
    }
    
    std::optional<DiodeCharacteristics> lookup(const std::string& partNumber) const {
        auto it = m_diodeDatabase.find(partNumber);
        return it != m_diodeDatabase.end() ? std::make_optional(it->second) : std::nullopt;
    }
    
    DiodeCharacteristics getOrDefault(const std::string& partNumber, const std::string& defaultPart = "1N4148") const {
        auto opt = lookup(partNumber);
        return opt.has_value() ? opt.value() : lookup(defaultPart).value_or(DiodeCharacteristics::Si1N4148());
    }
    
private:
    std::map<std::string, DiodeCharacteristics> m_diodeDatabase;
    
    DiodeDatabase() {
        m_diodeDatabase["1N4148"] = DiodeCharacteristics::Si1N4148();
        m_diodeDatabase["1N914"] = DiodeCharacteristics::Si1N914();
        m_diodeDatabase["OA90"] = DiodeCharacteristics::Ge_OA90();
        m_diodeDatabase["1N4007"] = DiodeCharacteristics::Si1N4007();
    }
};

class BJTDatabase {
public:
    static BJTDatabase& getInstance() {
        static BJTDatabase instance;
        return instance;
    }
    
    std::optional<BJTCharacteristics> lookup(const std::string& partNumber) const {
        auto it = m_bjtDatabase.find(partNumber);
        return it != m_bjtDatabase.end() ? std::make_optional(it->second) : std::nullopt;
    }
    
    BJTCharacteristics getOrDefault(const std::string& partNumber, const std::string& defaultPart = "2N3904") const {
        auto opt = lookup(partNumber);
        return opt.has_value() ? opt.value() : lookup(defaultPart).value_or(BJTCharacteristics::TwoN3904());
    }
    
private:
    std::map<std::string, BJTCharacteristics> m_bjtDatabase;
    
    BJTDatabase() {
        m_bjtDatabase["2N3904"] = BJTCharacteristics::TwoN3904();
        m_bjtDatabase["2N2222"] = BJTCharacteristics::TwoN2222();
        // Note: BC107 and 2N3906 static methods not defined in this version
        // m_bjtDatabase["BC107"] = BJTCharacteristics::BC107();
        // m_bjtDatabase["2N3906"] = BJTCharacteristics::TwoN3906();
    }
};

class FETDatabase {
public:
    static FETDatabase& getInstance() {
        static FETDatabase instance;
        return instance;
    }
    
    std::optional<FETCharacteristics> lookup(const std::string& partNumber) const {
        auto it = m_fetDatabase.find(partNumber);
        return it != m_fetDatabase.end() ? std::make_optional(it->second) : std::nullopt;
    }
    
    FETCharacteristics getOrDefault(const std::string& partNumber, const std::string& defaultPart = "2N7000") const {
        auto opt = lookup(partNumber);
        // Use TwoN7000() which is defined in FETCharacteristics
        return opt.has_value() ? opt.value() : lookup(defaultPart).value_or(FETCharacteristics::TwoN7000());
    }
    
private:
    std::map<std::string, FETCharacteristics> m_fetDatabase;
    
    FETDatabase() {
        m_fetDatabase["2N7000"] = FETCharacteristics::TwoN7000();
        m_fetDatabase["BS170"] = FETCharacteristics::TwoN7000();
        // Note: J201 also available
    }
};

struct NonlinearComponentInfo {
    std::string partNumber, name;
    std::optional<DiodeCharacteristics> diodeChar;
    std::optional<BJTCharacteristics> bjtChar;
    std::optional<FETCharacteristics> fetChar;
    
    static NonlinearComponentInfo fromDiode(const std::string& partNumber, const std::string& name = "D") {
        NonlinearComponentInfo info;
        info.partNumber = partNumber;
        info.name = name;
        info.diodeChar = DiodeDatabase::getInstance().getOrDefault(partNumber);
        return info;
    }
    
    static NonlinearComponentInfo fromBJT(const std::string& partNumber, const std::string& name = "Q", bool isPNP = false) {
        NonlinearComponentInfo info;
        info.partNumber = partNumber;
        info.name = name;
        info.bjtChar = BJTDatabase::getInstance().getOrDefault(partNumber);
        return info;
    }
    
    static NonlinearComponentInfo fromFET(const std::string& partNumber, const std::string& name = "M", bool isPNP = false) {
        NonlinearComponentInfo info;
        info.partNumber = partNumber;
        info.name = name;
        info.fetChar = FETDatabase::getInstance().getOrDefault(partNumber);
        return info;
    }
    
    std::string typeString() const {
        if (diodeChar.has_value()) return "DIODE";
        if (bjtChar.has_value()) return "BJT";
        if (fetChar.has_value()) return "FET";
        return "UNKNOWN";
    }
};

inline DiodeDatabase& getDiodeDB() { return DiodeDatabase::getInstance(); }
inline BJTDatabase& getBJTDB() { return BJTDatabase::getInstance(); }
inline FETDatabase& getFETDB() { return FETDatabase::getInstance(); }

}  // namespace ComponentDB
}  // namespace Nonlinear

#endif
