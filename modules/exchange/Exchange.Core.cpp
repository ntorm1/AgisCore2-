module;
#include <Eigen/Dense>
#include "AgisMacros.h"
#include "AgisDeclare.h"
#include <ankerl/unordered_dense.h>

module ExchangeModule;

import <filesystem>;
import <unordered_map>;

import PortfolioModule;
import AssetModule;
import AssetObserverModule;
import AgisArrayUtils;
import OrderModule;

namespace fs = std::filesystem;


namespace Agis
{

struct CovarianceMatrix
{
	CovarianceMatrix() {
		matrix = Eigen::MatrixXd::Zero(0, 0);
		step_size = 0;
		lookback = 0;
	}
	Eigen::MatrixXd matrix;
	size_t lookback;
	size_t step_size;
};

struct ExchangePrivate
{
	std::string exchange_id;
	size_t exchange_index;
	std::vector<std::string> columns;

	AssetFactory* asset_factory;
	std::vector<UniquePtr<Asset>> assets;
	std::unordered_map<std::string, size_t> asset_index_map;
	CovarianceMatrix covariance_matrix;
	std::vector<std::unique_ptr<Order>> orders;

	std::vector<long long> dt_index;
	long long current_dt = 0;
	size_t current_index = 0;
	std::string dt_format;
	bool on_close = false;

	ExchangePrivate(
		std::string exchange_id,
		size_t exchange_index,
		std::string dt_format);

	~ExchangePrivate();
};

//============================================================================
ExchangePrivate::ExchangePrivate(
	std::string _exchange_id,
	size_t _exchange_index,
	std::string _dt_format)
{
	exchange_id = _exchange_id;
	exchange_index = _exchange_index;
	dt_format = _dt_format;
	asset_factory = new AssetFactory(dt_format, exchange_id);
	covariance_matrix = CovarianceMatrix();
}

//============================================================================
ExchangePrivate::~ExchangePrivate()
{
	delete asset_factory;
}


//============================================================================
Exchange::Exchange(
	std::string exchange_id,
	size_t exchange_index,
	std::string dt_format,
	std::string source
) noexcept
{
	_source = source;
	_p = new ExchangePrivate(exchange_id, exchange_index, dt_format);
}


//============================================================================
std::expected<bool, AgisException> Exchange::load_folder() noexcept
{
	// get list of files in source directory
	auto source_path = fs::path(this->_source);
	auto files = fs::directory_iterator(source_path);
	
	// loop through files
	for (auto const& file : files)
	{
		// get file name minus the extension
		auto file_name = file.path().stem().string();
		AGIS_ASSIGN_OR_RETURN(asset, _p->asset_factory->create_asset(file_name, file.path().string()));
		_p->assets.push_back(std::move(asset));
	}

	return true;
}

//============================================================================
std::expected<bool, AgisException> Exchange::load_assets() noexcept
{
	// load source as fs path and check if it exists
	auto source_path = fs::path(this->_source);
	if (!fs::exists(source_path))
	{
		return std::unexpected(AgisException("Source path does not exist"));
	}

	// if source is a directory, call load_folder
	if (fs::is_directory(source_path))
	{
		AGIS_ASSIGN_OR_RETURN(res, this->load_folder());
	}
	else
	{
		return std::unexpected(AgisException("Source path is not a directory"));
	}
	for (auto& asset : _p->assets)
	{
		_p->asset_index_map.emplace(asset->get_id(), asset->get_index());
		// all assets registered to an exchange must have the same columns
		if(_p->columns.size() == 0)
		{
			_p->columns = asset->get_column_names();
		}
		else
		{
			auto asset_columns = asset->get_column_names();
			// if the two vectors are not equal, throw exception
			if (_p->columns != asset_columns)
			{
				return std::unexpected(AgisException("Asset columns do not match"));
			}
		}
	}
	this->build();
	return true;
}


//============================================================================
void
Exchange::register_portfolio(Portfolio* p) noexcept
{
	registered_portfolios.emplace(p->get_portfolio_index(), p);
}


//============================================================================
std::expected<bool, AgisException>
Exchange::step(long long global_dt) noexcept
{
	if(this->_p->current_index >= this->_p->dt_index.size())
	{
		return true;
	}
	_p->current_dt = this->_p->dt_index[this->_p->current_index];
	if (_p->current_dt != global_dt)
	{
		return true;
	}
	// move assets forward
	for (auto& asset : this->_p->assets)
	{
		asset->step(global_dt);
	}

	// flag portfolios to call next step
	for (auto& portfolio : registered_portfolios)
	{
		portfolio.second->_step_call = true;
	}
	_p->current_index++;
	return true;
}


//============================================================================
void
Exchange::reset() noexcept
{
	for (auto& asset : this->_p->assets)
	{
		asset->reset();
	}
	this->_p->current_index = 0;
}

//============================================================================
void Exchange::build() noexcept
{
	this->_p->dt_index.clear();

	for (auto& asset : this->_p->assets) 
	{
		this->_p->dt_index = sorted_union(
			this->_p->dt_index,
			asset->get_dt_index()
		);
	}
}


//============================================================================
std::vector<UniquePtr<Asset>>& Exchange::get_assets_mut() noexcept
{
	return _p->assets;
}

//============================================================================
size_t
Exchange::get_exchange_index() const noexcept
{
	return _p->exchange_index;
}


//============================================================================
std::optional<size_t>
Exchange::get_column_index(std::string const& column) const noexcept
{
	auto it = std::find(_p->columns.begin(), _p->columns.end(), column);
	if (it == _p->columns.end())
	{
		return std::nullopt;
	}
	return std::distance(_p->columns.begin(), it);
}


//============================================================================
long long Exchange::get_dt() const noexcept
{
	return _p->current_dt;
}


//============================================================================
std::string const&
Exchange::get_exchange_id() const noexcept
{
	return _p->exchange_id;
}


//============================================================================
std::string const&
Exchange::get_dt_format() const noexcept
{
	return _p->dt_format;
}


//============================================================================
std::optional<double>
Exchange::get_market_price(size_t asset_index) const noexcept
{
#ifdef _DEBUG
	if (asset_index < this->_index_offset)
	{
		return std::nullopt;
	}
#endif
	auto asset_index_offset = asset_index - this->_index_offset;
#ifdef _DEBUG
	if (asset_index_offset >= this->_p->assets.size())
	{
		return std::nullopt;
	}
#endif // DEBUG
	return this->_p->assets[asset_index_offset]->get_market_price(_p->on_close);
}


//============================================================================
std::optional<size_t>
Exchange::get_asset_index(std::string const& asset_id) const noexcept
{
	auto it = _p->asset_index_map.find(asset_id);
	if (it == _p->asset_index_map.end())
	{
		return std::nullopt;
	}
	return it->second;
}

//============================================================================
std::vector<long long> const&
Exchange::get_dt_index() const noexcept
{
	return _p->dt_index;
}


//============================================================================
std::expected<Asset const*, AgisException>
get_enclosing_asset(Asset const* a, Asset const* b)
{
	auto res = a->encloses(*b);
	if(a->encloses(*b))
	{
		return a;
	}
	else if (b->encloses(*a))
	{
		return b;
	}
	return std::unexpected(AgisException("Assets: " + a->get_id() + " and " + b->get_id() + " do not have a common dt index"));
}


//============================================================================
std::optional<double>
Exchange::get_covariance(size_t index1, size_t index2) const noexcept
{
	// make sure covariance matrix was initialized
	if (_p->covariance_matrix.matrix.rows() == 0)
	{
		return std::nullopt;
	}
	// make sure current row index is greater than lookback
	if (_p->current_index <= _p->covariance_matrix.lookback)
	{
		return std::nullopt;
	}
	return _p->covariance_matrix.matrix(index1, index2);
}


//============================================================================
std::expected<bool, AgisException>
Exchange::init_covariance_matrix(size_t lookback, size_t step_size) noexcept
{
	_p->covariance_matrix.matrix = Eigen::MatrixXd::Zero(_p->assets.size(), _p->assets.size());
	_p->covariance_matrix.lookback = lookback;
	_p->covariance_matrix.step_size = step_size;
	for (size_t i = 0; i < _p->assets.size(); i++)
	{
		for (size_t j = 0; j < i + 1; j++)
		{
			auto a1 = _p->assets[i].get();
			auto a2 = _p->assets[j].get();
			if (a1->rows() < lookback || a2->rows() < lookback)
			{
				continue;
			}
			auto enclosing_asset = get_enclosing_asset(a1, a2);
			if (!enclosing_asset) 
			{
				return std::unexpected(enclosing_asset.error());
			}
			if (j == i)
			{
				auto observer = std::make_unique<ReturnsVarianceObserver>(*a1, lookback);
				double* diagonal = &_p->covariance_matrix.matrix(i, j);
				observer->set_pointer(diagonal);
				a1->add_observer(std::move(observer));
				continue;
			}
			else
			{
				auto observer = std::make_unique<ReturnsCovarianceObserver>(*a1, *a2, lookback, step_size);
				double* upper_triangular = &_p->covariance_matrix.matrix(i, j);
				double* lower_triangular = &_p->covariance_matrix.matrix(j, i);
				observer->set_pointers(upper_triangular, lower_triangular);
				a1->add_observer(std::move(observer));
			}
		}
	}

	return true;
}


//============================================================================
std::vector<UniquePtr<Asset>> const&
Exchange::get_assets() const noexcept
{
	return _p->assets;
}


//============================================================================
Exchange::~Exchange()
{
	delete this->_p;
}


//============================================================================
UniquePtr<Exchange>
Exchange::create(
	std::string exchange_name,
	std::string dt_format,
	size_t exchange_index,
	std::string source)
{
	return std::make_unique<Exchange>(exchange_name, exchange_index, dt_format, source);
}


//============================================================================
void Exchange::destroy(Exchange* exchange) noexcept
{
	if (exchange != nullptr)
	{
		delete exchange;
	}
}


//============================================================================
std::optional<Asset const*>
Exchange::get_asset(size_t asset_index) const noexcept
{
	// prevent underflow
	if (asset_index < this->_index_offset)
	{
		return std::nullopt;
	}

	// get the asset index offset to the exchange
	auto asset_index_offset = asset_index - this->_index_offset;
	if (asset_index_offset >= this->_p->assets.size())
	{
		return std::nullopt;
	}
	return this->_p->assets[asset_index_offset].get();
}


//============================================================================
std::optional<Asset const*>
Exchange::get_asset(std::string const& asset_id) const noexcept
{
	auto index = this->get_asset_index(asset_id);
	if(!index) return std::nullopt;
	return this->get_asset(*index);
}


//============================================================================
std::vector<std::string> const&
Exchange::get_columns() const noexcept
{
	return _p->columns;
}


//============================================================================
std::optional<std::unique_ptr<Order>>
Exchange::place_order(std::unique_ptr<Order> order) noexcept
{
	// make sure order is valid
	if (!this->is_valid_order(order.get()))
	{
		order->reject(_p->current_dt);
		return std::move(order);
	}

	// attempt to fill order
	this->process_order(order.get());
	if (order->get_order_state() == OrderState::FILLED)
	{
		return std::move(order);
	}

	// otherwise store order in open orders
	order->set_order_state(OrderState::OPEN);
	this->_p->orders.push_back(std::move(order));
	return std::nullopt;
}


//============================================================================
void Exchange::process_market_order(Order* order) noexcept
{
	// get asset index
	auto asset_index = order->get_asset_index();
	asset_index -= this->_index_offset;

	// get asset
	auto asset = this->_p->assets[asset_index].get();

	// get asset price and fill if possible
	auto price = asset->get_market_price(_p->on_close);
	if (price)
	{
		order->fill(asset, price.value(), _p->current_dt);
	}
}


//============================================================================
void
Exchange::process_order(Order* order) noexcept
{
	switch (order->get_order_type())
	{
	case OrderType::MARKET_ORDER:
		this->process_market_order(order);
		break;
	}
}

//============================================================================
void
Exchange::process_orders(bool on_close) noexcept
{
	_p->on_close = on_close;
	for (auto orderIt = this->_p->orders.begin(); orderIt != this->_p->orders.end();)
	{
		auto& order = *orderIt;
		this->process_order(order.get());

		if (order->get_order_state() != OrderState::OPEN) {
			auto portfolio_index = order->get_portfolio_index();
			auto portfolio = registered_portfolios[portfolio_index];
			portfolio->process_order(std::move(order));

			// swap current order with last order and pop back
			std::iter_swap(orderIt, this->_p->orders.rbegin());
			this->_p->orders.pop_back();
		}
		else 
		{
			++orderIt;
		}
	}
}


//============================================================================
bool
Exchange::is_valid_order(Order const* order) const noexcept
{
	// validate order state, must be pending
	if (order->get_order_state() != OrderState::PENDING) 
	{
		return false;
	}

	// validate asset index 
	auto asset_index = order->get_asset_index();
	if (asset_index < this->_index_offset)
	{
		return false;
	}
	asset_index -= _index_offset;
	if (asset_index >= this->_p->assets.size())
	{
		return false;
	}

	// validate portfolio index
	auto portfolio_index = order->get_portfolio_index();
	if (registered_portfolios.find(portfolio_index) == registered_portfolios.end())
	{
		return false;
	}

	// validate asset is streaming
	auto& asset = this->_p->assets[asset_index];
	if (asset->get_state() != AssetState::STREAMING)
	{
		return false;
	}
	return true;
}


}