#ifndef MY_JSON_H
#define MY_JSON_H

#include <sstream>
#include <iostream>

inline std::ostream& jsonObjStart(std::ostream& os) {
    os << "{" << "\n";
    return os;
}

inline std::ostream& jsonObjEnd(std::ostream& os) {
    os << "}" << "\n";
    return os;
}

template<typename T>
inline std::ostream& jsonObjAdd(std::ostream& os, std::string key, T val) {
    os << "\"" << key << "\":" << val << ",\n";
    return os;
}


inline std::ostream& jsonArrayStart(std::ostream& os) {
    os << "[" << "\n";
    return os;
}

inline std::ostream& jsonArrayEnd(std::ostream& os) {
    os << "]" << "\n";
    return os;
}

template<typename T>
inline std::ostream& jsonArrayAdd(std::ostream& os, T val) {
    os  << val << ",";
    return os;
}

#endif // MY_JSON_H