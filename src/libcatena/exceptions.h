#ifndef CATENA_LIBCATENA_EXCEPTIONS
#define CATENA_LIBCATENA_EXCEPTIONS

namespace Catena {

class CatenaException : public std::runtime_error {
public:
CatenaException(const std::string& s) : std::runtime_error(s){}
};

class NetworkException : public CatenaException {
public:
NetworkException() : CatenaException("network error"){}
NetworkException(const std::string& s) : CatenaException(s){}
};

class BlockValidationException : public CatenaException {
public:
BlockValidationException() : CatenaException("error validating block"){}
BlockValidationException(const std::string& s) : CatenaException(s){}
};

class BlockHeaderException : public CatenaException {
public:
BlockHeaderException() : CatenaException("invalid block header"){}
BlockHeaderException(const std::string& s) : CatenaException(s){}
};

class SplitInputException : public CatenaException {
public:
SplitInputException(const std::string& s) : CatenaException(s){}
};

class ConvertInputException : public CatenaException {
public:
ConvertInputException(const std::string& s) : CatenaException(s){}
};

class SigningException : public CatenaException {
public:
SigningException() : CatenaException("error signing"){}
SigningException(const std::string& s) : CatenaException(s){}
};

class EncryptException : public CatenaException {
public:
EncryptException() : CatenaException("error encrypting"){}
EncryptException(const std::string& s) : CatenaException(s){}
};

class DecryptException : public CatenaException {
public:
DecryptException() : CatenaException("error decrypting"){}
DecryptException(const std::string& s) : CatenaException(s){}
};

class KeypairException : public CatenaException {
public:
KeypairException() : CatenaException("keypair error"){}
KeypairException(const std::string& s) : CatenaException(s){}
};

class InvalidTXSpecException : public CatenaException {
public:
InvalidTXSpecException() : CatenaException("bad transaction spec"){}
InvalidTXSpecException(const std::string& s) : CatenaException(s){}
};

class UserStatusException : public CatenaException {
public:
UserStatusException() : CatenaException("bad user status"){}
UserStatusException(const std::string& s) : CatenaException(s){}
};

class TransactionException : public CatenaException {
public:
TransactionException() : CatenaException("invalid transaction"){}
TransactionException(const std::string& s) : CatenaException(s){}
};

class SystemException : public CatenaException {
public:
SystemException() : CatenaException("system error"){}
SystemException(const std::string& s) : CatenaException(s){}
};

}

#endif
