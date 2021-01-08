#pragma once

#include <vector>

template<typename ObjectType>
class BulkDataContainer
{
    using ItemID = uint32_t;
public:    
    ItemID AllocateItem();
    void DeleteItem(ItemID item);
    ObjectType & GetUnderlyingItem(ItemID item);

private:
    std::vector<ObjectType> m_items;
};

template<typename T>
BulkDataContainer<T>::ItemID BulkDataContainer<T>::AllocateItem()
{
    m_items.push_back();
    return static_cast<ItemID>(m_items.size());
}

template<typename T>
void BulkDataContainer<T>::DeleteItem(ItemID item)
{
    assert(false);
}

template<typename T>
T & BulkDataContainer<T>::GetUnderlyingItem(ItemID item)
{
    return m_items[item];
}