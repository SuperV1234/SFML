#pragma once
#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Base/Assert.hpp"
#include "SFML/Base/Builtins/Memcpy.hpp"
#include "SFML/Base/Launder.hpp"
#include "SFML/Base/PlacementNew.hpp"
#include "SFML/Base/SizeT.hpp"
#include "SFML/Base/Traits/IsTriviallyCopyable.hpp"
#include "SFML/Base/Traits/IsTriviallyDestructible.hpp"


namespace sf::base
{
////////////////////////////////////////////////////////////
template <typename TItem>
class [[nodiscard]] TrivialVector
{
private:
    union [[nodiscard]] ItemUnion
    {
        TItem item;

        [[gnu::always_inline]] ItemUnion()
        {
        }
    };

    static_assert(SFML_BASE_IS_TRIVIALLY_COPYABLE(TItem));
    static_assert(SFML_BASE_IS_TRIVIALLY_DESTRUCTIBLE(TItem));

    static_assert(sizeof(ItemUnion) == sizeof(TItem));
    static_assert(alignof(ItemUnion) == alignof(TItem));

    static_assert(SFML_BASE_IS_TRIVIALLY_COPYABLE(ItemUnion));
    static_assert(SFML_BASE_IS_TRIVIALLY_DESTRUCTIBLE(ItemUnion));

    ItemUnion* m_data{nullptr};
    TItem*     m_endSize{nullptr};
    TItem*     m_endCapacity{nullptr};


    ////////////////////////////////////////////////////////////
    [[gnu::cold, gnu::noinline]] void reserveImpl(base::SizeT targetCapacity)
    {
        auto*      newData = new ItemUnion[targetCapacity];
        const auto oldSize = size();

        if (m_data != nullptr)
        {
            SFML_BASE_MEMCPY(newData, m_data, sizeof(ItemUnion) * oldSize);
            delete[] m_data;
        }
        else
        {
            SFML_BASE_ASSERT(size() == 0);
            SFML_BASE_ASSERT(capacity() == 0);
        }

        m_data        = newData;
        m_endSize     = data() + oldSize;
        m_endCapacity = data() + targetCapacity;
    }

public:
    ////////////////////////////////////////////////////////////
    [[nodiscard]] TrivialVector() = default;


    ////////////////////////////////////////////////////////////
    ~TrivialVector()
    {
        delete[] m_data;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit TrivialVector(base::SizeT initialSize) :
    m_data{new ItemUnion[initialSize]},
    m_endSize{data() + initialSize},
    m_endCapacity{data() + initialSize}
    {
        for (TItem* p = data(); p < m_endSize; ++p)
            SFML_BASE_PLACEMENT_NEW(p) TItem();
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit TrivialVector(const TItem* src, base::SizeT srcCount) :
    m_data{new ItemUnion[srcCount]},
    m_endSize{data() + srcCount},
    m_endCapacity{data() + srcCount}
    {
        for (TItem* p = data(); p < m_endSize; ++p)
            SFML_BASE_PLACEMENT_NEW(p) TItem(*src++);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] TrivialVector(const TrivialVector& rhs) :
    m_data{rhs.m_data == nullptr ? nullptr : new ItemUnion[rhs.size()]},
    m_endSize{data() + rhs.size()},
    m_endCapacity{data() + rhs.size()}
    {
        SFML_BASE_MEMCPY(m_data, rhs.m_data, sizeof(TItem) * rhs.size());
    }


    ////////////////////////////////////////////////////////////
    TrivialVector& operator=(const TrivialVector& rhs)
    {
        if (this == &rhs)
            return *this;

        reserve(rhs.size());
        m_endSize = data() + rhs.size();

        SFML_BASE_MEMCPY(m_data, rhs.m_data, sizeof(TItem) * rhs.size());

        return *this;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] TrivialVector(TrivialVector&& rhs) noexcept :
    m_data{rhs.m_data},
    m_endSize{rhs.m_endSize},
    m_endCapacity{rhs.m_endCapacity}
    {
        rhs.m_data = nullptr;
    }


    ////////////////////////////////////////////////////////////
    TrivialVector& operator=(TrivialVector&& rhs) noexcept
    {
        if (this == &rhs)
            return *this;

        delete[] m_data;

        m_data        = rhs.m_data;
        m_endSize     = rhs.m_endSize;
        m_endCapacity = rhs.m_endCapacity;

        rhs.m_data = nullptr;

        return *this;
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void resize(const SizeT n)
    {
        const auto oldSize = size();

        reserve(n);
        m_endSize = data() + n;

        for (auto* p = data() + oldSize; p < m_endSize; ++p)
            SFML_BASE_PLACEMENT_NEW(p) TItem();
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void resize(const SizeT n, const TItem& item)
    {
        const auto oldSize = size();

        reserve(n);
        m_endSize = data() + n;

        for (auto* p = data() + oldSize; p < m_endSize; ++p)
            SFML_BASE_PLACEMENT_NEW(p) TItem(item);
    }


    ////////////////////////////////////////////////////////////
    void shrinkToFit()
    {
        reserveImpl(size());
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] TItem*& reserveMore(const SizeT n)
    {
        const SizeT targetCapacity = size() + n;

        if (capacity() < targetCapacity) [[unlikely]]
            reserveImpl((targetCapacity * 3u / 2u) + n);

        return m_endSize;
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] TItem*& reserve(const SizeT targetCapacity)
    {
        if (capacity() < targetCapacity) [[unlikely]]
            reserveImpl(targetCapacity);

        return m_endSize;
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] void assignRange(const TItem* b, const TItem* e) noexcept
    {
        SFML_BASE_ASSERT(b != nullptr);
        SFML_BASE_ASSERT(e != nullptr);
        SFML_BASE_ASSERT(b <= e);

        const auto count = static_cast<SizeT>(e - b);

        reserve(count);

        SFML_BASE_MEMCPY(m_data, b, sizeof(TItem) * count);
        unsafeSetSize(count);
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] void unsafeEmplaceRange(const TItem* ptr, SizeT count) noexcept
    {
        SFML_BASE_ASSERT(size() + count <= capacity());
        SFML_BASE_ASSERT(m_data != nullptr);
        SFML_BASE_ASSERT(m_endSize != nullptr);

        SFML_BASE_MEMCPY(m_endSize, ptr, sizeof(TItem) * count);

        m_endSize += count;
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] void emplaceRange(const TItem* ptr, SizeT count) noexcept
    {
        reserveMore(count);
        unsafeEmplaceRange(ptr, count);
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] void unsafeEmplaceOther(const TrivialVector& rhs) noexcept
    {
        unsafeEmplaceRange(rhs.data(), rhs.size());
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void clear() noexcept
    {
        m_endSize = data();
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] SizeT size() const noexcept
    {
        return static_cast<SizeT>(m_endSize - data());
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] SizeT capacity() const noexcept
    {
        return static_cast<SizeT>(m_endCapacity - data());
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] bool empty() const noexcept
    {
        return data() == m_endSize;
    }


    ////////////////////////////////////////////////////////////
    template <typename... Ts>
    [[gnu::always_inline, gnu::flatten]] void unsafeEmplaceBack(Ts&&... xs)
    {
        SFML_BASE_ASSERT(size() <= capacity());
        SFML_BASE_ASSERT(m_data != nullptr);
        SFML_BASE_ASSERT(m_endSize != nullptr);

        SFML_BASE_PLACEMENT_NEW(m_endSize++) TItem(static_cast<Ts&&>(xs)...);
    }


    ////////////////////////////////////////////////////////////
    template <typename... Ts>
    [[gnu::always_inline, gnu::flatten]] void emplaceBack(Ts&&... xs)
    {
        reserveMore(1);
        unsafeEmplaceBack(static_cast<Ts&&>(xs)...);
    }


    ////////////////////////////////////////////////////////////
    template <typename T>
    [[gnu::always_inline, gnu::flatten]] void pushBack(T&& x)
    {
        reserveMore(1);
        unsafeEmplaceBack(static_cast<T&&>(x));
    }


    ////////////////////////////////////////////////////////////
    template <typename... TItems>
    [[gnu::always_inline, gnu::flatten]] void unsafePushBackMultiple(TItems&&... items)
    {
        SFML_BASE_ASSERT(size() + sizeof...(items) <= capacity());
        SFML_BASE_ASSERT(m_data != nullptr);
        SFML_BASE_ASSERT(m_endSize != nullptr);

        (SFML_BASE_PLACEMENT_NEW(m_endSize++) TItem(static_cast<TItems&&>(items)), ...);
    }


    ////////////////////////////////////////////////////////////
    template <typename... TItems>
    [[gnu::always_inline, gnu::flatten]] void pushBackMultiple(TItems&&... items)
    {
        reserveMore(sizeof...(items));
        unsafePushBackMultiple(static_cast<TItems&&>(items)...);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] TItem* data() noexcept
    {
        return SFML_BASE_LAUNDER_CAST(TItem*, m_data);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] const TItem* data() const noexcept
    {
        return SFML_BASE_LAUNDER_CAST(const TItem*, m_data);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] TItem& operator[](const SizeT i) noexcept
    {
        SFML_BASE_ASSERT(i < size());
        SFML_BASE_ASSERT(m_data != nullptr);

        return *(data() + i);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] const TItem& operator[](const SizeT i) const noexcept
    {
        SFML_BASE_ASSERT(i < size());
        SFML_BASE_ASSERT(m_data != nullptr);

        return *(data() + i);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] TItem* begin() noexcept
    {
        return data();
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] const TItem* begin() const noexcept
    {
        return data();
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] TItem* end() noexcept
    {
        return m_endSize;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] const TItem* end() const noexcept
    {
        return m_endSize;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] const TItem* cbegin() const noexcept
    {
        return data();
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] const TItem* cend() const noexcept
    {
        return m_endSize;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] TItem& front() noexcept
    {
        return this->operator[](0u);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] const TItem& front() const noexcept
    {
        return this->operator[](0u);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] TItem& back() noexcept
    {
        return this->operator[](size() - 1u);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] const TItem& back() const noexcept
    {
        return this->operator[](size() - 1u);
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] void unsafeSetSize(base::SizeT newSize) noexcept
    {
        m_endSize = SFML_BASE_LAUNDER_CAST(TItem*, m_data) + newSize;
    }
};

} // namespace sf::base
