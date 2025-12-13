
#include "pch.h"
#pragma comment(lib, "libsodium.lib")

std::array<uint8_t, 32> g_MasterKey{};
bool g_HasMasterKey = false;

std::string WStringToUtf8(const std::wstring& wstr)
{
	if (wstr.empty()) return {};
	int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), result.data(), sizeNeeded, nullptr, nullptr);
	return result;
}

std::vector<uint8_t> CStringToBytes(const CString& str)
{
	LPCTSTR psz = (LPCTSTR)str;
	int len = str.GetLength();
	if (len == 0) return {};
	
	int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, psz, len, nullptr, 0, nullptr, nullptr);
	if (sizeNeeded <= 0) return {};
	
	std::vector<uint8_t> result(sizeNeeded);
	WideCharToMultiByte(CP_UTF8, 0, psz, len, (LPSTR)result.data(), sizeNeeded, nullptr, nullptr);
	return result;
}

bool DeriveKeyArgon2id(const std::wstring& masterPassword,
	const std::vector<uint8_t>& salt,
	std::array<uint8_t, 32>& outKey)
{
	if (sodium_init() < 0) return false;
	if (salt.size() == 0) return false;
	std::string pwUtf8 = WStringToUtf8(masterPassword);
	int rc = crypto_pwhash(outKey.data(), outKey.size(),
		pwUtf8.c_str(), pwUtf8.size(),
		salt.data(),
		crypto_pwhash_OPSLIMIT_MODERATE,
		crypto_pwhash_MEMLIMIT_MODERATE,
		crypto_pwhash_ALG_ARGON2ID13);
	return rc == 0;
}

bool AesGcmEncrypt(const std::array<uint8_t, 32>& key,
	const std::vector<uint8_t>& plaintext,
	std::vector<uint8_t>& outCiphertext,
	std::vector<uint8_t>& outNonce,
	std::vector<uint8_t>& outTag)
{
	BCRYPT_ALG_HANDLE hAlg = nullptr;
	BCRYPT_KEY_HANDLE hKey = nullptr;
	NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, nullptr, 0);
	if (status != 0) return false;

	status = BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_GCM,
		(ULONG)sizeof(BCRYPT_CHAIN_MODE_GCM), 0);
	if (status != 0) { BCryptCloseAlgorithmProvider(hAlg, 0); return false; }

	DWORD keyObjLen = 0, cbRes = 0;
	status = BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&keyObjLen, sizeof(DWORD), &cbRes, 0);
	if (status != 0) { BCryptCloseAlgorithmProvider(hAlg, 0); return false; }

	std::vector<BYTE> keyObj(keyObjLen);
	status = BCryptGenerateSymmetricKey(hAlg, &hKey, keyObj.data(), keyObjLen,
		(PUCHAR)key.data(), (ULONG)key.size(), 0);
	if (status != 0) { BCryptCloseAlgorithmProvider(hAlg, 0); return false; }

	outNonce.resize(12);
	BCryptGenRandom(nullptr, outNonce.data(), (ULONG)outNonce.size(), BCRYPT_USE_SYSTEM_PREFERRED_RNG);

	outCiphertext.resize(plaintext.size());
	outTag.resize(16);

	BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO ainfo;
	BCRYPT_INIT_AUTH_MODE_INFO(ainfo);
	ainfo.pbNonce = outNonce.data();
	ainfo.cbNonce = (ULONG)outNonce.size();
	ainfo.pbTag = outTag.data();
	ainfo.cbTag = (ULONG)outTag.size();
	ainfo.pbAuthData = nullptr;
	ainfo.cbAuthData = 0;
	status = BCryptEncrypt(hKey,
		(PUCHAR)plaintext.data(), (ULONG)plaintext.size(),
		&ainfo,
		nullptr, 0,
		outCiphertext.data(), (ULONG)outCiphertext.size(),
		&cbRes,
		0);
	BCryptDestroyKey(hKey);
	BCryptCloseAlgorithmProvider(hAlg, 0);
	return status == 0;
}

bool AesGcmDecrypt(const std::array<uint8_t, 32>& key,
	const std::vector<uint8_t>& nonce,
	const std::vector<uint8_t>& tag,
	const std::vector<uint8_t>& ciphertext,
	std::vector<uint8_t>& outPlaintext)
{
	BCRYPT_ALG_HANDLE hAlg = nullptr;
	BCRYPT_KEY_HANDLE hKey = nullptr;
	NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, nullptr, 0);
	if (status != 0) return false;

	status = BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_GCM,
		(ULONG)sizeof(BCRYPT_CHAIN_MODE_GCM), 0);
	if (status != 0) { BCryptCloseAlgorithmProvider(hAlg, 0); return false; }

	DWORD keyObjLen = 0, cbRes = 0;
	status = BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&keyObjLen, sizeof(DWORD), &cbRes, 0);
	if (status != 0) { BCryptCloseAlgorithmProvider(hAlg, 0); return false; }

	std::vector<BYTE> keyObj(keyObjLen);
	status = BCryptGenerateSymmetricKey(hAlg, &hKey, keyObj.data(), keyObjLen,
		(PUCHAR)key.data(), (ULONG)key.size(), 0);
	if (status != 0) { BCryptCloseAlgorithmProvider(hAlg, 0); return false; }

	outPlaintext.resize(ciphertext.size());

	BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO ainfo;
	BCRYPT_INIT_AUTH_MODE_INFO(ainfo);
	ainfo.pbNonce = const_cast<PUCHAR>(nonce.data());
	ainfo.cbNonce = (ULONG)nonce.size();
	ainfo.pbTag = const_cast<PUCHAR>(tag.data());
	ainfo.cbTag = (ULONG)tag.size();
	ainfo.pbAuthData = nullptr;
	ainfo.cbAuthData = 0;

	status = BCryptDecrypt(hKey,
		(PUCHAR)ciphertext.data(), (ULONG)ciphertext.size(),
		&ainfo,
		nullptr, 0,
		outPlaintext.data(), (ULONG)outPlaintext.size(),
		&cbRes,
		0);

	BCryptDestroyKey(hKey);
	BCryptCloseAlgorithmProvider(hAlg, 0);
	return status == 0;
}

