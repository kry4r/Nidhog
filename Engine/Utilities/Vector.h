#pragma once

#include "CommonHeaders.h"

namespace nidhog::utl
{
	//类似于 std::vector 的向量类，具有基本功能
	//可以在模板参数中指定是否希望在删除元素时
	//或清除/破坏向量时调用元素的析构函数

	template<typename T, bool destruct = true>
	class vector
	{
    public:
        // 默认构造函数，不分配内存
        vector() = default;

        // 构造函数调整vector的大小并初始化“count”项。
        constexpr explicit vector(u64 count)
        {
            resize(count);
        }

        // 构造函数调整vector的大小并使用“value”初始化“count”项
        constexpr explicit vector(u64 count, const T& value)
        {
            resize(count, value);
        }

        template<typename it, typename = std::enable_if_t<std::_Is_iterator_v<it>>>
        constexpr explicit vector(it first, it last)
        {
            for (; first != last; ++first)
            {
                emplace_back(*first);
            }
        }

        // 拷贝构造函数，通过复制另一个vector来构造
        // 所复制vector中的item必须是可复制的
        constexpr vector(const vector& o)
        {
            *this = o;
        }

        // 移动构造函数，通过move另一个vector来构造
        // move后原vector将为空
        constexpr vector(const vector&& o)
            : _capacity{ o._capacity }, _size{ o._size }, _data{ o._data }
        {
            o.reset();
        }

        // Copy复制赋值运算符，清除此vector并从其他vector复制
        // 必须是可复制的
        constexpr vector& operator=(const vector& o)
        {
            assert(this != std::addressof(o));
            if (this != std::addressof(o))
            {
                clear();
                reserve(o._size);
                for (auto& item : o)
                {
                    emplace_back(item);
                }
                assert(_size == o._size);
            }

            return *this;
        }

        // 移动运算符
        // 释放该vector中的所有资源并将另一个vector移动到该vector中
        constexpr vector& operator=(vector&& o)
        {
            assert(this != std::addressof(o));
            if (this != std::addressof(o))
            {
                destroy();
                move(o);
            }

            return *this;
        }
        
        // 销毁模板参数中指定的vector及其item
        ~vector() { destroy(); }

        // 通过复制“value”在vector末尾插入一个item
        constexpr void push_back(const T& value)
        {
            emplace_back(value);
        }

        // 通过move“value”在vector末尾插入一个item
        constexpr void push_back(T&& value)
        {
            emplace_back(std::move(value));
        }

        // Copy- or move-constructs an item at the end of the vector.
        template<typename... params>
        constexpr decltype(auto) emplace_back(params&&... p)
        {
            if (_size == _capacity)
            {
                reserve(((_capacity + 1) * 3) >> 1); // reserve 50% more
            }
            assert(_size < _capacity);

            T *const item{ new (std::addressof(_data[_size])) T(std::forward<params>(p)...) };
            ++_size;
            return *item;
        }

        // 调整vector的大小并使用默认值初始化新item。
        constexpr void resize(u64 new_size)
        {
            static_assert(std::is_default_constructible_v<T>,
                "Type must be default-constructible.");

            if (new_size > _size)
            {
                reserve(new_size);
                while (_size < new_size)
                {
                    emplace_back();
                }
            }
            else if (new_size < _size)
            {
                if constexpr (destruct)
                {
                    destruct_range(new_size, _size);
                }
                _size = new_size;
            }

            // Do nothing if new_size == _size.
            assert(new_size == _size);
        }

        // 调整vector的大小并使用copy"value"初始化新item。
        constexpr void resize(u64 new_size, const T& value)
        {
            static_assert(std::is_copy_constructible_v<T>,
                "Type must be copy-constructible.");

            if (new_size > _size)
            {
                reserve(new_size);
                while (_size < new_size)
                {
                    emplace_back(value);
                }
            }
            else if (new_size < _size)
            {
                if constexpr (destruct)
                {
                    destruct_range(new_size, _size);
                }
                _size = new_size;
            }

            // Do nothing if new_size == _size.
            assert(new_size == _size);
        }

        // 分配内存以包含指定数量的items.
        constexpr void reserve(u64 new_capacity)
        {
            if (new_capacity > _capacity)
            {
                // NOTE: 如果分配了新的内存区域
                //       realoc()会自动复制缓冲区中的数据
                void* new_buffer{ realloc(_data, new_capacity * sizeof(T)) };
                assert(new_buffer);
                if (new_buffer)
                {
                    _data = static_cast<T*>(new_buffer);
                    _capacity = new_capacity;
                }
            }
        }

        // 删除指定索引处的item
        constexpr T *const erase(u64 index)
        {
            assert(_data && index < _size);
            return erase(std::addressof(_data[index]));
        }

        // 删除指定位置的item
        constexpr T *const erase(T *const item)
        {
            assert(_data && item >= std::addressof(_data[0]) &&
                item < std::addressof(_data[_size]));
            if constexpr (destruct) item->~T();
            --_size;
            if (item < std::addressof(_data[_size]))
            {
                memcpy(item, item + 1, (std::addressof(_data[_size]) - item) * sizeof(T));
            }

            return item;
        }

        // 和erase()差不多但是更快，只复制最后一个item
        constexpr T *const erase_unordered(u64 index)
        {
            assert(_data && index < _size);
            return erase_unordered(std::addressof(_data[index]));
        }

        // 和erase()差不多但是更快，只复制最后一个item
        constexpr T *const erase_unordered(T *const item)
        {
            assert(_data && item >= std::addressof(_data[0]) &&
                item < std::addressof(_data[_size]));
            if constexpr (destruct) item->~T();
            --_size;
            if (item < std::addressof(_data[_size]))
            {
                memcpy(item, std::addressof(_data[_size]), sizeof(T));
            }

            return item;
        }

        // 清除vector并销毁模板参数中指定的item
        constexpr void clear()
        {
            if constexpr (destruct)
            {
                destruct_range(0, _size);
            }
            _size = 0;
        }

        // Swaps two vectors
        constexpr void swap(vector& o)
        {
            if (this != std::addressof(o))
            {
                //使用move提高效率
                auto temp(std::move(o));
                o.move(*this);
                move(temp);
            }
        }

        // 指向数据开始的指针。 可能为空。
        [[nodiscard]] constexpr T* data()
        {
            return _data;
        }

        // 指向数据开始的指针。 可能为空。
        [[nodiscard]] constexpr T *const data() const
        {
            return _data;
        }

        // Returns true if vector is empty.
        [[nodiscard]] constexpr bool empty() const
        {
            return _size == 0;
        }

        // Return the number of items in the vector.
        [[nodiscard]] constexpr u64 size() const
        {
            return _size;
        }

        // Returns the current capacity of the vector.
        [[nodiscard]] constexpr u64 capacity() const
        {
            return _capacity;
        }

        // Indexing operator. 返回对指定索引处的item的引用
        [[nodiscard]] constexpr T& operator[](u64 index)
        {
            assert(_data && index < _size);
            return _data[index];
        }


        // Indexing operator. 返回对指定索引处的item的引用
        [[nodiscard]] constexpr const T& operator[](u64 index) const
        {
            assert(_data && index < _size);
            return _data[index];
        }

        // 返回对第一个item的索引 
        // 当vector为空时，调用出错
        [[nodiscard]] constexpr T& front()
        {
            assert(_data && _size);
            return _data[0];
        }

        // 返回对第一个item的索引 
        // 当vector为空时，调用出错
        [[nodiscard]] constexpr const T& front() const
        {
            assert(_data && _size);
            return _data[0];
        }

        // 返回对第一个item的索引 
        // 当vector为空时，调用出错
        [[nodiscard]] constexpr T& back()
        {
            assert(_data && _size);
            return _data[_size - 1];
        }

        // 返回对第一个item的索引 
        // 当vector为空时，调用出错
        [[nodiscard]] constexpr const T& back() const
        {
            assert(_data && _size);
            return _data[_size - 1];
        }

        // 返回指向第一个item的指针. Returns null when vector is empty.
        [[nodiscard]] constexpr T* begin()
        {
            assert(_data);
            return std::addressof(_data[0]);
        }

        // 返回指向第一个item的指针. Returns null when vector is empty.
        [[nodiscard]] constexpr const T* begin() const
        {
            assert(_data);
            return std::addressof(_data[0]);
        }

        // 返回指向last item的指针. Returns null when vector is empty.
        [[nodiscard]] constexpr T* end()
        {
            assert(_data);
            return std::addressof(_data[_size]);
        }

        // 返回指向last item的指针. Returns null when vector is empty.
        [[nodiscard]] constexpr const T* end() const
        {
            assert(_data);
            return std::addressof(_data[_size]);
        }

    private:
        constexpr void move(vector& o)
        {
            _capacity = o._capacity;
            _size = o._size;
            _data = o._data;
            o.reset();
        }

        constexpr void reset()
        {
            _capacity = 0;
            _size = 0;
            _data = nullptr;
        }

        constexpr void destruct_range(u64 first, u64 last)
        {
            assert(destruct);
            assert(first <= _size && last <= _size && first <= last);
            if (_data)
            {
                for (; first != last; ++first)
                {
                    _data[first].~T();
                }
            }
        }

        constexpr void destroy()
        {
            assert([&] {return _capacity ? _data != nullptr : _data == nullptr; }());
            clear();
            _capacity = 0;
            if (_data) free(_data);
            _data = nullptr;
        }
        
        u64 _capacity{ 0 };
        u64 _size{ 0 };
        T*  _data{ nullptr };

	};
}