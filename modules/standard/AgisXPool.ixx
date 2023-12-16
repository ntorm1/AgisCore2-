module;
#pragma once
#include <type_traits>
export module AgisXPool;

import <vector>;
import <memory>;
import <shared_mutex>;
import <atomic>;

namespace Agis
{


template <typename T>
concept DefaultConstructible = std::is_default_constructible_v<T>;

template <typename T, typename = void>
struct has_init : std::false_type {};

template <typename T, typename... Args>
concept PoolFunctions = requires(T t, Args&&... args) {
	{ t.reset() } -> std::same_as<void>; // requires reset function
};

export template <typename T>
requires DefaultConstructible<T> && PoolFunctions<T>
class ObjectPool
{

private:
	std::vector<std::unique_ptr<T>> _objects;
	std::shared_mutex _mutex;
	std::atomic<size_t> _index = 0;
public:
	ObjectPool(size_t n)
	{
		_objects.reserve(n);
		for (size_t i = 0; i < n; i++)
		{
			_objects.push_back(std::make_unique<T>());
		}
	}

	void reset()
	{
		for (auto& obj : _objects)
		{
			if (!obj)
			{
				obj = std::make_unique<T>();
			}
			else
			{
				obj->reset();
			}
		}
		_index = 0;
	}

	template <typename... Args>
	std::unique_ptr<T> pop_unique(Args&&... args)
	{
		size_t item_index = _index.fetch_add(1);
		if (item_index >= _objects.size())
		{
			_mutex.lock();
			// Double the capacity and fill with default-constructed objects
			_objects.resize((_objects.size() + 1) * 2);

			for (size_t i = item_index; i < _objects.size(); ++i)
			{
				_objects[i] = std::make_unique<T>();
			}
			_mutex.unlock();
			std::unique_ptr<T> obj = std::move(_objects[item_index]);
			_objects[item_index] = nullptr;
			obj->init(std::forward<Args>(args)...);
			return std::move(obj);
		}
		else
		{
			std::unique_ptr<T> obj = std::move(_objects[item_index]);
			_objects[item_index] = nullptr;
			obj->init(std::forward<Args>(args)...);
			return std::move(obj);
		}
	}

	template <typename... Args>
	T* get(Args&&... args)
	{
		size_t item_index = _index.fetch_add(1);
		if (item_index >= _objects.size())
		{
			_mutex.lock();
			_objects.resize((_objects.size() + 1) * 2);
			for (size_t i = item_index; i < _objects.size(); ++i)
			{
				_objects[i] = std::make_unique<T>();
			}
			_mutex.unlock();
			T* obj = _objects[item_index].get();
			obj->init(std::forward<Args>(args)...);
			return obj;
		}
		else
		{
			T* obj = _objects[item_index].get();
			obj->init(std::forward<Args>(args)...);
			return obj;
		}
	}
};

}