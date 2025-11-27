#pragma once
// Port target: src/app/auth_screen.rs
// Purpose: Authentication screen logic (username/password, cookies input).

#include <string>

namespace app {
namespace auth_screen {

struct Credentials {
    std::string username;
    std::string password;
    std::string cookies; // Cookie header, if used
};

enum class AuthState {
    Idle,
    Authorizing,
    Authorized,
    Failed
};

inline AuthState& _auth_state_ref() {
    static AuthState s = AuthState::Idle;
    return s;
}

// Basic header-only implementation to mirror a simple auth flow.
// - If username is non-empty AND (password OR cookies) provided => Authorized.
// - Otherwise => Failed.
// - Sets intermediate Authorizing state to mimic async flow.
inline bool authorize(const Credentials& creds) {
    _auth_state_ref() = AuthState::Authorizing;
    const bool ok = !creds.username.empty() && (!creds.password.empty() || !creds.cookies.empty());
    _auth_state_ref() = ok ? AuthState::Authorized : AuthState::Failed;
    return ok;
}

inline AuthState state() {
    return _auth_state_ref();
}

} // namespace auth_screen
} // namespace app
