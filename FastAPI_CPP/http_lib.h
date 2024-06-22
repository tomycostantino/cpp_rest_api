// Tomas Costantino

#ifndef HTTP_LIB_H
#define HTTP_LIB_H

#include <string>
#include <map>
#include <sstream>
#include <vector>
#include <algorithm>
#include <variant>

namespace http {

    class JSON {
    public:
        using Object = std::map<std::string, JSON>;
        using Array = std::vector<JSON>;
        using Value = std::variant<std::nullptr_t, bool, int, double, std::string, Array, Object>;

        JSON() : m_value(nullptr) {}
        JSON(std::nullptr_t) : m_value(nullptr) {}
        JSON(bool value) : m_value(value) {}
        JSON(int value) : m_value(value) {}
        JSON(double value) : m_value(value) {}
        JSON(const char* value) : m_value(std::string(value)) {}
        JSON(const std::string& value) : m_value(value) {}
        JSON(const Array& value) : m_value(value) {}
        JSON(const Object& value) : m_value(value) {}

        static JSON object(std::initializer_list<std::pair<const std::string, JSON>> init) {
            return JSON(Object(init.begin(), init.end()));
        }

        static JSON array(std::initializer_list<JSON> init) {
            return JSON(Array(init.begin(), init.end()));
        }

        std::string stringify() const {
            return std::visit([](auto&& arg) -> std::string {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, std::nullptr_t>) {
                    return "null";
                } else if constexpr (std::is_same_v<T, bool>) {
                    return arg ? "true" : "false";
                } else if constexpr (std::is_same_v<T, int>) {
                    return std::to_string(arg);
                } else if constexpr (std::is_same_v<T, double>) {
                    return std::to_string(arg);
                } else if constexpr (std::is_same_v<T, std::string>) {
                    return "\"" + escape_string(arg) + "\"";
                } else if constexpr (std::is_same_v<T, Array>) {
                    std::string result = "[";
                    for (size_t i = 0; i < arg.size(); ++i) {
                        if (i > 0) result += ",";
                        result += arg[i].stringify();
                    }
                    result += "]";
                    return result;
                } else if constexpr (std::is_same_v<T, Object>) {
                    std::string result = "{";
                    bool first = true;
                    for (const auto& [key, value] : arg) {
                        if (!first) result += ",";
                        result += "\"" + escape_string(key) + "\":" + value.stringify();
                        first = false;
                    }
                    result += "}";
                    return result;
                }
            }, m_value);
        }

    private:
        Value m_value;

        static std::string escape_string(const std::string& s) {
            std::string result;
            for (char c : s) {
                switch (c) {
                    case '"': result += "\\\""; break;
                    case '\\': result += "\\\\"; break;
                    case '\b': result += "\\b"; break;
                    case '\f': result += "\\f"; break;
                    case '\n': result += "\\n"; break;
                    case '\r': result += "\\r"; break;
                    case '\t': result += "\\t"; break;
                    default:
                        if ('\x00' <= c && c <= '\x1f') {
                            result += "\\u" + std::string(4 - std::to_string((int)c).length(), '0') + std::to_string((int)c);
                        } else {
                            result += c;
                        }
                }
            }
            return result;
        }
    };

    enum class Method {
        GET,
        HEAD,
        POST,
        PUT,
        PATCH,
        DELETE,
        CONNECT,
        OPTIONS,
        TRACE,
        UNKNOWN
    };

    enum class HttpStatus {
        OK = 200,
        CREATED = 201,
        ACCEPTED = 202,
        NO_CONTENT = 204,
        BAD_REQUEST = 400,
        UNAUTHORIZED = 401,
        FORBIDDEN = 403,
        NOT_FOUND = 404,
        METHOD_NOT_ALLOWED = 405,
        INTERNAL_SERVER_ERROR = 500,
        NOT_IMPLEMENTED = 501,
        BAD_GATEWAY = 502,
        SERVICE_UNAVAILABLE = 503
    };

    struct Version {
        int major;
        int minor;
    };

    struct Request {
        Method method;
        std::string uri;
        Version version;
        std::map<std::string, std::string> headers;
        std::string body;
    };

    struct Response {
        Version version;
        HttpStatus status;
        std::map<std::string, std::string> headers;
        std::string body;

        std::string status_message() const {
            switch (status) {
                case HttpStatus::OK: return "OK";
                case HttpStatus::CREATED: return "Created";
                case HttpStatus::ACCEPTED: return "Accepted";
                case HttpStatus::NO_CONTENT: return "No Content";
                case HttpStatus::BAD_REQUEST: return "Bad Request";
                case HttpStatus::UNAUTHORIZED: return "Unauthorized";
                case HttpStatus::FORBIDDEN: return "Forbidden";
                case HttpStatus::NOT_FOUND: return "Not Found";
                case HttpStatus::METHOD_NOT_ALLOWED: return "Method Not Allowed";
                case HttpStatus::INTERNAL_SERVER_ERROR: return "Internal Server Error";
                case HttpStatus::NOT_IMPLEMENTED: return "Not Implemented";
                case HttpStatus::BAD_GATEWAY: return "Bad Gateway";
                case HttpStatus::SERVICE_UNAVAILABLE: return "Service Unavailable";
                default: return "Unknown Status";
            }
        }
    };

    inline std::string trim(const std::string& str) {
        auto start = std::find_if_not(str.begin(), str.end(), ::isspace);
        auto end = std::find_if_not(str.rbegin(), str.rend(), ::isspace).base();
        return (start < end ? std::string(start, end) : std::string());
    }

    inline std::vector<std::string> split(const std::string& str, char delim) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(str);
        while (std::getline(tokenStream, token, delim)) {
            tokens.push_back(token);
        }
        return tokens;
    }

    // Parsers
    inline Method string_to_method(const std::string& method) {
        if (method == "GET") return Method::GET;
        if (method == "HEAD") return Method::HEAD;
        if (method == "POST") return Method::POST;
        if (method == "PUT") return Method::PUT;
        if (method == "DELETE") return Method::DELETE;
        if (method == "CONNECT") return Method::CONNECT;
        if (method == "OPTIONS") return Method::OPTIONS;
        if (method == "TRACE") return Method::TRACE;
        if (method == "PATCH") return Method::PATCH;
        return Method::UNKNOWN;
    }

    inline std::string method_to_string(Method method) {
        switch (method) {
            case Method::GET: return "GET";
            case Method::HEAD: return "HEAD";
            case Method::POST: return "POST";
            case Method::PUT: return "PUT";
            case Method::DELETE: return "DELETE";
            case Method::CONNECT: return "CONNECT";
            case Method::OPTIONS: return "OPTIONS";
            case Method::TRACE: return "TRACE";
            case Method::PATCH: return "PATCH";
            default: return "UNKNOWN";
        }
    }

    inline Request parse_request(const std::string& raw_request) {
        Request request;
        std::istringstream stream(raw_request);
        std::string line;

        std::getline(stream, line);
        auto parts = split(line, ' ');
        if (parts.size() >= 3) {
            request.method = string_to_method(parts[0]);
            request.uri = parts[1];
            auto version_parts = split(parts[2].substr(5), '.');
            request.version = {std::stoi(version_parts[0]), std::stoi(version_parts[1])};
        }

        while (std::getline(stream, line) && line != "\r") {
            auto colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                auto key = trim(line.substr(0, colon_pos));
                auto value = trim(line.substr(colon_pos + 1));
                request.headers[key] = value;
            }
        }

        std::string body((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        request.body = body;

        return request;
    }

    inline std::string construct_response(const Response& response) {
        std::ostringstream stream;
        stream << "HTTP/" << response.version.major << "." << response.version.minor << " "
               << static_cast<int>(response.status) << " " << response.status_message() << "\r\n";

        for (const auto& header : response.headers) {
            stream << header.first << ": " << header.second << "\r\n";
        }

        stream << "\r\n" << response.body;
        return stream.str();
    }

    inline Response HTTP_200_OK(const JSON& body = JSON(), std::map<std::string, std::string> headers = {{"Content-Type", "application/json"}}) {
        return Response{{1, 1}, HttpStatus::OK, std::move(headers), body.stringify()};
    }

    inline Response HTTP_201_CREATED(const JSON& body = JSON(), std::map<std::string, std::string> headers = {{"Content-Type", "application/json"}}) {
        return Response{{1, 1}, HttpStatus::CREATED, std::move(headers), body.stringify()};
    }

    inline Response HTTP_400_BAD_REQUEST(const JSON& body = JSON(), std::map<std::string, std::string> headers = {{"Content-Type", "application/json"}}) {
        return Response{{1, 1}, HttpStatus::BAD_REQUEST, std::move(headers), body.stringify()};
    }

    inline Response HTTP_404_NOT_FOUND(const JSON& body = JSON(), std::map<std::string, std::string> headers = {{"Content-Type", "application/json"}}) {
        return Response{{1, 1}, HttpStatus::NOT_FOUND, std::move(headers), body.stringify()};
    }

    inline Response HTTP_500_INTERNAL_SERVER_ERROR(const JSON& body = JSON(), std::map<std::string, std::string> headers = {{"Content-Type", "application/json"}}) {
        return Response{{1, 1}, HttpStatus::INTERNAL_SERVER_ERROR, std::move(headers), body.stringify()};
    }

    inline Response custom_response(HttpStatus status, const JSON& body = JSON(), std::map<std::string, std::string> headers = {{"Content-Type", "application/json"}}) {
        return Response{{1, 1}, status, std::move(headers), body.stringify()};
    }
}

#endif