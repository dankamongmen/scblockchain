#include <libcatena/truststore.h>
#include <libcatena/utility.h>

namespace Catena {

std::ostream& operator<<(std::ostream& s, const TrustStore& ts){
	for(const auto& k : ts.keys){
		const KeyLookup& kl = k.first;
		s << "hash: ";
	       	HexOutput(s, kl.first) << " idx: " << kl.second << "\n";
	}
	return s;
}

}
