// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#include "framework.h"

#include <vector>
#include <string>
#include <array>

#include "sqlite3.h"
#include <sodium.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "libsodium.lib")

// Master key in RAM
extern std::array<uint8_t, 32> g_MasterKey;
extern bool g_HasMasterKey;

// Crypto functions
bool DeriveKeyArgon2id(const std::wstring& masterPassword,
	const std::vector<uint8_t>& salt,
	std::array<uint8_t, 32>& outKey);

bool AesGcmEncrypt(const std::array<uint8_t, 32>& key,
	const std::vector<uint8_t>& plaintext,
	std::vector<uint8_t>& outCiphertext,
	std::vector<uint8_t>& outNonce,
	std::vector<uint8_t>& outTag);

bool AesGcmDecrypt(const std::array<uint8_t, 32>& key,
	const std::vector<uint8_t>& nonce,
	const std::vector<uint8_t>& tag,
	const std::vector<uint8_t>& ciphertext,
	std::vector<uint8_t>& outPlaintext);

// Helper: CString to bytes (UTF-8)
std::vector<uint8_t> CStringToBytes(const CString& str);

#endif //PCH_H
