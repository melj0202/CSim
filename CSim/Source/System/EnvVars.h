#pragma once
#include "IEnvVars.h"

class EnvVars : public IEnvVars {
    public:
        EnvVars() {load();}
        ~EnvVars() {save();}

        void load();
        void save();

        void setVar(const std::string& key, const std::string& value) override;
        void setVar(const std::string& key, const long& value) override;
        void setVar(const std::string& key, const double& value) override;
        void setVar(const std::string& key, const bool& value) override;
        void setVar(const std::string& key, const unsigned int& value) override;
        void setVar(const std::string& key, const unsigned long& value) override;
        void setVar(const std::string& key, const unsigned long long& value) override;
        void setVar(const std::string& key, const char& value) override;
        void setVar(const std::string& key, const char* value) override;
        void setVar(const std::string& key, const int& value) override;
        const EnvVar& getVar(const std::string& key) override;
        const std::unordered_map<std::string, EnvVar>& getVars() const override { return m_vars; };

    private:
        std::unordered_map<std::string, EnvVar> m_vars;
};