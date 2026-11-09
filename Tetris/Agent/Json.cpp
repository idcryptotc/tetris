#include "Json.h"

#include <cctype>
#include <cstdlib>
#include <stdexcept>

namespace Tetris
{
    const JsonValue *JsonValue::Find(const std::string &key) const
    {
        if (type != Type::Object)
        {
            return nullptr;
        }

        auto it = objectValue.find(key);
        return it == objectValue.end() ? nullptr : &it->second;
    }

    int64_t JsonValue::AsInt() const
    {
        return type == Type::Number ? numberValue : 0;
    }

    bool JsonValue::AsBool() const
    {
        return type == Type::Bool ? boolValue : false;
    }

    std::string JsonValue::AsString() const
    {
        return type == Type::String ? stringValue : std::string();
    }

    // ---------- Парсер ----------

    namespace
    {
        struct JsonParser
        {
            const std::string &text;
            std::size_t pos{ 0 };

            explicit JsonParser(const std::string &t) : text(t) {}

            void SkipWhitespace()
            {
                while (pos < text.size()
                    && (text[pos] == ' ' || text[pos] == '\t' || text[pos] == '\n' || text[pos] == '\r'))
                {
                    ++pos;
                }
            }

            char Peek() const
            {
                return pos < text.size() ? text[pos] : '\0';
            }

            bool Consume(char expected)
            {
                if (Peek() == expected)
                {
                    ++pos;
                    return true;
                }

                return false;
            }

            void Expect(char expected)
            {
                if (!Consume(expected))
                {
                    throw std::runtime_error("JSON: ожидался символ '" + std::string(1, expected) + "'");
                }
            }

            JsonValue ParseValue()
            {
                SkipWhitespace();
                char c = Peek();

                if (c == '{')
                {
                    return ParseObject();
                }

                if (c == '[')
                {
                    return ParseArray();
                }

                if (c == '"')
                {
                    return JsonValue(ParseString());
                }

                if (c == 't' || c == 'f')
                {
                    return ParseBool();
                }

                if (c == 'n')
                {
                    ParseLiteral("null");
                    return JsonValue();
                }

                return ParseNumber();
            }

            JsonValue ParseObject()
            {
                JsonValue value;
                value.type = JsonValue::Type::Object;
                Expect('{');
                SkipWhitespace();

                if (Consume('}'))
                {
                    return value;
                }

                while (true)
                {
                    SkipWhitespace();
                    std::string key = ParseString();
                    SkipWhitespace();
                    Expect(':');
                    value.objectValue[key] = ParseValue();
                    SkipWhitespace();

                    if (Consume(','))
                    {
                        continue;
                    }

                    Expect('}');
                    break;
                }

                return value;
            }

            JsonValue ParseArray()
            {
                JsonValue value;
                value.type = JsonValue::Type::Array;
                Expect('[');
                SkipWhitespace();

                if (Consume(']'))
                {
                    return value;
                }

                while (true)
                {
                    value.arrayValue.push_back(ParseValue());
                    SkipWhitespace();

                    if (Consume(','))
                    {
                        continue;
                    }

                    Expect(']');
                    break;
                }

                return value;
            }

            std::string ParseString()
            {
                Expect('"');
                std::string result;

                while (true)
                {
                    if (pos >= text.size())
                    {
                        throw std::runtime_error("JSON: незакрытая строка");
                    }

                    char c = text[pos++];

                    if (c == '"')
                    {
                        break;
                    }

                    if (c == '\\')
                    {
                        if (pos >= text.size())
                        {
                            throw std::runtime_error("JSON: незакрытый экранирующий символ");
                        }

                        char e = text[pos++];

                        switch (e)
                        {
                            case '"': result += '"'; break;
                            case '\\': result += '\\'; break;
                            case '/': result += '/'; break;
                            case 'b': result += '\b'; break;
                            case 'f': result += '\f'; break;
                            case 'n': result += '\n'; break;
                            case 'r': result += '\r'; break;
                            case 't': result += '\t'; break;
                            case 'u':
                                {
                                    // \uXXXX — читаем 4 шестнадцатеричные цифры,
                                    // декодируем как BMP (дополнительные плоскости
                                    // не нужны: протокол — ASCII + цифры).
                                    if (pos + 4 > text.size())
                                    {
                                        throw std::runtime_error("JSON: неполный \\uXXXX");
                                    }

                                    unsigned int code = 0;

                                    for (int i = 0; i < 4; ++i)
                                    {
                                        char h = text[pos++];
                                        code <<= 4;

                                        if (h >= '0' && h <= '9')
                                        {
                                            code += static_cast<unsigned int>(h - '0');
                                        }
                                        else if (h >= 'a' && h <= 'f')
                                        {
                                            code += static_cast<unsigned int>(h - 'a' + 10);
                                        }
                                        else if (h >= 'A' && h <= 'F')
                                        {
                                            code += static_cast<unsigned int>(h - 'A' + 10);
                                        }
                                        else
                                        {
                                            throw std::runtime_error("JSON: некорректный \\uXXXX");
                                        }
                                    }

                                    // UTF-8 кодирование BMP-символа.
                                    if (code < 0x80)
                                    {
                                        result += static_cast<char>(code);
                                    }
                                    else if (code < 0x800)
                                    {
                                        result += static_cast<char>(0xC0 | (code >> 6));
                                        result += static_cast<char>(0x80 | (code & 0x3F));
                                    }
                                    else
                                    {
                                        result += static_cast<char>(0xE0 | (code >> 12));
                                        result += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                                        result += static_cast<char>(0x80 | (code & 0x3F));
                                    }

                                    break;
                                }
                            default:
                                throw std::runtime_error("JSON: неизвестный экранирующий символ");
                        }
                    }
                    else
                    {
                        result += c;
                    }
                }

                return result;
            }

            JsonValue ParseBool()
            {
                if (Peek() == 't')
                {
                    ParseLiteral("true");
                    return JsonValue(true);
                }

                ParseLiteral("false");
                return JsonValue(false);
            }

            void ParseLiteral(const char *literal)
            {
                std::size_t len = 0;

                while (literal[len] != '\0')
                {
                    ++len;
                }

                if (text.compare(pos, len, literal) != 0)
                {
                    throw std::runtime_error("JSON: некорректный литерал");
                }

                pos += len;
            }

            JsonValue ParseNumber()
            {
                std::size_t start = pos;

                if (Peek() == '-')
                {
                    ++pos;
                }

                while (pos < text.size() && std::isdigit(static_cast<unsigned char>(text[pos])) != 0)
                {
                    ++pos;
                }

                if (pos < text.size() && text[pos] == '.')
                {
                    ++pos;

                    while (pos < text.size() && std::isdigit(static_cast<unsigned char>(text[pos])) != 0)
                    {
                        ++pos;
                    }
                }

                if (pos < text.size() && (text[pos] == 'e' || text[pos] == 'E'))
                {
                    ++pos;

                    if (pos < text.size() && (text[pos] == '+' || text[pos] == '-'))
                    {
                        ++pos;
                    }

                    while (pos < text.size() && std::isdigit(static_cast<unsigned char>(text[pos])) != 0)
                    {
                        ++pos;
                    }
                }

                if (start == pos)
                {
                    throw std::runtime_error("JSON: некорректное число");
                }

                const std::string numberText = text.substr(start, pos - start);
                JsonValue value;
                value.type = JsonValue::Type::Number;
                value.numberValue = static_cast<int64_t>(std::strtoll(numberText.c_str(), nullptr, 10));
                return value;
            }
        };
    }

    bool ParseJson(const std::string &text, JsonValue &out)
    {
        try
        {
            JsonParser parser(text);
            out = parser.ParseValue();
            parser.SkipWhitespace();

            if (parser.pos != text.size())
            {
                return false; // хвост после значения — некорректно
            }

            return true;
        }
        catch (const std::exception &)
        {
            return false;
        }
    }

    // ---------- Сериализатор ----------

    namespace
    {
        void SerializeString(const std::string &s, std::string &out)
        {
            out += '"';

            for (char c : s)
            {
                switch (c)
                {
                    case '"': out += "\\\""; break;
                    case '\\': out += "\\\\"; break;
                    case '\b': out += "\\b"; break;
                    case '\f': out += "\\f"; break;
                    case '\n': out += "\\n"; break;
                    case '\r': out += "\\r"; break;
                    case '\t': out += "\\t"; break;
                    default:
                        if (static_cast<unsigned char>(c) < 0x20)
                        {
                            // Контрольные символы — \u00XX.
                            static const char hex[] = "0123456789abcdef";
                            out += "\\u00";
                            out += hex[(static_cast<unsigned char>(c) >> 4) & 0xF];
                            out += hex[static_cast<unsigned char>(c) & 0xF];
                        }
                        else
                        {
                            out += c;
                        }

                        break;
                }
            }

            out += '"';
        }

        void SerializeValue(const JsonValue &value, std::string &out)
        {
            switch (value.type)
            {
                case JsonValue::Type::Null:
                    out += "null";
                    break;
                case JsonValue::Type::Bool:
                    out += value.boolValue ? "true" : "false";
                    break;
                case JsonValue::Type::Number:
                    out += std::to_string(value.numberValue);
                    break;
                case JsonValue::Type::String:
                    SerializeString(value.stringValue, out);
                    break;
                case JsonValue::Type::Array:
                    out += '[';

                    for (std::size_t i = 0; i < value.arrayValue.size(); ++i)
                    {
                        if (i > 0)
                        {
                            out += ',';
                        }

                        SerializeValue(value.arrayValue[i], out);
                    }

                    out += ']';
                    break;
                case JsonValue::Type::Object:
                    out += '{';

                    bool first = true;

                    for (const auto &pair : value.objectValue)
                    {
                        if (!first)
                        {
                            out += ',';
                        }

                        first = false;
                        SerializeString(pair.first, out);
                        out += ':';
                        SerializeValue(pair.second, out);
                    }

                    out += '}';
                    break;
            }
        }
    }

    std::string SerializeJson(const JsonValue &value)
    {
        std::string out;
        SerializeValue(value, out);
        return out;
    }
}
