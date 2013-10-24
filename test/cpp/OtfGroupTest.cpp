/*
 * Copyright 2013 Real Logic Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "gtest/gtest.h"
#include "otf_api/Listener.h"

using namespace sbe::on_the_fly;
using ::std::cout;
using ::std::endl;

#include "OtfMessageTest.h"
#include "OtfMessageTestCBs.h"

class OtfGroupTest : public OtfMessageTest, public OtfMessageTestCBs
{
protected:
#define NUM_ITERATIONS 3
#define GROUP_ID 10

    virtual void constructMessageIr(Ir &ir)
    {
        Ir::TokenByteOrder byteOrder = Ir::SBE_LITTLE_ENDIAN;
        std::string messageStr = std::string("MessageWithRepeatingGroup");
        std::string groupDimensionStr = std::string("groupSizeEncoding");
        std::string groupStr = std::string("GroupName");
        std::string fieldStr = std::string("FieldName");

        ir.addToken(0, 0xFFFFFFFF, Ir::BEGIN_MESSAGE, byteOrder, Ir::NONE, TEMPLATE_ID, messageStr);
        ir.addToken(0, 0, Ir::BEGIN_GROUP, byteOrder, Ir::NONE, GROUP_ID, groupStr);
        ir.addToken(0, 0, Ir::BEGIN_COMPOSITE, byteOrder, Ir::NONE, 0xFFFF, groupDimensionStr);
        ir.addToken(0, 2, Ir::ENCODING, byteOrder, Ir::UINT16, 0xFFFF, std::string("blockLength"));
        ir.addToken(2, 1, Ir::ENCODING, byteOrder, Ir::UINT8, 0xFFFF, std::string("numInGroup"));
        ir.addToken(0, 0, Ir::END_COMPOSITE, byteOrder, Ir::NONE, 0xFFFF, groupDimensionStr);
        ir.addToken(0, 0, Ir::BEGIN_FIELD, byteOrder, Ir::NONE, FIELD_ID, fieldStr);
        ir.addToken(0, 4, Ir::ENCODING, byteOrder, Ir::UINT32, 0xFFFF, std::string("uint32"));
        ir.addToken(0, 0, Ir::END_FIELD, byteOrder, Ir::NONE, FIELD_ID, fieldStr);
        ir.addToken(0, 0, Ir::END_GROUP, byteOrder, Ir::NONE, GROUP_ID, groupStr);
        ir.addToken(0, 0xFFFFFFFF, Ir::END_MESSAGE, byteOrder, Ir::NONE, TEMPLATE_ID, messageStr);
    };

    virtual void constructMessage()
    {
        *((uint16_t *)(buffer_ + bufferLen_)) = 4;
        bufferLen_ += 2;
        buffer_[bufferLen_++] = NUM_ITERATIONS;
        for (int i = 0; i < NUM_ITERATIONS; i++)
        {
            *((uint32_t *)(buffer_ + bufferLen_)) = i;
            bufferLen_ += 4;
        }
    };

    virtual int onNext(const Field &f)
    {
        OtfMessageTestCBs::onNext(f);

        if (numFieldsSeen_ == 2)
        {
            EXPECT_EQ(numGroupsSeen_, 1);
            EXPECT_EQ(f.fieldName(), "FieldName");
        }
        else if (numFieldsSeen_ == 3)
        {
            EXPECT_EQ(numGroupsSeen_, 3);
            EXPECT_EQ(f.fieldName(), "FieldName");
        }
        else if (numFieldsSeen_ == 4)
        {
            EXPECT_EQ(numGroupsSeen_, 5);
            EXPECT_EQ(f.fieldName(), "FieldName");
        }
        return 0;
    };

    virtual int onNext(const Group &g)
    {
        OtfMessageTestCBs::onNext(g);

        EXPECT_EQ(g.name(), "GroupName");
        EXPECT_EQ(g.numInGroup(), NUM_ITERATIONS);
        if (numGroupsSeen_ == 1)
        {
            EXPECT_EQ(g.event(), Group::START);
            EXPECT_EQ(numFieldsSeen_, 1);
            EXPECT_EQ(g.iteration(), 0);
        }
        else if (numGroupsSeen_ == 2)
        {
            EXPECT_EQ(g.event(), Group::END);
            EXPECT_EQ(numFieldsSeen_, 2);
            EXPECT_EQ(g.iteration(), 0);
        }
        else if (numGroupsSeen_ == 3)
        {
            EXPECT_EQ(g.event(), Group::START);
            EXPECT_EQ(numFieldsSeen_, 2);
            EXPECT_EQ(g.iteration(), 1);
        }
        else if (numGroupsSeen_ == 4)
        {
            EXPECT_EQ(g.event(), Group::END);
            EXPECT_EQ(numFieldsSeen_, 3);
            EXPECT_EQ(g.iteration(), 1);
        }
        else if (numGroupsSeen_ == 5)
        {
            EXPECT_EQ(g.event(), Group::START);
            EXPECT_EQ(numFieldsSeen_, 3);
            EXPECT_EQ(g.iteration(), 2);
        }
        else if (numGroupsSeen_ == 6)
        {
            EXPECT_EQ(g.event(), Group::END);
            EXPECT_EQ(numFieldsSeen_, 4);
            EXPECT_EQ(g.iteration(), 2);
        }
        return 0;
    };

    virtual int onError(const Error &e)
    {
        return OtfMessageTestCBs::onError(e);
    };
};

TEST_F(OtfGroupTest, shouldHandleRepeatingGroup)
{
    listener_.dispatchMessageByHeader(std::string("templateId"), messageHeaderIr_, this)
        .resetForDecode(buffer_, bufferLen_)
        .subscribe(this, this, this);
    EXPECT_EQ(numFieldsSeen_, 4);
    EXPECT_EQ(numErrorsSeen_, 0);
    EXPECT_EQ(numCompletedsSeen_, 1);
    EXPECT_EQ(numGroupsSeen_, 6);
}