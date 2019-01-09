#ifndef CATENA_LIBCATENA_EXCEPTIONS
#define CATENA_LIBCATENA_EXCEPTIONS

namespace Catena {

class NetworkException : public std::runtime_error {
public:
NetworkException() : std::runtime_error("network error"){}
NetworkException(const std::string& s) : std::runtime_error(s){}
};

class BlockValidationException : public std::runtime_error {
public:
BlockValidationException() : std::runtime_error("error validating block"){}
BlockValidationException(const std::string& s) : std::runtime_error(s){}
};

class BlockHeaderException : public std::runtime_error {
public:
BlockHeaderException() : std::runtime_error("invalid block header"){}
BlockHeaderException(const std::string& s) : std::runtime_error(s){}
};

class SplitInputException : public std::runtime_error {
public:
SplitInputException(const std::string& s) : std::runtime_error(s){}
};

class ConvertInputException : public std::runtime_error {
public:
ConvertInputException(const std::string& s) : std::runtime_error(s){}
};

class SigningException : public std::runtime_error {
public:
SigningException() : std::runtime_error("error signing"){}
SigningException(const std::string& s) : std::runtime_error(s){}
};

class EncryptException : public std::runtime_error {
public:
EncryptException() : std::runtime_error("error encrypting"){}
EncryptException(const std::string& s) : std::runtime_error(s){}
};

class DecryptException : public std::runtime_error {
public:
DecryptException() : std::runtime_error("error decrypting"){}
DecryptException(const std::string& s) : std::runtime_error(s){}
};

class KeypairException : public std::runtime_error {
public:
KeypairException() : std::runtime_error("keypair error"){}
KeypairException(const std::string& s) : std::runtime_error(s){}
};

}

#endif
