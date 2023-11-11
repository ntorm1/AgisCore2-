module;

#include <type_traits>

export module AgisPointer;

import <optional>;
import <memory>;

namespace Agis
{


//==================================================================================================
export template <typename T>
class NullableUniquePtr
{
private:
	std::unique_ptr<T> ptr;


public:
    explicit NullableUniquePtr(std::unique_ptr<T> other) noexcept : ptr(std::move(other))
	{}

    std::optional<T*> get() const noexcept
    {
        if (ptr)
		{
			return ptr.get();
		}
        return std::nullopt;
    }

    T* unwrap() const noexcept
	{
        return ptr.get();
	}

    // add boolen operator
    explicit operator bool() const noexcept
	{
		return ptr != nullptr;
	}
};


//==================================================================================================
export template <typename T>
class NonNullUniquePtr
{
    std::unique_ptr<T> ptr;

    explicit NonNullUniquePtr(std::unique_ptr<T>&& other) noexcept: ptr(std::move(other))
	{}

public:
    template <typename... Args>
    static std::optional<NonNullUniquePtr<T>> create(Args&&... args) noexcept
    {
        static_assert(std::is_nothrow_constructible<T, Args...>::value,
            "T must have a noexcept constructor");
        auto p = std::make_unique<T>(std::forward<Args>(args)...);
        if (!p)
		{
			return std::nullopt;
		}
		return NonNullUniquePtr<T>(std::move(p));
    }

    T* operator->() const noexcept
    {
        return ptr.get();
    }


    T& operator*() const noexcept
	{
		return *ptr;
	}
};


//==================================================================================================
export template <typename T>
class NonNullPtr
{
private:
    T* ptr;
    bool owns;

    template <typename... Args>
    NonNullPtr(Args&&... args) : ptr(new T(std::forward<Args>(args)...)), owns(true)
    {}

    NonNullPtr(T* other) : ptr(other), owns(false)
	{}

    NonNullPtr(std::unique_ptr<T>&& other) : ptr(other.release()), owns(true)
    {}

    NonNullPtr(std::unique_ptr<T>& other) : ptr(other.get()), owns(false)
    {}


public:
    ~NonNullPtr()
    {
        if (owns)
        {
            delete ptr;
        }
    }

    template <typename... Args>
    static std::optional<NonNullPtr<T>> create(Args&&... args) noexcept
    {
        auto p = NonNullPtr<T>(std::forward<Args>(args)...);
        if (!p)
        {
            return std::nullopt;
        }
        return p;
    }

    // Move constructor
    NonNullPtr(NonNullPtr&& other) noexcept : ptr(other.ptr) 
    {
        other.ptr = nullptr;
    }

    // Move assignment operator
    NonNullPtr& operator=(NonNullPtr&& other) noexcept 
    {
        if (this != &other) {
            delete ptr;
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    // pointer operator
    T* operator->() const noexcept 
    {
		return ptr;
	}

    // dereference operator
    T& operator*() const noexcept 
    {
        return *ptr;
    }
};


}