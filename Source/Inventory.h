#pragma once
#include "Block.h"
struct InventorySlot {
    BlockType m_block = BlockType::Empty;
    uint8 m_count = 0;
};

#define MAX_SLOTS 8
#define MAX_BLOCKS std::numeric_limits<decltype(InventorySlot::m_count)>::max()

struct Inventory {
private:
    bool AddBlocks(InventorySlot& slot, auto& count)
    {
        auto availableCount = MAX_BLOCKS - slot.m_count;
        auto incriment = Min(count, availableCount);
        slot.m_count += incriment;
        assert(incriment <= count);
        count -= incriment;
        return count == 0;
    }

public:
    InventorySlot m_slots[MAX_SLOTS] = {};
    uint32 m_slotSelected = {};

    auto Add(BlockType block, auto count)
    {
        assert(count);
        if (count == 0)
            return 0;

        InventorySlot& selectedSlot = HotSlot();
        if (selectedSlot.m_block == block)
            AddBlocks(selectedSlot, count);

        for (uint32 i = 0; i < MAX_SLOTS && count; i++)
        {
            InventorySlot& slot = m_slots[i];

            if (slot.m_block == block)
                AddBlocks(slot, count);
        }

        for (uint32 i = 0; i < MAX_SLOTS && count; i++)
        {
            InventorySlot& slot = m_slots[i];

            if (slot.m_block == BlockType::Empty)
            {
                slot.m_block = block;
                AddBlocks(slot, count);
            }
        }

        return count;
    }
    bool Remove(auto count)
    {
        InventorySlot& slot = HotSlot();
        auto removalAmount = Min(slot.m_count, count);
        slot.m_count -= removalAmount;
        if (slot.m_count == 0)
        {
            slot.m_block = BlockType::Empty;
        }
        return removalAmount == count;
    }
    InventorySlot& HotSlot()
    {
        return m_slots[m_slotSelected];
    }
};
