/*
 * Copyright (c) 2020, Ali Mohammad Pur <mpfard@serenityos.org>
 * Copyright (c) 2022, the SerenityOS developers.
 * Copyright (c) 2025, Altomani Gianluca <altomanigianluca@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibCrypto/ASN1/DER.h>
#include <LibCrypto/BigInt/UnsignedBigInteger.h>
#include <LibCrypto/Hash/HashManager.h>
#include <LibCrypto/OpenSSL.h>
#include <LibCrypto/PK/PK.h>

namespace Crypto::PK {

class RSAPublicKey {
public:
    RSAPublicKey(UnsignedBigInteger n, UnsignedBigInteger e)
        : m_modulus(move(n))
        , m_public_exponent(move(e))
        , m_length(m_modulus.byte_length())
    {
    }

    RSAPublicKey()
        : m_modulus(0)
        , m_public_exponent(0)
    {
    }

    UnsignedBigInteger const& modulus() const { return m_modulus; }
    UnsignedBigInteger const& public_exponent() const { return m_public_exponent; }
    size_t length() const { return m_length; }

    ErrorOr<bool> is_valid() const;

    ErrorOr<ByteBuffer> export_as_der() const
    {
        ASN1::Encoder encoder;
        TRY(encoder.write_constructed(ASN1::Class::Universal, ASN1::Kind::Sequence, [&]() -> ErrorOr<void> {
            TRY(encoder.write(m_modulus));
            TRY(encoder.write(m_public_exponent));
            return {};
        }));

        return encoder.finish();
    }

private:
    UnsignedBigInteger m_modulus;
    UnsignedBigInteger m_public_exponent;
    size_t m_length { 0 };
};

class RSAPrivateKey {
public:
    RSAPrivateKey(UnsignedBigInteger n, UnsignedBigInteger d, UnsignedBigInteger e)
        : m_modulus(move(n))
        , m_private_exponent(move(d))
        , m_public_exponent(move(e))
        , m_length(m_modulus.byte_length())
    {
    }

    RSAPrivateKey(UnsignedBigInteger n, UnsignedBigInteger d, UnsignedBigInteger e, UnsignedBigInteger p, UnsignedBigInteger q, UnsignedBigInteger dp, UnsignedBigInteger dq, UnsignedBigInteger qinv)
        : m_modulus(move(n))
        , m_private_exponent(move(d))
        , m_public_exponent(move(e))
        , m_prime_1(move(p))
        , m_prime_2(move(q))
        , m_exponent_1(move(dp))
        , m_exponent_2(move(dq))
        , m_coefficient(move(qinv))
        , m_length(m_modulus.byte_length())
    {
    }

    RSAPrivateKey() = default;

    UnsignedBigInteger const& modulus() const { return m_modulus; }
    UnsignedBigInteger const& private_exponent() const { return m_private_exponent; }
    UnsignedBigInteger const& public_exponent() const { return m_public_exponent; }
    UnsignedBigInteger const& prime1() const { return m_prime_1; }
    UnsignedBigInteger const& prime2() const { return m_prime_2; }
    UnsignedBigInteger const& exponent1() const { return m_exponent_1; }
    UnsignedBigInteger const& exponent2() const { return m_exponent_2; }
    UnsignedBigInteger const& coefficient() const { return m_coefficient; }
    size_t length() const { return m_length; }

    ErrorOr<bool> is_valid() const;

    ErrorOr<ByteBuffer> export_as_der() const
    {
        if (m_prime_1.is_zero() || m_prime_2.is_zero()) {
            return Error::from_string_literal("Cannot export private key without prime factors");
        }

        ASN1::Encoder encoder;
        TRY(encoder.write_constructed(ASN1::Class::Universal, ASN1::Kind::Sequence, [&]() -> ErrorOr<void> {
            TRY(encoder.write(0x00u)); // version
            TRY(encoder.write(m_modulus));
            TRY(encoder.write(m_public_exponent));
            TRY(encoder.write(m_private_exponent));
            TRY(encoder.write(m_prime_1));
            TRY(encoder.write(m_prime_2));
            TRY(encoder.write(m_exponent_1));
            TRY(encoder.write(m_exponent_2));
            TRY(encoder.write(m_coefficient));
            return {};
        }));

        return encoder.finish();
    }

private:
    UnsignedBigInteger m_modulus;
    UnsignedBigInteger m_private_exponent;
    UnsignedBigInteger m_public_exponent;
    UnsignedBigInteger m_prime_1;
    UnsignedBigInteger m_prime_2;
    UnsignedBigInteger m_exponent_1;  // d mod (p-1)
    UnsignedBigInteger m_exponent_2;  // d mod (q-1)
    UnsignedBigInteger m_coefficient; // q^-1 mod p
    size_t m_length { 0 };
};

template<typename PubKey, typename PrivKey>
struct RSAKeyPair {
    PubKey public_key;
    PrivKey private_key;
};

class RSA : public PKSystem<RSAPrivateKey, RSAPublicKey> {
public:
    using KeyPairType = RSAKeyPair<PublicKeyType, PrivateKeyType>;

    static ErrorOr<KeyPairType> parse_rsa_key(ReadonlyBytes der, bool is_private, Vector<StringView> current_scope);
    static ErrorOr<KeyPairType> generate_key_pair(size_t bits, UnsignedBigInteger e = 65537);

    RSA(KeyPairType const& pair)
        : PKSystem<RSAPrivateKey, RSAPublicKey>(pair.public_key, pair.private_key)
    {
    }

    RSA(PublicKeyType const& pubkey, PrivateKeyType const& privkey)
        : PKSystem<RSAPrivateKey, RSAPublicKey>(pubkey, privkey)
    {
    }

    RSA(PrivateKeyType const& privkey)
    {
        m_private_key = privkey;
        m_public_key = RSAPublicKey(m_private_key.modulus(), m_private_key.public_exponent());
    }

    RSA(PublicKeyType const& pubkey)
    {
        m_public_key = pubkey;
    }

    RSA(ByteBuffer const& publicKeyPEM, ByteBuffer const& privateKeyPEM)
    {
        import_public_key(publicKeyPEM);
        import_private_key(privateKeyPEM);
    }

    RSA(StringView privKeyPEM)
    {
        import_private_key(privKeyPEM.bytes());
        m_public_key = RSAPublicKey(m_private_key.modulus(), m_private_key.public_exponent());
    }

    virtual ErrorOr<ByteBuffer> encrypt(ReadonlyBytes in) override;
    virtual ErrorOr<ByteBuffer> decrypt(ReadonlyBytes in) override;

    virtual ErrorOr<ByteBuffer> sign(ReadonlyBytes message) override;
    virtual ErrorOr<bool> verify(ReadonlyBytes message, ReadonlyBytes signature) override;

    virtual ByteString class_name() const override
    {
        return "RSA";
    }

    virtual size_t output_size() const override
    {
        return m_public_key.length();
    }

    void import_public_key(ReadonlyBytes, bool pem = true);
    void import_private_key(ReadonlyBytes, bool pem = true);

    PrivateKeyType const& private_key() const { return m_private_key; }
    PublicKeyType const& public_key() const { return m_public_key; }

    void set_public_key(PublicKeyType const& key) { m_public_key = key; }
    void set_private_key(PrivateKeyType const& key) { m_private_key = key; }

protected:
    virtual ErrorOr<void> configure(OpenSSL_PKEY_CTX& ctx);
};

ErrorOr<EVP_MD const*> hash_kind_to_hash_type(Hash::HashKind hash_kind);

class RSA_EME : public RSA {
public:
    template<typename... Args>
    RSA_EME(Hash::HashKind hash_kind, Args... args)
        : RSA(args...)
        , m_hash_kind(hash_kind)
    {
    }

    ~RSA_EME() = default;

    virtual ErrorOr<ByteBuffer> sign(ReadonlyBytes) override
    {
        return Error::from_string_literal("Signing is not supported");
    }
    virtual ErrorOr<bool> verify(ReadonlyBytes, ReadonlyBytes) override
    {
        return Error::from_string_literal("Verifying is not supported");
    }

protected:
    Hash::HashKind m_hash_kind { Hash::HashKind::Unknown };
};

class RSA_EMSA : public RSA {
public:
    template<typename... Args>
    RSA_EMSA(Hash::HashKind hash_kind, Args... args)
        : RSA(args...)
        , m_hash_kind(hash_kind)
    {
    }

    ~RSA_EMSA() = default;

    virtual ErrorOr<ByteBuffer> encrypt(ReadonlyBytes) override
    {
        return Error::from_string_literal("Encrypting is not supported");
    }
    virtual ErrorOr<ByteBuffer> decrypt(ReadonlyBytes) override
    {
        return Error::from_string_literal("Decrypting is not supported");
    }

    virtual ErrorOr<bool> verify(ReadonlyBytes message, ReadonlyBytes signature) override;
    virtual ErrorOr<ByteBuffer> sign(ReadonlyBytes message) override;

protected:
    Hash::HashKind m_hash_kind { Hash::HashKind::Unknown };
};

class RSA_PKCS1_EME : public RSA_EME {
public:
    template<typename... Args>
    RSA_PKCS1_EME(Args... args)
        : RSA_EME(Hash::HashKind::None, args...)
    {
    }

    ~RSA_PKCS1_EME() = default;

    virtual ByteString class_name() const override
    {
        return "RSA_PKCS1-EME";
    }

protected:
    ErrorOr<void> configure(OpenSSL_PKEY_CTX& ctx) override;
};

class RSA_PKCS1_EMSA : public RSA_EMSA {
public:
    template<typename... Args>
    RSA_PKCS1_EMSA(Hash::HashKind hash_kind, Args... args)
        : RSA_EMSA(hash_kind, args...)
    {
    }

    ~RSA_PKCS1_EMSA() = default;

    virtual ByteString class_name() const override
    {
        return "RSA_PKCS1-EMSA";
    }

protected:
    ErrorOr<void> configure(OpenSSL_PKEY_CTX& ctx) override;
};

class RSA_OAEP_EME : public RSA_EME {
public:
    template<typename... Args>
    RSA_OAEP_EME(Hash::HashKind hash_kind, Args... args)
        : RSA_EME(hash_kind, args...)
    {
    }

    ~RSA_OAEP_EME() = default;

    virtual ByteString class_name() const override
    {
        return "RSA_OAEP-EME";
    }

    void set_label(ReadonlyBytes label) { m_label = label; }

protected:
    ErrorOr<void> configure(OpenSSL_PKEY_CTX& ctx) override;

private:
    Optional<ReadonlyBytes> m_label {};
};

class RSA_PSS_EMSA : public RSA_EMSA {
public:
    template<typename... Args>
    RSA_PSS_EMSA(Hash::HashKind hash_kind, Args... args)
        : RSA_EMSA(hash_kind, args...)
    {
    }

    ~RSA_PSS_EMSA() = default;

    virtual ByteString class_name() const override
    {
        return "RSA_PSS-EMSA";
    }

    void set_salt_length(int value) { m_salt_length = value; }

protected:
    ErrorOr<void> configure(OpenSSL_PKEY_CTX& ctx) override;

private:
    Optional<int> m_salt_length;
};

}
