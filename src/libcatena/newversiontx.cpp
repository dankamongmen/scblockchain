#include <libcatena/newversiontx.h>

namespace Catena {

bool NewVersionTX::Extract(const unsigned char* data __attribute__ ((unused)),
			unsigned len __attribute__ ((unused))){
	return false;
}

std::ostream& NewVersionTX::TXOStream(std::ostream& s) const {
	return s << "NewVersion";
}

nlohmann::json NewVersionTX::JSONify() const {
	return nlohmann::json({{"type", "NewVersion"}});
}

std::pair<std::unique_ptr<unsigned char[]>, size_t> NewVersionTX::Serialize() const {
	std::unique_ptr<unsigned char[]> ret(new unsigned char[2]);
	TXType_to_nbo(TXTypes::NewVersion, ret.get());
	return std::make_pair(std::move(ret), 2);
}

}
