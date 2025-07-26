/**
 * @file json_test.cpp
 * @brief Comprehensive tests for Go-style JSON package using nlohmann/json
 */

#include <gtest/gtest.h>
#include <gocxx/encoding/json.h>
#include <gocxx/io/io.h>
#include <sstream>

using namespace gocxx::encoding::json;
using namespace gocxx::base;
using namespace gocxx::io;

class JsonTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup for each test
    }
    
    void TearDown() override {
        // Cleanup after each test
    }
};

// Test JsonValue construction and type checking
TEST_F(JsonTest, JsonValueConstruction) {
    // Test null
    JsonValue null_val = MakeNull();
    EXPECT_TRUE(null_val.is_null());
    EXPECT_FALSE(null_val.is_boolean());
    
    // Test boolean
    JsonValue bool_val = MakeBool(true);
    EXPECT_TRUE(bool_val.is_boolean());
    EXPECT_EQ(bool_val.get<bool>(), true);
    
    // Test integer
    JsonValue int_val = MakeInt(42);
    EXPECT_TRUE(int_val.is_number_integer());
    EXPECT_EQ(int_val.get<int64_t>(), 42);
    
    // Test float
    JsonValue float_val = MakeFloat(3.14);
    EXPECT_TRUE(float_val.is_number_float());
    EXPECT_DOUBLE_EQ(float_val.get<double>(), 3.14);
    
    // Test string
    JsonValue string_val = MakeString("hello");
    EXPECT_TRUE(string_val.is_string());
    EXPECT_EQ(string_val.get<std::string>(), "hello");
    
    // Test array
    JsonArray arr = {MakeInt(1), MakeInt(2), MakeInt(3)};
    JsonValue array_val = MakeArray(arr);
    EXPECT_TRUE(array_val.is_array());
    EXPECT_EQ(array_val.size(), 3);
    
    // Test object
    JsonObject obj = {
        {"name", MakeString("John")},
        {"age", MakeInt(30)}
    };
    JsonValue object_val = MakeObject(obj);
    EXPECT_TRUE(object_val.is_object());
    EXPECT_EQ(object_val.size(), 2);
}

// Test basic marshal/unmarshal
TEST_F(JsonTest, BasicMarshalUnmarshal) {
    // Test marshaling simple values
    auto result = MarshalString(MakeString("hello"));
    ASSERT_TRUE(result.Ok());
    EXPECT_EQ(result.value, "\"hello\"");
    
    result = MarshalString(MakeInt(42));
    ASSERT_TRUE(result.Ok());
    EXPECT_EQ(result.value, "42");
    
    result = MarshalString(MakeBool(true));
    ASSERT_TRUE(result.Ok());
    EXPECT_EQ(result.value, "true");
    
    result = MarshalString(MakeNull());
    ASSERT_TRUE(result.Ok());
    EXPECT_EQ(result.value, "null");
    
    // Test unmarshaling
    JsonValue value;
    auto unmarshal_result = UnmarshalString("\"hello\"", value);
    ASSERT_TRUE(unmarshal_result.Ok());
    EXPECT_TRUE(value.is_string());
    EXPECT_EQ(value.get<std::string>(), "hello");
    
    unmarshal_result = UnmarshalString("42", value);
    ASSERT_TRUE(unmarshal_result.Ok());
    EXPECT_TRUE(value.is_number_integer());
    EXPECT_EQ(value.get<int64_t>(), 42);
}

// Test array marshal/unmarshal
TEST_F(JsonTest, ArrayMarshalUnmarshal) {
    JsonArray arr = {
        MakeInt(1),
        MakeString("two"),
        MakeBool(true),
        MakeNull()
    };
    JsonValue array_val = MakeArray(arr);
    
    auto marshal_result = MarshalString(array_val);
    ASSERT_TRUE(marshal_result.Ok());
    
    JsonValue unmarshaled;
    auto unmarshal_result = UnmarshalString(marshal_result.value, unmarshaled);
    ASSERT_TRUE(unmarshal_result.Ok());
    EXPECT_TRUE(unmarshaled.is_array());
    EXPECT_EQ(unmarshaled.size(), 4);
    
    // Check array contents
    EXPECT_EQ(GetInt(unmarshaled[0]), 1);
    EXPECT_EQ(GetString(unmarshaled[1]), "two");
    EXPECT_EQ(GetBool(unmarshaled[2]), true);
    EXPECT_TRUE(unmarshaled[3].is_null());
}

// Test object marshal/unmarshal
TEST_F(JsonTest, ObjectMarshalUnmarshal) {
    JsonObject obj = {
        {"name", MakeString("John Doe")},
        {"age", MakeInt(30)},
        {"active", MakeBool(true)},
        {"score", MakeFloat(95.5)},
        {"address", MakeNull()}
    };
    JsonValue object_val = MakeObject(obj);
    
    auto marshal_result = MarshalString(object_val);
    ASSERT_TRUE(marshal_result.Ok());
    
    JsonValue unmarshaled;
    auto unmarshal_result = UnmarshalString(marshal_result.value, unmarshaled);
    ASSERT_TRUE(unmarshal_result.Ok());
    EXPECT_TRUE(unmarshaled.is_object());
    
    // Check object contents
    EXPECT_EQ(GetString(unmarshaled["name"]), "John Doe");
    EXPECT_EQ(GetInt(unmarshaled["age"]), 30);
    EXPECT_EQ(GetBool(unmarshaled["active"]), true);
    EXPECT_DOUBLE_EQ(GetFloat(unmarshaled["score"]), 95.5);
    EXPECT_TRUE(unmarshaled["address"].is_null());
}

// Test nested structures
TEST_F(JsonTest, NestedStructures) {
    JsonObject address = {
        {"street", MakeString("123 Main St")},
        {"city", MakeString("Anytown")},
        {"zip", MakeString("12345")}
    };
    
    JsonArray hobbies = {
        MakeString("reading"),
        MakeString("swimming"),
        MakeString("coding")
    };
    
    JsonObject person = {
        {"name", MakeString("Alice")},
        {"age", MakeInt(28)},
        {"address", MakeObject(address)},
        {"hobbies", MakeArray(hobbies)}
    };
    
    JsonValue person_val = MakeObject(person);
    
    auto marshal_result = MarshalString(person_val);
    ASSERT_TRUE(marshal_result.Ok());
    
    JsonValue unmarshaled;
    auto unmarshal_result = UnmarshalString(marshal_result.value, unmarshaled);
    ASSERT_TRUE(unmarshal_result.Ok());
    
    // Verify nested access
    EXPECT_EQ(GetString(unmarshaled["name"]), "Alice");
    EXPECT_EQ(GetString(unmarshaled["address"]["city"]), "Anytown");
    EXPECT_EQ(GetString(unmarshaled["hobbies"][1]), "swimming");
}

// Test JSON validation
TEST_F(JsonTest, JSONValidation) {
    EXPECT_TRUE(ValidString("{}"));
    EXPECT_TRUE(ValidString("[]"));
    EXPECT_TRUE(ValidString("\"hello\""));
    EXPECT_TRUE(ValidString("42"));
    EXPECT_TRUE(ValidString("true"));
    EXPECT_TRUE(ValidString("null"));
    EXPECT_TRUE(ValidString("{\"key\": \"value\"}"));
    
    EXPECT_FALSE(ValidString("{"));
    EXPECT_FALSE(ValidString("{key: value}"));
    EXPECT_FALSE(ValidString("\"unclosed string"));
    EXPECT_FALSE(ValidString("undefined"));
}

// Test compact functionality
TEST_F(JsonTest, CompactJSON) {
    std::string json_with_whitespace = R"({
        "name": "John",
        "age": 30,
        "active": true
    })";
    
    std::vector<uint8_t> input(json_with_whitespace.begin(), json_with_whitespace.end());
    auto compact_result = Compact(input);
    ASSERT_TRUE(compact_result.Ok());
    
    std::string compacted(compact_result.value.begin(), compact_result.value.end());
    EXPECT_EQ(compacted, "{\"active\":true,\"age\":30,\"name\":\"John\"}");
}

// Test indent functionality
TEST_F(JsonTest, IndentJSON) {
    JsonObject obj = {
        {"name", MakeString("John")},
        {"age", MakeInt(30)}
    };
    
    auto marshal_result = Marshal(MakeObject(obj));
    ASSERT_TRUE(marshal_result.Ok());
    
    auto indent_result = Indent(marshal_result.value, "", "  ");
    ASSERT_TRUE(indent_result.Ok());
    
    std::string indented(indent_result.value.begin(), indent_result.value.end());
    EXPECT_TRUE(indented.find("\n") != std::string::npos); // Should contain newlines
    EXPECT_TRUE(indented.find("  ") != std::string::npos); // Should contain indentation
}

// Mock Writer for testing Encoder
class MockWriter : public gocxx::io::Writer {
private:
    std::ostringstream buffer_;
    
public:
    gocxx::base::Result<std::size_t> Write(const uint8_t* buffer, std::size_t size) override {
        std::string str(reinterpret_cast<const char*>(buffer), size);
        buffer_ << str;
        return gocxx::base::Result<std::size_t>(size);
    }
    
    std::string GetBuffer() const {
        return buffer_.str();
    }
};

// Test Encoder
TEST_F(JsonTest, EncoderTest) {
    auto mock_writer = std::make_shared<MockWriter>();
    auto encoder = NewEncoder(mock_writer);
    
    JsonObject obj = {
        {"message", MakeString("hello")},
        {"count", MakeInt(5)}
    };
    
    auto encode_result = encoder->Encode(MakeObject(obj));
    ASSERT_TRUE(encode_result.Ok());
    
    std::string output = mock_writer->GetBuffer();
    EXPECT_TRUE(output.find("\"message\"") != std::string::npos);
    EXPECT_TRUE(output.find("\"hello\"") != std::string::npos);
    EXPECT_TRUE(output.find("\"count\"") != std::string::npos);
    EXPECT_TRUE(output.find("5") != std::string::npos);
    EXPECT_TRUE(output.back() == '\n'); // Should end with newline
}

// Test Encoder with indentation
TEST_F(JsonTest, EncoderIndentTest) {
    auto mock_writer = std::make_shared<MockWriter>();
    auto encoder = NewEncoder(mock_writer);
    encoder->SetIndent("", "  ");
    
    JsonObject obj = {
        {"name", MakeString("test")},
        {"data", MakeArray({MakeInt(1), MakeInt(2)})}
    };
    
    auto encode_result = encoder->Encode(MakeObject(obj));
    ASSERT_TRUE(encode_result.Ok());
    
    std::string output = mock_writer->GetBuffer();
    EXPECT_TRUE(output.find("  ") != std::string::npos); // Should contain indentation
}

// Mock Reader for testing Decoder
class MockReader : public gocxx::io::Reader {
private:
    std::istringstream stream_;
    
public:
    explicit MockReader(const std::string& data) : stream_(data) {}
    
    gocxx::base::Result<std::size_t> Read(uint8_t* buffer, std::size_t size) override {
        if (stream_.eof()) {
            return gocxx::base::Result<std::size_t>(gocxx::errors::New("EOF"));
        }
        
        stream_.read(reinterpret_cast<char*>(buffer), size);
        std::size_t bytes_read = stream_.gcount();
        return gocxx::base::Result<std::size_t>(bytes_read);
    }
};

// Test Decoder
TEST_F(JsonTest, DecoderTest) {
    std::string json_data = "{\"name\":\"test\",\"value\":42}";
    auto mock_reader = std::make_shared<MockReader>(json_data);
    auto decoder = NewDecoder(mock_reader);
    
    JsonValue value;
    auto decode_result = decoder->Decode(value);
    ASSERT_TRUE(decode_result.Ok());
    
    EXPECT_TRUE(value.is_object());
    EXPECT_EQ(GetString(value["name"]), "test");
    EXPECT_EQ(GetInt(value["value"]), 42);
}

// Test utility functions
TEST_F(JsonTest, UtilityFunctions) {
    JsonValue null_val = MakeNull();
    JsonValue bool_val = MakeBool(true);
    JsonValue int_val = MakeInt(42);
    JsonValue float_val = MakeFloat(3.14);
    JsonValue string_val = MakeString("test");
    JsonValue array_val = MakeArray({MakeInt(1), MakeInt(2)});
    JsonValue object_val = MakeObject({{"key", MakeString("value")}});
    
    // Test type checking functions
    EXPECT_TRUE(IsNull(null_val));
    EXPECT_TRUE(IsBool(bool_val));
    EXPECT_TRUE(IsInt(int_val));
    EXPECT_TRUE(IsFloat(float_val));
    EXPECT_TRUE(IsString(string_val));
    EXPECT_TRUE(IsArray(array_val));
    EXPECT_TRUE(IsObject(object_val));
    
    // Test getter functions with defaults
    EXPECT_EQ(GetBool(bool_val), true);
    EXPECT_EQ(GetBool(string_val, false), false); // default
    
    EXPECT_EQ(GetInt(int_val), 42);
    EXPECT_EQ(GetInt(string_val, 99), 99); // default
    
    EXPECT_DOUBLE_EQ(GetFloat(float_val), 3.14);
    EXPECT_DOUBLE_EQ(GetFloat(string_val, 1.0), 1.0); // default
    
    EXPECT_EQ(GetString(string_val), "test");
    EXPECT_EQ(GetString(int_val, "default"), "default"); // default
}

// Test error handling
TEST_F(JsonTest, ErrorHandling) {
    // Test invalid JSON
    JsonValue value;
    auto result = UnmarshalString("{invalid json", value);
    EXPECT_FALSE(result.Ok());
    EXPECT_TRUE(result.err->error().find("unmarshal error") != std::string::npos);
    
    // Test marshal error (this is harder to trigger with nlohmann::json)
    // Most values can be marshaled, so this test mainly checks the error path exists
}

// Test binary data handling
TEST_F(JsonTest, BinaryDataHandling) {
    std::string json_str = "{\"test\": 123}";
    std::vector<uint8_t> binary_data(json_str.begin(), json_str.end());
    
    JsonValue value;
    auto unmarshal_result = Unmarshal(binary_data, value);
    ASSERT_TRUE(unmarshal_result.Ok());
    
    auto marshal_result = Marshal(value);
    ASSERT_TRUE(marshal_result.Ok());
    
    std::string result_str(marshal_result.value.begin(), marshal_result.value.end());
    EXPECT_TRUE(ValidString(result_str));
}

// Performance test (basic)
TEST_F(JsonTest, BasicPerformance) {
    // Create a moderately complex JSON structure
    JsonArray large_array;
    for (int i = 0; i < 1000; ++i) {
        JsonObject item = {
            {"id", MakeInt(i)},
            {"name", MakeString("item_" + std::to_string(i))},
            {"active", MakeBool(i % 2 == 0)}
        };
        large_array.push_back(MakeObject(item));
    }
    
    JsonValue large_value = MakeArray(large_array);
    
    // Test marshal performance
    auto start = std::chrono::high_resolution_clock::now();
    auto marshal_result = MarshalString(large_value);
    auto end = std::chrono::high_resolution_clock::now();
    
    ASSERT_TRUE(marshal_result.Ok());
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 1000); // Should complete in less than 1 second
    
    // Test unmarshal performance
    start = std::chrono::high_resolution_clock::now();
    JsonValue unmarshaled;
    auto unmarshal_result = UnmarshalString(marshal_result.value, unmarshaled);
    end = std::chrono::high_resolution_clock::now();
    
    ASSERT_TRUE(unmarshal_result.Ok());
    
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 1000); // Should complete in less than 1 second
}
