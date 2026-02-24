#pragma once

namespace fs {

enum class Error : int {
    Ok = 0,
    NotFound = -1,
    NoSpace = -2,
    InvalidArg = -3,
    IOError = -4,
    NotMounted = -5,
    AlreadyExists = -6,
};

inline bool succeeded(Error e) { return static_cast<int>(e) >= 0; }
inline bool failed(Error e) { return static_cast<int>(e) < 0; }

}
