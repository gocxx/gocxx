/**
 * @file json.cpp
 * @brief Implementation of Go-inspired JSON encoding and decoding using nlohmann/json
 */

#include <gocxx/encoding/json.h>
#include <gocxx/errors/errors.h>
#include <sstream>
#include <algorithm>

namespace gocxx {
namespace encoding {
namespace json {

// Core JSON functions - exact Go API

gocxx::base::Result<std::vector<uint8_t>> Marshal(const JsonValue& value) {
    try {
        std::string json_str = value.dump();
        std::vector<uint8_t> result(json_str.begin(), json_str.end());
        return gocxx::base::Result<std::vector<uint8_t>>(std::move(result));
    } catch (const std::exception& e) {
        return gocxx::base::Result<std::vector<uint8_t>>(
            gocxx::errors::New("marshal error: " + std::string(e.what()))
        );
    }
}

gocxx::base::Result<std::string> MarshalString(const JsonValue& value) {
    try {
        std::string result = value.dump();
        return gocxx::base::Result<std::string>(std::move(result));
    } catch (const std::exception& e) {
        return gocxx::base::Result<std::string>(
            gocxx::errors::New("marshal error: " + std::string(e.what()))
        );
    }
}

gocxx::base::Result<void> Unmarshal(const std::vector<uint8_t>& data, JsonValue& value) {
    try {
        std::string json_str(data.begin(), data.end());
        value = JsonValue::parse(json_str);
        return gocxx::base::Result<void>();
    } catch (const nlohmann::json::parse_error& e) {
        return gocxx::base::Result<void>(
            gocxx::errors::New("unmarshal error: " + std::string(e.what()))
        );
    } catch (const std::exception& e) {
        return gocxx::base::Result<void>(
            gocxx::errors::New("unmarshal error: " + std::string(e.what()))
        );
    }
}

gocxx::base::Result<void> UnmarshalString(const std::string& data, JsonValue& value) {
    try {
        value = JsonValue::parse(data);
        return gocxx::base::Result<void>();
    } catch (const nlohmann::json::parse_error& e) {
        return gocxx::base::Result<void>(
            gocxx::errors::New("unmarshal error: " + std::string(e.what()))
        );
    } catch (const std::exception& e) {
        return gocxx::base::Result<void>(
            gocxx::errors::New("unmarshal error: " + std::string(e.what()))
        );
    }
}

bool Valid(const std::vector<uint8_t>& data) {
    try {
        std::string json_str(data.begin(), data.end());
        JsonValue::parse(json_str);
        return true;
    } catch (...) {
        return false;
    }
}

bool ValidString(const std::string& data) {
    try {
        JsonValue::parse(data);
        return true;
    } catch (...) {
        return false;
    }
}

gocxx::base::Result<std::vector<uint8_t>> Compact(const std::vector<uint8_t>& src) {
    JsonValue value;
    auto unmarshal_result = Unmarshal(src, value);
    if (!unmarshal_result.Ok()) {
        return gocxx::base::Result<std::vector<uint8_t>>(unmarshal_result.err);
    }
    
    return Marshal(value);
}

gocxx::base::Result<std::vector<uint8_t>> Indent(
    const std::vector<uint8_t>& src,
    const std::string& prefix,
    const std::string& indent
) {
    JsonValue value;
    auto unmarshal_result = Unmarshal(src, value);
    if (!unmarshal_result.Ok()) {
        return gocxx::base::Result<std::vector<uint8_t>>(unmarshal_result.err);
    }
    
    try {
        // nlohmann/json uses indent as number of spaces
        int indent_size = static_cast<int>(indent.size());
        std::string json_str = value.dump(indent_size);
        
        // Add prefix to each line if specified
        if (!prefix.empty()) {
            std::istringstream iss(json_str);
            std::ostringstream oss;
            std::string line;
            bool first = true;
            while (std::getline(iss, line)) {
                if (!first) {
                    oss << '\n';
                }
                oss << prefix << line;
                first = false;
            }
            json_str = oss.str();
        }
        
        std::vector<uint8_t> result(json_str.begin(), json_str.end());
        return gocxx::base::Result<std::vector<uint8_t>>(std::move(result));
    } catch (const std::exception& e) {
        return gocxx::base::Result<std::vector<uint8_t>>(
            gocxx::errors::New("indent error: " + std::string(e.what()))
        );
    }
}

// Streaming JSON

std::unique_ptr<Encoder> NewEncoder(std::shared_ptr<gocxx::io::Writer> writer) {
    return std::make_unique<Encoder>(writer);
}

std::unique_ptr<Decoder> NewDecoder(std::shared_ptr<gocxx::io::Reader> reader) {
    return std::make_unique<Decoder>(reader);
}

// Encoder implementation

Encoder::Encoder(std::shared_ptr<gocxx::io::Writer> writer)
    : writer_(writer), escape_html_(true) {}

gocxx::base::Result<void> Encoder::Encode(const JsonValue& value) {
    try {
        std::string json_str;
        
        if (!indent_.empty()) {
            int indent_size = static_cast<int>(indent_.size());
            json_str = value.dump(indent_size);
            
            // Add prefix to each line if specified
            if (!prefix_.empty()) {
                std::istringstream iss(json_str);
                std::ostringstream oss;
                std::string line;
                bool first = true;
                while (std::getline(iss, line)) {
                    if (!first) {
                        oss << '\n';
                    }
                    oss << prefix_ << line;
                    first = false;
                }
                json_str = oss.str();
            }
        } else {
            json_str = value.dump();
        }
        
        json_str += '\n'; // Go's encoder adds a newline
        
        std::vector<uint8_t> data(json_str.begin(), json_str.end());
        auto write_result = writer_->Write(data);
        if (!write_result.Ok()) {
            return gocxx::base::Result<void>(write_result.err);
        }
        
        return gocxx::base::Result<void>();
    } catch (const std::exception& e) {
        return gocxx::base::Result<void>(
            gocxx::errors::New("encode error: " + std::string(e.what()))
        );
    }
}

void Encoder::SetIndent(const std::string& prefix, const std::string& indent) {
    prefix_ = prefix;
    indent_ = indent;
}

void Encoder::SetEscapeHTML(bool escape) {
    escape_html_ = escape;
}

// Decoder implementation

Decoder::Decoder(std::shared_ptr<gocxx::io::Reader> reader)
    : reader_(reader), use_number_(false), disable_unknown_fields_(false) {}

gocxx::base::Result<void> Decoder::Decode(JsonValue& value) {
    // Read all available data
    std::vector<uint8_t> buffer(4096);
    std::vector<uint8_t> data;
    
    while (true) {
        auto read_result = reader_->Read(buffer);
        if (!read_result.Ok()) {
            if (data.empty()) {
                return gocxx::base::Result<void>(read_result.err);
            }
            break; // Use what we have
        }
        
        if (read_result.value == 0) {
            break; // No more data
        }
        
        data.insert(data.end(), buffer.begin(), buffer.begin() + read_result.value);
        
        // Check if we have a complete JSON value
        std::string json_str(data.begin(), data.end());
        try {
            value = JsonValue::parse(json_str);
            return gocxx::base::Result<void>();
        } catch (const nlohmann::json::parse_error&) {
            // Continue reading more data
            continue;
        }
    }
    
    return Unmarshal(data, value);
}

bool Decoder::More() {
    // Simplified implementation - in real implementation would check stream
    return false;
}

gocxx::base::Result<JsonValue> Decoder::Token() {
    // Simplified implementation - would return individual tokens
    JsonValue value;
    auto result = Decode(value);
    if (!result.Ok()) {
        return gocxx::base::Result<JsonValue>(result.err);
    }
    return gocxx::base::Result<JsonValue>(std::move(value));
}

void Decoder::UseNumber() {
    use_number_ = true;
}

void Decoder::DisallowUnknownFields() {
    disable_unknown_fields_ = true;
}

// Utility functions

bool IsNull(const JsonValue& value) {
    return value.is_null();
}

bool IsBool(const JsonValue& value) {
    return value.is_boolean();
}

bool IsInt(const JsonValue& value) {
    return value.is_number_integer();
}

bool IsFloat(const JsonValue& value) {
    return value.is_number_float();
}

bool IsString(const JsonValue& value) {
    return value.is_string();
}

bool IsArray(const JsonValue& value) {
    return value.is_array();
}

bool IsObject(const JsonValue& value) {
    return value.is_object();
}

bool GetBool(const JsonValue& value, bool defaultValue) {
    return value.is_boolean() ? value.get<bool>() : defaultValue;
}

int64_t GetInt(const JsonValue& value, int64_t defaultValue) {
    if (value.is_number_integer()) {
        return value.get<int64_t>();
    } else if (value.is_number_float()) {
        return static_cast<int64_t>(value.get<double>());
    }
    return defaultValue;
}

double GetFloat(const JsonValue& value, double defaultValue) {
    if (value.is_number()) {
        return value.get<double>();
    }
    return defaultValue;
}

std::string GetString(const JsonValue& value, const std::string& defaultValue) {
    return value.is_string() ? value.get<std::string>() : defaultValue;
}

JsonArray GetArray(const JsonValue& value, const JsonArray& defaultValue) {
    if (value.is_array()) {
        return value.get<JsonArray>();
    }
    return defaultValue;
}

JsonObject GetObject(const JsonValue& value, const JsonObject& defaultValue) {
    if (value.is_object()) {
        return value.get<JsonObject>();
    }
    return defaultValue;
}

JsonValue MakeNull() {
    return JsonValue(nullptr);
}

JsonValue MakeBool(bool value) {
    return JsonValue(value);
}

JsonValue MakeInt(int64_t value) {
    return JsonValue(value);
}

JsonValue MakeFloat(double value) {
    return JsonValue(value);
}

JsonValue MakeString(const std::string& value) {
    return JsonValue(value);
}

JsonValue MakeArray(const JsonArray& value) {
    return JsonValue(value);
}

JsonValue MakeObject(const JsonObject& value) {
    return JsonValue(value);
}

} // namespace json
} // namespace encoding
} // namespace gocxx
