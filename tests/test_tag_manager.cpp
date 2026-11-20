// ============================================================
// TagManager 锟斤拷元锟斤拷锟斤拷
//
// 锟斤拷锟斤拷锟斤拷锟捷ｏ拷
//   1. 锟斤拷锟斤拷 JSON 锟斤拷锟角╋拷锟斤拷锟斤拷欠锟斤拷锟饺?//   2. 锟斤拷锟节的憋拷签锟杰凤拷锟揭碉拷锟斤拷锟街讹拷锟角凤拷锟斤拷确
//   3. 锟斤拷锟斤拷锟节的憋拷签锟斤拷锟斤拷 nullptr
//   4. 锟斤拷锟借备筛选锟角凤拷锟斤拷确
// ============================================================
#include <gtest/gtest.h>
#include "core/tag/TagManager.h"

// TEST_F 锟斤拷 F 锟斤拷示 Fixture锟斤拷锟叫具ｏ拷
// TagManagerTest 锟斤拷一锟斤拷锟洁，SetUp() 锟斤拷每锟斤拷锟斤拷锟斤拷前执锟斤拷
class TagManagerTest : public ::testing::Test
{
protected:
    // SetUp 锟斤拷锟斤拷每锟斤拷 TEST_F 执锟斤拷前锟皆讹拷锟斤拷锟斤拷
    void SetUp() override
    {
        // 锟斤拷锟截诧拷锟斤拷锟斤拷锟斤拷
        TagManager::instance().loadFromJson(R"({
            "tags": [
                {"tag_name":"TE-101","description":"Temperature","unit":"\u2103","data_type":5,"source_device":"PLC-01","source_address":100},
                {"tag_name":"PT-101","description":"Pressure","unit":"MPa","data_type":5,"source_device":"PLC-01","source_address":102},
                {"tag_name":"HV-101","description":"Valve","unit":"","data_type":0,"source_device":"PLC-02","source_address":200}
            ]
        })");
    }
};

// 锟斤拷锟斤拷 1锟斤拷锟斤拷锟截猴拷应锟斤拷恰锟斤拷锟斤拷 3 锟斤拷锟斤拷签
// EXPECT_EQ锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟饺ｏ拷锟斤拷示失锟杰碉拷锟斤拷锟斤拷锟斤拷锟斤拷
TEST_F(TagManagerTest, ShouldLoadCorrectNumberOfTags)
{
    EXPECT_EQ(TagManager::instance().tagCount(), 3);
}

// 锟斤拷锟斤拷 2锟斤拷锟斤拷锟斤拷锟斤拷锟叫的憋拷签应锟斤拷锟斤拷锟斤拷效指锟诫，锟街讹拷值锟斤拷确
TEST_F(TagManagerTest, ShouldFindExistingTag)
{
    const TagDataPoint* tag = TagManager::instance().findTag("TE-101");
    // ASSERT_NE锟斤拷锟斤拷锟?tag == nullptr锟斤拷锟斤拷锟斤拷锟斤拷止锟剿诧拷锟皆ｏ拷锟斤拷锟斤拷拇锟斤拷锟斤拷薹锟斤拷锟斤拷校锟?    ASSERT_NE(tag, nullptr);
    EXPECT_EQ(tag->description, "Temperature");
    EXPECT_EQ(tag->unit, "\u2103");
}

// 锟斤拷锟斤拷 3锟斤拷锟斤拷锟揭诧拷锟斤拷锟节的憋拷签应锟斤拷锟斤拷 nullptr
TEST_F(TagManagerTest, ShouldReturnNullForMissingTag)
{
    const TagDataPoint* tag = TagManager::instance().findTag("NON_EXISTENT");
    EXPECT_EQ(tag, nullptr);
}

// 锟斤拷锟斤拷 4锟斤拷锟斤拷锟借备筛选
TEST_F(TagManagerTest, ShouldFilterTagsByDevice)
{
    auto plc1_tags = TagManager::instance().getTagsByDevice("PLC-01");
    EXPECT_EQ(plc1_tags.size(), 2);  // PLC-01 锟斤拷 2 锟斤拷锟斤拷签

    auto plc2_tags = TagManager::instance().getTagsByDevice("PLC-02");
    EXPECT_EQ(plc2_tags.size(), 1);  // PLC-02 锟斤拷 1 锟斤拷锟斤拷签

    auto none_tags = TagManager::instance().getTagsByDevice("NONEXIST");
    EXPECT_EQ(none_tags.size(), 0);  // 锟斤拷锟斤拷锟节碉拷锟借备锟斤拷锟截匡拷
}