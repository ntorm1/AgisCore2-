module;

#include "AgisMacros.h"
#include "AgisDeclare.h"

module HydraModule;

import ExchangeMapModule;

namespace Agis
{

struct HydraPrivate
{
	ExchangeMap exchanges;
};


//============================================================================
Hydra::Hydra()
{
	_p = new HydraPrivate();
}


//============================================================================
Hydra::~Hydra()
{
	delete _p;
}


//============================================================================
std::expected<bool, AgisException>
Hydra::build() noexcept
{
	AGIS_ASSIGN_OR_RETURN(res, _p->exchanges.build());
}


//============================================================================
ExchangeMap const&
Hydra::get_exchanges() const noexcept
{
	return _p->exchanges;
}


//============================================================================
std::vector<long long>
const& Hydra::get_dt_index() const noexcept
{
	return _p->exchanges.get_dt_index();
}


//============================================================================
bool
Hydra::asset_exists(std::string const& asset_id) const noexcept
{
	return _p->exchanges.asset_exists(asset_id);
}


//============================================================================
std::expected<Exchange const*, AgisException>
Hydra::create_exchange(std::string exchange_id, std::string dt_format, std::string source)
{
	return _p->exchanges.create_exchange(exchange_id, dt_format, source);
}


}