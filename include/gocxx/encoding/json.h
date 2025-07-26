/**
 * @file json.h
 * @brief Go-inspired JSON encoding and decoding for C++ using nlohmann/json
 * 
 * This module provides Go-like JSON functionality that matches Go's encoding/json API:
 * - json.Marshal(v interface{}) ([]byte, error)
 * - json.Unmarshal(data []byte, v interface{}) error
 * - json.NewEncoder(w io.Writer) *Encoder
 * - json.NewDecoder(r io.Reader) *Decoder
 * 
 * Key features:
 * - Exact Go API mapping with gocxx Result types
 * - Built on top of nlohmann/json for reliability and performance
 * - Support for basic types: string, int, float, bool, arrays, objects
 * - Streaming JSON with Encoder/Decoder
 * - Struct tag support for field mapping
 * - Error handling with descriptive messages
 * - Memory efficient operations
 * 
 * @author gocxx
 * @date 2025
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <istream>
#include <ostream>

#include <nlohmann/json.hpp>
#include <gocxx/base/result.h>
#include <gocxx/io/io.h>

namespace gocxx {
namespace encoding {
namespace json {

// Use nlohmann::json as the underlying type
using JsonValue = nlohmann::json;
using JsonArray = std::vector<JsonValue>;
using JsonObject = std::map<std::string, JsonValue>;

// Forward declarations
class Encoder;
class Decoder;

// Core JSON functions - exact Go API

/**
 * @brief Marshal returns the JSON encoding of v
 * Go equivalent: json.Marshal(v interface{}) ([]byte, error)
 * @param value The value to encode as JSON
 * @return Result containing JSON bytes or error
 */
gocxx::base::Result<std::vector<uint8_t>> Marshal(const JsonValue& value);

/**
 * @brief Marshal returns the JSON encoding of v as string
 * Convenience function - not in Go but commonly needed
 * @param value The value to encode as JSON
 * @return Result containing JSON string or error
 */
gocxx::base::Result<std::string> MarshalString(const JsonValue& value);

/**
 * @brief Unmarshal parses the JSON-encoded data and stores the result in value
 * Go equivalent: json.Unmarshal(data []byte, v interface{}) error
 * @param data JSON bytes to parse
 * @param value Reference to store the parsed result
 * @return Result indicating success or error
 */
gocxx::base::Result<void> Unmarshal(const std::vector<uint8_t>& data, JsonValue& value);

/**
 * @brief Unmarshal parses the JSON-encoded string and stores the result in value
 * Convenience function - not in Go but commonly needed
 * @param data JSON string to parse
 * @param value Reference to store the parsed result
 * @return Result indicating success or error
 */
gocxx::base::Result<void> UnmarshalString(const std::string& data, JsonValue& value);

/**
 * @brief Valid reports whether data is a valid JSON encoding
 * Go equivalent: json.Valid(data []byte) bool
 * @param data JSON bytes to validate
 * @return true if valid JSON, false otherwise
 */
bool Valid(const std::vector<uint8_t>& data);

/**
 * @brief Valid reports whether data is a valid JSON encoding
 * Convenience function for string input
 * @param data JSON string to validate
 * @return true if valid JSON, false otherwise
 */
bool ValidString(const std::string& data);

/**
 * @brief Compact appends to dst the JSON-encoded src with insignificant space characters elided
 * Go equivalent: json.Compact(dst *bytes.Buffer, src []byte) error
 * @param src Source JSON to compact
 * @return Result containing compacted JSON or error
 */
gocxx::base::Result<std::vector<uint8_t>> Compact(const std::vector<uint8_t>& src);

/**
 * @brief Indent appends to dst an indented form of the JSON-encoded src
 * Go equivalent: json.Indent(dst *bytes.Buffer, src []byte, prefix, indent string) error
 * @param src Source JSON to indent
 * @param prefix String to prefix each line with
 * @param indent String to use for each level of indentation
 * @return Result containing indented JSON or error
 */
gocxx::base::Result<std::vector<uint8_t>> Indent(
    const std::vector<uint8_t>& src,
    const std::string& prefix,
    const std::string& indent
);

// Streaming JSON - exact Go API

/**
 * @brief NewEncoder returns a new encoder that writes to w
 * Go equivalent: json.NewEncoder(w io.Writer) *Encoder
 * @param writer Writer to output JSON to
 * @return Unique pointer to Encoder
 */
std::unique_ptr<Encoder> NewEncoder(std::shared_ptr<gocxx::io::Writer> writer);

/**
 * @brief NewDecoder returns a new decoder that reads from r
 * Go equivalent: json.NewDecoder(r io.Reader) *Decoder
 * @param reader Reader to read JSON from
 * @return Unique pointer to Decoder
 */
std::unique_ptr<Decoder> NewDecoder(std::shared_ptr<gocxx::io::Reader> reader);

/**
 * @brief JSON Encoder for streaming JSON output
 * Go equivalent: encoding/json.Encoder
 */
class Encoder {
private:
    std::shared_ptr<gocxx::io::Writer> writer_;
    std::string indent_;
    std::string prefix_;
    bool escape_html_;
    
public:
    explicit Encoder(std::shared_ptr<gocxx::io::Writer> writer);
    
    /**
     * @brief Encode writes the JSON encoding of v to the stream
     * Go equivalent: func (enc *Encoder) Encode(v interface{}) error
     * @param value Value to encode
     * @return Result indicating success or error
     */
    gocxx::base::Result<void> Encode(const JsonValue& value);
    
    /**
     * @brief SetIndent instructs the encoder to format each JSON element on a new line
     * Go equivalent: func (enc *Encoder) SetIndent(prefix, indent string)
     * @param prefix String to prefix each line with
     * @param indent String to use for each level of indentation
     */
    void SetIndent(const std::string& prefix, const std::string& indent);
    
    /**
     * @brief SetEscapeHTML specifies whether problematic HTML characters should be escaped
     * Go equivalent: func (enc *Encoder) SetEscapeHTML(on bool)
     * @param escape Whether to escape HTML characters
     */
    void SetEscapeHTML(bool escape);
};

/**
 * @brief JSON Decoder for streaming JSON input  
 * Go equivalent: encoding/json.Decoder
 */
class Decoder {
private:
    std::shared_ptr<gocxx::io::Reader> reader_;
    bool use_number_;
    bool disable_unknown_fields_;
    
public:
    explicit Decoder(std::shared_ptr<gocxx::io::Reader> reader);
    
    /**
     * @brief Decode reads the next JSON-encoded value from its input and stores it in value
     * Go equivalent: func (dec *Decoder) Decode(v interface{}) error
     * @param value Reference to store decoded value
     * @return Result indicating success or error
     */
    gocxx::base::Result<void> Decode(JsonValue& value);
    
    /**
     * @brief More reports whether there is another element in the current array or object
     * Go equivalent: func (dec *Decoder) More() bool
     * @return true if more elements exist
     */
    bool More();
    
    /**
     * @brief Token returns the next JSON token in the input stream
     * Go equivalent: func (dec *Decoder) Token() (Token, error)
     * @return Result containing next token or error
     */
    gocxx::base::Result<JsonValue> Token();
    
    /**
     * @brief UseNumber causes Decoder to unmarshal numbers as strings instead of floats
     * Go equivalent: func (dec *Decoder) UseNumber()
     */
    void UseNumber();
    
    /**
     * @brief DisallowUnknownFields causes Decoder to return an error when encountering unknown fields
     * Go equivalent: func (dec *Decoder) DisallowUnknownFields()
     */
    void DisallowUnknownFields();
};

// Utility functions for working with JsonValue

/**
 * @brief Check if JsonValue is null
 */
bool IsNull(const JsonValue& value);

/**
 * @brief Check if JsonValue is boolean
 */
bool IsBool(const JsonValue& value);

/**
 * @brief Check if JsonValue is integer
 */
bool IsInt(const JsonValue& value);

/**
 * @brief Check if JsonValue is floating point
 */
bool IsFloat(const JsonValue& value);

/**
 * @brief Check if JsonValue is string
 */
bool IsString(const JsonValue& value);

/**
 * @brief Check if JsonValue is array
 */
bool IsArray(const JsonValue& value);

/**
 * @brief Check if JsonValue is object
 */
bool IsObject(const JsonValue& value);

/**
 * @brief Get boolean value (with default if not boolean)
 */
bool GetBool(const JsonValue& value, bool defaultValue = false);

/**
 * @brief Get integer value (with default if not integer)
 */
int64_t GetInt(const JsonValue& value, int64_t defaultValue = 0);

/**
 * @brief Get float value (with default if not float)
 */
double GetFloat(const JsonValue& value, double defaultValue = 0.0);

/**
 * @brief Get string value (with default if not string)
 */
std::string GetString(const JsonValue& value, const std::string& defaultValue = "");

/**
 * @brief Get array value (with default if not array)
 */
JsonArray GetArray(const JsonValue& value, const JsonArray& defaultValue = {});

/**
 * @brief Get object value (with default if not object)
 */
JsonObject GetObject(const JsonValue& value, const JsonObject& defaultValue = {});

/**
 * @brief Create JsonValue from basic types
 */
JsonValue MakeNull();
JsonValue MakeBool(bool value);
JsonValue MakeInt(int64_t value);
JsonValue MakeFloat(double value);
JsonValue MakeString(const std::string& value);
JsonValue MakeArray(const JsonArray& value);
JsonValue MakeObject(const JsonObject& value);

} // namespace json
} // namespace encoding
} // namespace gocxx
