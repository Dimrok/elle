#include <openssl/crypto.h>
#include <openssl/engine.h>
#include <openssl/err.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

#include <elle/Buffer.hh>
#include <elle/log.hh>
#include <elle/serialization/binary.hh>

#include <cryptography/Cipher.hh>
#include <cryptography/Error.hh>
#include <cryptography/Oneway.hh>
#include <cryptography/SecretKey.hh>
#include <cryptography/cryptography.hh>
#include <cryptography/envelope.hh>
#include <cryptography/finally.hh>
#include <cryptography/raw.hh>

ELLE_LOG_COMPONENT("infinit.cryptography.envelope");

namespace infinit
{
  namespace cryptography
  {
    namespace envelope
    {
      /*-----------------.
      | Static Functions |
      `-----------------*/

      // Return the difference between a secret key in its serialized form
      // and the originally requested length for this key.
      //
      // This overhead therefore represents the additional bits implied by
      // the serialization mechanism.
      static
      elle::Natural32
      _overhead()
      {
        ELLE_DEBUG_FUNCTION("");

        // The static password used to construct this dummy secret key
        // whose only purpose is to calculate the serialization overhead.
        elle::String password =
          "An equation means nothing to me unless it expresses "
          "a thought of God. Srinivasa Ramanujan";
        elle::Natural32 length_original = password.length() * 8;

        // The cipher is not important as it does not impact the
        // serialization's overhead.
        Cipher cipher = Cipher::aes256;
        Mode mode = Mode::cbc;
        Oneway oneway = Oneway::sha256;
        SecretKey secret(password, cipher, mode, oneway);

        elle::Buffer buffer;

        {
          elle::IOStream stream(buffer.ostreambuf());
          elle::serialization::binary::SerializerOut output(stream, false); // XXX
          output.serialize("secret", secret);
        }

        elle::Natural32 length_final = buffer.size() * 8;

        ELLE_ASSERT_GTE(length_final, length_original);

        // Return the difference between the requested secret length and
        // the actual one.
        elle::Natural32 overhead = length_final - length_original;
        ELLE_DEBUG("overhead: %s", overhead);
        return (overhead);
      }

      /*----------.
      | Functions |
      `----------*/

      elle::Buffer
      seal(elle::ConstWeakBuffer const& plain,
           ::EVP_PKEY_CTX* context,
           int (*function)(EVP_PKEY_CTX*,
                           unsigned char*,
                           size_t*,
                           const unsigned char*,
                           size_t),
           ::EVP_CIPHER const* cipher,
           ::EVP_MD const* oneway,
           elle::Natural32 const padding_size)
      {
        ELLE_DEBUG_FUNCTION(context, function, cipher, oneway, padding_size);
        ELLE_DUMP("plain: %s", plain);

        // Make sure the cryptographic system is set up.
        cryptography::require();

        // The overhead represents the difference between a bare secret
        // key length and the same key in its serialized form. Note that
        // the overhead is expressed in bits.
        //
        // Without computing this difference, one may think the following
        // is going to encrypt the temporary secret key with the given
        // context, ending up with an output code key looking as below:
        //
        //   [symmetrically encrypted secret][padding]
        //
        // Unfortunately, before asymmetrically encrypting the secret key,
        // one must serialize it. Since the serialization mechanism adds
        // a few bytes itself, the final layout ressembles more the following:
        //
        //   [symmetrically encrypted [serialized overhead/secret]][padding]
        //
        // The following calculates the overhead generated by serialization to
        // compute the right length for the secret key.
        //
        // Note that this overhead is only calculated once as it does not
        // depend on the nature of the secret key that will be generated.
        static elle::Natural32 overhead = _overhead();

        // 1) Compute the size of the secret key to generate by taking
        //    into account the padding size, if any.
        ELLE_ASSERT_GTE(static_cast<elle::Natural32>(
                          ::EVP_PKEY_bits(::EVP_PKEY_CTX_get0_pkey(context))),
                        (padding_size + overhead));

        // The maximum length, in bits, of the generated symmetric key.
        elle::Natural32 const ceiling = 512;
        elle::Natural32 const length =
          (::EVP_PKEY_bits(::EVP_PKEY_CTX_get0_pkey(context)) -
           (padding_size + overhead)) < ceiling ?
          ::EVP_PKEY_bits(::EVP_PKEY_CTX_get0_pkey(context)) -
          (padding_size + overhead) : ceiling;

        // Resolve back the cipher, mode and oneway.
        std::pair<Cipher, Mode> _cipher = cipher::resolve(cipher);
        Oneway _oneway = oneway::resolve(oneway);

        // 2) Generate a temporary secret key.
        SecretKey secret =
          secretkey::generate(length, _cipher.first, _cipher.second, _oneway);

        // 3) Cipher the plain text with the secret key.
        elle::Buffer data = secret.encipher(plain);

        // 4) Asymmetrically encrypt the secret with the given function
        //    and encryption context.

        // Serialize the secret.
        elle::Buffer buffer;

        {
          elle::IOStream stream(buffer.ostreambuf());
          elle::serialization::binary::SerializerOut output(stream, false); // XXX
          output.serialize("secret", secret);
        }

        ELLE_ASSERT_LTE(buffer.size() * 8, length + overhead);

        // Encrypt the secret key's archive.
        elle::Buffer key = raw::asymmetric::encrypt(buffer, context, function);

        // 5) Serialize the asymmetrically encrypted key and the symmetrically
        //    encrypted data.
        elle::Buffer code;

        {
          elle::IOStream _stream(code.ostreambuf());
          elle::serialization::binary::SerializerOut _output(_stream, false); // XXX
          _output.serialize("key", key);
          _output.serialize("data", data);
        }

        return (code);
      }

      elle::Buffer
      open(elle::ConstWeakBuffer const& code,
           ::EVP_PKEY_CTX* context,
           int (*function)(EVP_PKEY_CTX*,
                           unsigned char*,
                           size_t*,
                           const unsigned char*,
                           size_t))
      {
        ELLE_DEBUG_FUNCTION(context, function);
        ELLE_DUMP("code: %s", code);

        // Make sure the cryptographic system is set up.
        cryptography::require();

        // 1) Extract the key and ciphered data from the code which
        //    is supposed to be an archive.
        elle::Buffer key;
        elle::Buffer data;

        elle::IOStream stream(code.istreambuf());
        elle::serialization::binary::SerializerIn input(stream, false); // XXX

        input.serialize("key", key);
        input.serialize("data", data);

        // 2) Decrypt the key so as to reveal the symmetric secret key.

        // Decrypt the key.
        elle::Buffer buffer = raw::asymmetric::decrypt(key, context, function);

        // Finally extract the secret key since decrypted.
        elle::IOStream _stream(buffer.istreambuf());
        elle::serialization::binary::SerializerIn _input(_stream, false); // XXX
        SecretKey secret = _input.deserialize<SecretKey>("secret");

        // 3) Decrypt the data with the secret key.
        elle::Buffer clear = secret.decipher(data);

        ELLE_DUMP("clear: %s", clear);

        return (clear);
      }
    }
  }
}
