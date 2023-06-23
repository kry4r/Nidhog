#pragma once

#include "CommonHeaders.h"


namespace nidhog::utl
{

#if USE_STL_VECTOR
#pragma message("WARNING: using utl::free_list with std::vector result in duplicate calls to class constructor!")
#endif

	template<typename T> 
	class free_list
	{
		//ȷ����С��32λ
		static_assert(sizeof(T) >= sizeof(u32));
	public:
        free_list() = default;
		explicit free_list(u32 count)
		{
			//��reserveһЩ�ڴ�
			_array.reserve(count);
		}
		~free_list()
		{
			//����������Ƿ�Ϊ0
			assert(!_size);
#if USE_STL_VECTOR
            memset(_array.data(), 0, _array.size() * sizeof(T));
#endif
		}

        template<class... params>
        constexpr u32 add(params&&... p)
        {
            u32 id{ u32_invalid_id };
            //��鵱ǰ�Ƿ��п��ò��
            if (_next_free_index == u32_invalid_id)
            {
                id = (u32)_array.size();
                _array.emplace_back(std::forward<params>(p)...);
            }
            else
            {
                //��û�У����·���һ��
                id = _next_free_index;
                assert(id < _array.size() && already_removed(id));
                _next_free_index = *(const u32 *const)std::addressof(_array[id]);
                new (std::addressof(_array[id])) T(std::forward<params>(p)...);
            }
            ++_size;
            return id;
        }

        constexpr void remove(u32 id)
        {
            assert(id < _array.size() && !already_removed(id));
            T& item{ _array[id] };
            item.~T();
            //���δ��ʼ������
            DEBUG_OP(memset(std::addressof(_array[id]), 0xcc, sizeof(T)));
            *(u32 *const)std::addressof(_array[id]) = _next_free_index;
            _next_free_index = id;
            --_size;
        }

        constexpr u32 size() const
        {
            return _size;
        }

        constexpr u32 capacity() const
        {
            return _array.size();
        }

        constexpr u32 empty() const
        {
            return _size == 0;
        }

        //����free list�е���Ŀ
        [[nodiscard]] constexpr T& operator[](u32 id)
        {
            assert(id < _array.size() && !already_removed(id));
            return _array[id];
        }

        [[nodiscard]] constexpr const T& operator[](u32 id) const
        {
            assert(id < _array.size() && !already_removed(id));
            return _array[id];
        }

	private:
        //������ĸ��ֽڴ�Ž������������ò�ۺ��Ƿ���ʣ��
        constexpr bool already_removed(u32 id)
        {
            // NOTE: when sizeof(T) == sizeof(u32) ���ǾͲ�����item�Ѿ�remove������²���
            if constexpr (sizeof(T) > sizeof(u32))
            {
                u32 i{ sizeof(u32) }; //����ǰ��4 bytes.
                const u8 *const p{ (const u8 *const)std::addressof(_array[id]) };
                //�������ｫ���ʼ��Ϊ��Ч���ݣ����������������������ʵ���Լ���vector����ֹ��һ��Ϊ
                while ((p[i] == 0xcc) && (i < sizeof(T))) ++i;
                return i == sizeof(T);
            }
            else
            {
                return true;
            }
        }
#if USE_STL_VECTOR
        utl::vector<T>          _array;
#else
        utl::vector<T, false>   _array;
#endif
        u32                     _next_free_index{ u32_invalid_id };
        u32                     _size{ 0 };//ʹ�ò������
	};
}