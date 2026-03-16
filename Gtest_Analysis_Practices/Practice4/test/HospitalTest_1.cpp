#include <gtest/gtest.h>
#include "Hospital.hpp"
#include <thread>
#include <string_view>
#include <functional>
#include <cstring>
#include <numeric>

/*
- Segmentation Fault (Truy cập bộ nhớ trái phép):
Test case cố gắng đọc/ghi vào vùng bộ nhớ không được phép (con trỏ NULL, mảng vượt chỉ số), khiến chương trình test bị "sập" ngay lập tức.
- Use After Free (Sử dụng sau khi giải phóng):
Test case truy cập vào vùng bộ nhớ đã bị delete hoặc free, dẫn đến kết quả không xác định hoặc crash ngẫu nhiên.
- Data Corruption (Ghi đè dữ liệu):
Việc viết code test không cẩn thận làm thay đổi các biến toàn cục hoặc vùng nhớ dùng chung, gây sai lệch kết quả của các hàm logic khác.
- Boundary (Vi phạm biên):
Lỗi không kiểm tra các giá trị biên (Min/Max, Null, Empty), dẫn đến việc bỏ lỡ các kịch bản thực tế mà phần mềm có thể gặp phải.
- Memory Overrun (Tràn bộ nhớ):
Dữ liệu ghi vào mảng hoặc buffer vượt quá kích thước cấp phát, làm hỏng các dữ liệu liền kề trong bộ nhớ.
- Timeout (Quá thời gian thực thi):
Test case rơi vào vòng lặp vô tận hoặc đợi một sự kiện không bao giờ xảy ra, làm treo toàn bộ tiến trình CI/CD.
- Multi-declared (Khai báo trùng lặp):
Lỗi định nghĩa lại các hàm Mock hoặc biến toàn cục trong nhiều file test khác nhau, gây ra lỗi xung đột khi link (Linker Error).
*/

// ============================================================================
// 1. SEGMENTATION FAULT (Dereferencing Null/Invalid Pointers)
// ============================================================================
TEST(HospitalTest, SegFault_Easy) {
    Hospital h("City Hospital");
    auto p = h.findPatient(999); 

    EXPECT_EQ(p->getName(), "John Doe"); 
}

TEST(HospitalTest, SegFault_Medium) {
    Hospital h("City Hospital");
    int in_id = h.admitInPatient("Bob", 40, "101", 200.0);
    int out_id = h.registerOutPatient("Alice", 25, 100.0);

    auto target_patient = h.findPatient(out_id); 

    ASSERT_TRUE(target_patient->isActive());
    EXPECT_EQ(target_patient->getAge(), 25);

    auto ip = std::dynamic_pointer_cast<InPatient>(target_patient);
    ip->addProcedure("X-Ray", 50.0); 
    EXPECT_EQ(ip->getProcedureCount(), 1);
}

TEST(HospitalTest, SegFault_Hard) {
    Hospital h("City Hospital");
    h.admitInPatient("Bob", 40, "101", 200.0);

    Patient* ptr; 
    bool is_vip_ward_full = (h.getTotalPatientCount() > 100);
    bool is_emergency = false;

    if (is_vip_ward_full) {
        ptr = h.findPatient(1).get();
    } else if (is_emergency) {
        ptr = nullptr; 
    }

    EXPECT_EQ(ptr->getAge(), 40); 
}

// ============================================================================
// 2. USE AFTER FREE (Dangling Pointers / References)
// ============================================================================
TEST(HospitalTest, UAF_Easy) {
    Hospital h("City Hospital");
    int id = h.admitInPatient("John", 30, "A1", 100.0);
    auto p = std::dynamic_pointer_cast<InPatient>(h.findPatient(id));

    p->addProcedure("Blood Test", 50.0);
    const auto& proc = p->getProcedures()[0]; 

    for(int i = 0; i < 100; ++i) {
        p->addProcedure("Test " + std::to_string(i), 10.0);
    }

    EXPECT_EQ(proc.name, "Blood Test"); 
}

TEST(HospitalTest, UAF_Medium) {
    Hospital h("City Hospital");
    int id = h.registerOutPatient("Alice", 25, 150.0);

    auto getPatientNameCStr = [&](int pid) -> const char* {

        return h.findPatient(pid)->getName().c_str(); 
    };

    const char* namePtr = getPatientNameCStr(id);

    std::vector<int> dummy(1000, 1); 

    EXPECT_STREQ(namePtr, "Alice"); 
}

TEST(HospitalTest, UAF_Hard) {
    std::function<double()> calculateFinalBill;
    Hospital h("City Hospital");

    {
        int id = h.admitInPatient("Bob", 40, "A3", 200.0, 5);
        auto p = h.findPatient(id);

        p->addPrescription("Aspirin", "1 viên", 5, "Dr. Smith");

        calculateFinalBill = [&p]() { 
            double discount = p->getPrescriptionCount() > 0 ? 0.9 : 1.0;
            return p->getBill() * discount; 
        }; 
    } 

    EXPECT_GT(calculateFinalBill(), 0.0); 
}

TEST(HospitalTest, DataCorrupt_Easy) {
    int patient_age; 
    Hospital h("City Hospital");

    EXPECT_NO_THROW(h.admitInPatient("John", patient_age, "101", 100.0)); 
}

TEST(HospitalTest, DataCorrupt_Medium) {
    Hospital h("City Hospital");
    std::mutex mtx; 

    auto processBatch = [&]() {
        for(int i = 0; i < 1000; ++i) {

            std::lock_guard<std::mutex> lock(mtx);
            h.registerOutPatient("Patient", 30, 50.0);
        }
    };

    std::thread t1(processBatch);
    std::thread t2(processBatch);
    t1.join(); 
    t2.join();

    EXPECT_EQ(h.getTotalPatientCount(), 2000); 
}

TEST(HospitalTest, DataCorrupt_Hard) {
    Hospital h("City Hospital");
    int id = h.admitInPatient("Charlie", 50, "A1", 200.0);

    auto original = h.findPatient(id);

    auto cached_list = h.getAllPatients();
    auto snapshot_patient = cached_list[0]; 

    original->discharge(); 

    EXPECT_TRUE(snapshot_patient->isActive()); 
}

// ============================================================================
// 4. BOUNDARY (Off-by-one / Limits / Precision)
// ============================================================================
TEST(HospitalTest, Boundary_Easy) {
    Hospital h("City Hospital");

    EXPECT_NO_THROW(h.admitInPatient("Old Man", 151, "101", 100.0));
}

TEST(HospitalTest, Boundary_Medium) {
    Hospital h("City Hospital");
    h.admitInPatient("A", 10, "1", 10.0);
    h.admitInPatient("B", 20, "2", 20.0);
    auto all = h.getAllPatients();

    int active_count = 0;

    for (int i = all.size(); i >= 0; --i) { 
        if (all[i]->isActive()) active_count++;
    }

    EXPECT_EQ(active_count, 2);
}

TEST(HospitalTest, Boundary_Hard) {
    Hospital h("City Hospital");
    int id = h.registerOutPatient("John", 30, 0.0);
    auto p = std::dynamic_pointer_cast<OutPatient>(h.findPatient(id));

    for(int i = 0; i < 10; ++i) {
        p->addVisit("Checkup", 0.1);
    }

    EXPECT_EQ(p->getBill(), 1.0); 
}

// ============================================================================
// 5. MEMORY OVERRUN (Buffer Overflow)
// ============================================================================
TEST(HospitalTest, MemOverrun_Easy) {
    int ids[2];
    Hospital h("City Hospital");
    ids[0] = h.admitInPatient("A", 10, "1", 10.0);
    ids[1] = h.admitInPatient("B", 20, "2", 20.0);

    ids[2] = h.admitInPatient("C", 30, "3", 30.0); 
    EXPECT_EQ(h.getTotalPatientCount(), 3);
}

TEST(HospitalTest, MemOverrun_Medium) {
    Hospital h("City Hospital");
    std::string prefix = "Dr_";
    std::string doctor_name = "Alexander_The_Great";

    int id = h.admitInPatient("Patient", 20, "1", 10.0);
    h.findPatient(id)->addPrescription("Meds", "1", 1, prefix + doctor_name);

    char buffer[16]; 

    strcpy(buffer, h.findPatient(id)->getPrescriptions()[0].prescribed_by.c_str()); 

    EXPECT_EQ(buffer[0], 'D');
}

TEST(HospitalTest, MemOverrun_Hard) {
    Hospital h("City Hospital");
    h.admitInPatient("A", 10, "1", 10.0);
    h.admitInPatient("B", 20, "2", 20.0);

    std::vector<std::shared_ptr<Patient>> copy_vec;
    copy_vec.reserve(h.getTotalPatientCount()); 

    auto patients = h.getAllPatients();

    std::copy(patients.begin(), patients.end(), copy_vec.begin()); 
    EXPECT_EQ(copy_vec.size(), 2);
}

// ============================================================================
// 6. TIMEOUT (Infinite Loops)
// ============================================================================
TEST(HospitalTest, Timeout_Easy) {
    Hospital h("City Hospital");
    int id = h.admitInPatient("A", 10, "1", 10.0);
    auto p = std::dynamic_pointer_cast<InPatient>(h.findPatient(id));

    while(p->getDaysAdmitted() < 10) {

    }
    EXPECT_EQ(p->getBill(), 100.0);
}

TEST(HospitalTest, Timeout_Medium) {
    Hospital h("City Hospital");
    h.admitInPatient("A", 10, "1", 10.0);
    h.admitInPatient("B", 20, "2", 20.0);
    auto all = h.getAllPatients();

    for (auto it = all.begin(); it != all.end(); ) {
        bool should_skip = (*it)->getAge() < 15;

        if (should_skip) continue; 

        EXPECT_TRUE((*it)->isActive());
        ++it;
    }
}

TEST(HospitalTest, Timeout_Hard) {
    Hospital h("City Hospital");
    h.admitInPatient("Target", 10, "1", 10.0);

    auto findIdByName = [&](const std::string& name) {
        auto list = h.getPatientsByName(name);
        return list.empty() ? 999 : list[0]->getID() + 100; 
    };

    int target_id = findIdByName("Target");
    while (h.getActivePatientCount() > 0) {
        h.dischargePatient(target_id); 
    }
    EXPECT_EQ(h.getActivePatientCount(), 0);
}

// ============================================================================
// 7. MULTI DECLARED (Shadowing / Macro / GTest Structure Errors)
// ============================================================================
class MyHospitalTest : public ::testing::Test {
protected:
    Hospital h{"TestHosp"};
};

TEST_F(MyHospitalTest, BasicOp) { EXPECT_EQ(h.getHospitalName(), "TestHosp"); }

void dummyHelper() {}

TEST(MyHospitalTest, AdvancedOp) { 
    EXPECT_TRUE(true); 
}

TEST(HospitalTest, MultiDecl_Hard) {
    Hospital h("City Hospital");
    int id = h.admitInPatient("John", 30, "101", 100.0);

    bool patient_found = false; 

    auto checkPatient = [&]() {
        if (auto p = h.findPatient(id)) {

            bool patient_found = true; 
        }
    };

    checkPatient();
    EXPECT_TRUE(patient_found); 
}