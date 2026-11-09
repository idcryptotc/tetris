#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace Tetris
{
    // Минимальный JSON: значение, парсер, сериализатор.
    // Без внешних зависимостей. Поддерживает: null, bool, number (int64),
    // string, array, object. Достаточно для агентского канала и MCP-сервера.
    class JsonValue
    {
    public:
        enum class Type
        {
            Null,
            Bool,
            Number,
            String,
            Array,
            Object
        };

        Type type{ Type::Null };
        bool boolValue{ false };
        int64_t numberValue{ 0 };
        std::string stringValue;
        std::vector<JsonValue> arrayValue;
        std::map<std::string, JsonValue> objectValue;

        JsonValue() = default;
        JsonValue(bool v) : type(Type::Bool), boolValue(v) {}
        JsonValue(int v) : type(Type::Number), numberValue(v) {}
        JsonValue(int64_t v) : type(Type::Number), numberValue(v) {}
        JsonValue(unsigned int v) : type(Type::Number), numberValue(static_cast<int64_t>(v)) {}
        JsonValue(const char *v) : type(Type::String), stringValue(v) {}
        JsonValue(const std::string &v) : type(Type::String), stringValue(v) {}

        // Доступ к объекту (создаёт поле при записи).
        JsonValue &operator[](const std::string &key)
        {
            type = Type::Object;
            return objectValue[key];
        }

        // Поиск поля объекта (nullptr, если нет или не объект).
        const JsonValue *Find(const std::string &key) const;

        // Справочные геттеры (безопасные значения по умолчанию).
        int64_t AsInt() const;
        bool AsBool() const;
        std::string AsString() const;
    };

    // Парсер: true при успехе, out — разобранный значение.
    bool ParseJson(const std::string &text, JsonValue &out);

    // Сериализатор (без пробелов).
    std::string SerializeJson(const JsonValue &value);
}
