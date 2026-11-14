#include"vcm/DTC_Manager.hpp"
#include<gtest/gtest.h>
#include<thread>

class TestDtcManager: public ::testing::Test{
    protected:
        DtcManager dtc_manager;
};

//------1.正常上报DTC----
TEST_F(TestDtcManager, ReportDTC){
    dtc_manager.reportDtc(0x0123,0x01);
    dtc_manager.reportDtc(0x123,0x01);
    EXPECT_EQ(dtc_manager.getDtcCount(0x01),1);
}

//----2.状态位的叠加---
TEST_F(TestDtcManager, StatusBit){
    dtc_manager.reportDtc(0x0123,0x01);
    dtc_manager.reportDtc(0x0123,0x04);
    EXPECT_EQ(dtc_manager.getDtcCount(0x05),1);
}

//---3.清除故障状态位---
TEST_F(TestDtcManager, ClearStatusBit){
    dtc_manager.reportDtc(0x0123,0x09);
    dtc_manager.clearDtcStatus(0x0123,0x01);
    EXPECT_EQ(dtc_manager.getDtcCount(0x08),1);
}

//---4.清空故障码---
TEST_F(TestDtcManager, ClearAllDTC){
    dtc_manager.reportDtc(0x0123,0x09);
    dtc_manager.reportDtc(0x0001,0x09);
    dtc_manager.clearAllDtc();
    EXPECT_EQ(dtc_manager.getDtcCount(0x09),0);
}

//---5.获取故障列表---
TEST_F(TestDtcManager, GetList){
    dtc_manager.reportDtc(0x0123,0x09);
    dtc_manager.reportDtc(0x0001,0x09);
    dtc_manager.reportDtc(0x0002,0x04);
    auto list = dtc_manager.getDtcList(0x09);
    EXPECT_EQ(list.size(),2);
}

//---6.混合掩码查询---
TEST_F(TestDtcManager, MixedMask){
    dtc_manager.reportDtc(0x0123,0x09);
    auto list = dtc_manager.getDtcList(0x01);
    EXPECT_EQ(list.size(),1);
}

//---7.全量统计---
TEST_F(TestDtcManager, CountAll){
    dtc_manager.reportDtc(0x0123,0x09);
    EXPECT_EQ(dtc_manager.getDtcCount(0xff),1);
}

//---8.正常老化---
TEST_F(TestDtcManager, AgingCycle){
    dtc_manager.reportDtc(0x0123,0x88);
    for(int i = 0; i < 39; ++i){
        dtc_manager.processAgingCycle();
    }
    EXPECT_EQ(dtc_manager.getDtcCount(0x88),1);
    dtc_manager.processAgingCycle();
    EXPECT_EQ(dtc_manager.getDtcCount(0x88),0);
}

//---9.重复老化---
TEST_F(TestDtcManager, AgingCycleRepeat){
    dtc_manager.reportDtc(0x0123,0x88);
    for(int i = 0; i < 20; ++i){
        dtc_manager.processAgingCycle();
    }
    dtc_manager.reportDtc(0x0123,0x01);
    dtc_manager.processAgingCycle();//重置为40
    dtc_manager.clearDtcStatus(0x0123,0x01);
    for(int i = 0; i < 20; ++i){
        dtc_manager.processAgingCycle();
    }
    EXPECT_EQ(dtc_manager.getDtcCount(0x88),1);
    for(int i = 0; i < 20; ++i){
        dtc_manager.processAgingCycle();
    }
    EXPECT_EQ(dtc_manager.getDtcCount(0x88),0);
}

//---10超限测试清除不存在的故障
TEST_F(TestDtcManager, ClearNonExist){
    dtc_manager.clearDtcStatus(0x0123,0x01);
}

//---11.TSAN并发测试
TEST_F(TestDtcManager, Tsan){
    std::thread produce1([&]{
        for(int i = 0; i < 20;++i){
            uint32_t virtual_code = 0x0100 | (uint8_t)(i);
            dtc_manager.reportDtc(virtual_code,0x08);
        }
    });
     std::thread produce2([&]{
        for(int i = 0; i < 20;++i){
            uint32_t virtual_code = 0x0200 | (uint8_t)(i);
            dtc_manager.reportDtc(virtual_code,0x08);
        }
    });
     std::thread produce3([&]{
        for(int i = 0; i < 20;++i){
            uint32_t virtual_code = 0x0300 | (uint8_t)(i);
            dtc_manager.reportDtc(virtual_code,0x08);
        }
    });
     std::thread produce4([&]{
        for(int i = 0; i < 20;++i){
            uint32_t virtual_code = 0x0400 | (uint8_t)(i);
            dtc_manager.reportDtc(virtual_code,0x08);
        }
    });

    std::thread consume1([&]{
        for(int i = 0; i < 20;++i){
            uint32_t virtual_code = 0x0100 | (uint8_t)(i);
            dtc_manager.clearDtcStatus(virtual_code,0x08);
        }
    });
     std::thread consume2([&]{
        for(int i = 0; i < 20;++i){
            uint32_t virtual_code = 0x0200 | (uint8_t)(i);
            dtc_manager.clearDtcStatus(virtual_code,0x08);
        }
    });
     std::thread consume3([&]{
        for(int i = 0; i < 20;++i){
            uint32_t virtual_code = 0x0300 | (uint8_t)(i);
            dtc_manager.clearDtcStatus(virtual_code,0x08);
        }
    });
     std::thread consume4([&]{
        for(int i = 0; i < 20;++i){
            uint32_t virtual_code = 0x0400 | (uint8_t)(i);
            dtc_manager.clearDtcStatus(virtual_code,0x08);
        }
    });
    produce1.join();
    produce2.join();
    produce3.join();
    produce4.join();
    consume1.join();
    consume2.join();
    consume3.join();
    consume4.join();
    
    EXPECT_TRUE(true);
}